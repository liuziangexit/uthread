/**
 * Copyright (C) 2019 liuziangexit. All rights reserved.
 * Licensed under the GNU General Public License v3.0 License (the "License");
 * you may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 * https://www.gnu.org/licenses/gpl-3.0.html
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#define _GNU_SOURCE
#include <dlfcn.h> //for dlsym
#undef _GNU_SOURCE
#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include "../common/vector.c"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// BEGIN internal functions
bool uimpl_switch_to(struct uthread_executor_t *exec, struct uthread_t *thread);
struct uthread_t *uimpl_next(struct uthread_executor_t *exec, bool allow_back);
struct uthread_t *uimpl_threadp(struct uthread_executor_t *exec,
                                uthread_id_t id);

static bool uimpl_epoll_init(struct uthread_executor_t *exec) {
  if (exec->epoll < 0) {
    int new_epoll = epoll_create(1);
    if (new_epoll < 0) {
      UTHREAD_DEBUG_PRINT("create epoll failed\n");
      return false;
    }
    exec->epoll = new_epoll;
    UTHREAD_DEBUG_PRINT("create epoll succeeded\n");
  }
  return true;
}

_Static_assert(sizeof(uint64_t) == sizeof(uthread_id_t) ||
                   sizeof(uint32_t) == sizeof(uthread_id_t) ||
                   sizeof(uint16_t) == sizeof(uthread_id_t) ||
                   sizeof(uint8_t) == sizeof(uthread_id_t),
               "what?");
static void uimpl_epoll_add(int epoll, struct uthread_t *handle, int fd,
                            int events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = events;
  ev.data.u64 = (uint64_t)handle->id;
  UTHREAD_CHECK(epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ev) != -1,
                "uimpl_epoll_add failed");
}

static bool uimpl_epoll_del(int epoll, int fd) {
  return epoll_ctl(epoll, EPOLL_CTL_DEL, fd, 0) != -1;
}

// return the previous option
static bool uimpl_set_nonblock(int fd, bool enable) {
  int flags = fcntl(fd, F_GETFL, 0);
  UTHREAD_CHECK(flags != -1, "fcntl failed");
  bool prev = flags & O_NONBLOCK;
  if (enable)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;
  UTHREAD_CHECK(fcntl(fd, F_SETFL, flags) != -1, "fcntl failed");
  return prev;
}

// switch to a uthread that has a epoll event pending on
static void uimpl_epoll_switch(int epoll, struct uthread_executor_t *exec) {
  struct uthread_t *current = uimpl_threadp(exec, exec->current_thread);
  current->state = WAITING_IO;
  struct uthread_t *next = uimpl_next(exec, false);
  UTHREAD_CHECK(next, "uimpl_epoll_switch failed");
  uimpl_switch_to(exec, next);
}
// END internal functions

// BEGIN hook implementation
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
#ifdef UTHREAD_DEBUG
  // TODO check the fd is actually on LISTEN mode
#endif
  typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
  accept_t real = (accept_t)dlsym(RTLD_NEXT, "accept");
  if (!real) {
    UTHREAD_ABORT("retrieve system function accept with dlsym failed");
  }
  struct uthread_executor_t *exec = uthread_current_executor();
  if (!exec) {
    // not calling from uthread
    return real(sockfd, addr, addrlen);
  }
  // calling from uthread
  int unread = 0;
  // TODO linux不支持在listen的sock上用ioctl
  // FIONREAD，搞明白epoll是怎么做到的，然后在这里用那个方法
  // 补充：看起来用select可以做到，不知道性能如何？实际上，本质问题应该描述为“如何检测linux上的文件描述符是否可读？”
  // UTHREAD_CHECK(ioctl(sockfd, FIONREAD, &unread) != -1, "ioctl failed");
  if (unread > 0) {
    // if there is already an incoming client are waiting
    return real(sockfd, addr, addrlen);
  } else {
    /*otherwise switch to another uthread because there is noting to do for
    this uthread except wait*/
    // 1. create epoll for the current executor if it doesn't exist
    UTHREAD_CHECK(uimpl_epoll_init(exec), "can not create epoll");
    // 2. add listening socket into epoll's interest list
    uimpl_epoll_add(exec->epoll, uimpl_threadp(exec, exec->current_thread),
                    sockfd, EPOLLIN);
    // 3. switch to other uthread
    uimpl_epoll_switch(exec->epoll, exec);
    // 4. when the control flow goes back, remove the socket from epoll
    // interest list
    if (!uimpl_epoll_del(exec->epoll, sockfd)) {
      UTHREAD_ABORT("uimpl_epoll_del failed");
    }
    // 5. do accept
    return real(sockfd, addr, addrlen);
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  typedef int (*connect_t)(int, const struct sockaddr *, socklen_t);
  connect_t real = (connect_t)dlsym(RTLD_NEXT, "connect");
  if (!real) {
    UTHREAD_ABORT("retrieve system function connect with dlsym failed");
  }
  struct uthread_executor_t *exec = uthread_current_executor();
  if (!exec) {
    // not calling from uthread
    return real(sockfd, addr, addrlen);
  }
  // calling from uthread
  /*
  based on http://man7.org/linux/man-pages/man2/connect.2.html
  asynchronous call the connect, and switch to other uthread.
  once the control flow goes back we check the err code and just return it.
  */
  // 1. create epoll for the current executor if it doesn't exist
  UTHREAD_CHECK(uimpl_epoll_init(exec), "can not create epoll");
  // 2. add connecting socket into epoll's interest list
  uimpl_epoll_add(exec->epoll, uimpl_threadp(exec, exec->current_thread),
                  sockfd, EPOLLOUT);
  // 3. set the fd to non-block mode
  bool nonblock = uimpl_set_nonblock(sockfd, true);
  // 4. call connect
  int ret = real(sockfd, addr, addrlen);
  if (ret != 0 && errno != EAGAIN && errno != EINPROGRESS) {
    return -1;
  }
  // 5. switch to other uthread
  uimpl_epoll_switch(exec->epoll, exec);
  // 6. when the control flow goes back, remove the socket from epoll interest
  // list
  if (!uimpl_epoll_del(exec->epoll, sockfd)) {
    UTHREAD_ABORT("uimpl_epoll_del failed");
  }
  // 7. check whether the operation completed successfully
  int err;
  socklen_t errlen = sizeof(err);
  UTHREAD_CHECK(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) != -1,
                "getsockopt failed");
  // 8. recover the previous non-block option
  if (!nonblock) {
    uimpl_set_nonblock(sockfd, false);
  }
  // 9. return the err code of the connect operation
  if (err == 0) {
    return 0;
  } else {
    errno = err;
    return -1;
  }
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  typedef ssize_t (*send_t)(int, const void *, size_t, int);
  send_t real = (send_t)dlsym(RTLD_NEXT, "send");
  if (!real) {
    UTHREAD_ABORT("retrieve system function send with dlsym failed");
  }
  struct uthread_executor_t *exec = uthread_current_executor();
  if (!exec) {
    // not calling from uthread
    return real(sockfd, buf, len, flags);
  }
  // calling from uthread
  // 1. create epoll for the current executor if it doesn't exist
  UTHREAD_CHECK(uimpl_epoll_init(exec), "can not create epoll");
  // 2. set the fd to non-block mode
  bool nonblock = uimpl_set_nonblock(sockfd, true);
  // 3. call non-blocking send
  int ret = real(sockfd, buf, len, flags);
  if (ret != -1) {
    // send ok
    return ret;
  }
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    // something goes wrong
    return -1;
  }
  // send operation are pending
  // 4. add connecting socket into epoll's interest list
  uimpl_epoll_add(exec->epoll, uimpl_threadp(exec, exec->current_thread),
                  sockfd, EPOLLOUT);
  // 5. switch to other uthread
  uimpl_epoll_switch(exec->epoll, exec);
  // 6. when the control flow goes back, remove the socket from epoll interest
  // list
  if (!uimpl_epoll_del(exec->epoll, sockfd)) {
    UTHREAD_ABORT("uimpl_epoll_del failed");
  }
  // 7. recover the previous non-block option
  if (!nonblock) {
    uimpl_set_nonblock(sockfd, false);
  }
  // 8. return the number of bytes sent
  return len;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  typedef ssize_t (*recv_t)(int sockfd, void *buf, size_t len, int flags);
  recv_t real = (recv_t)dlsym(RTLD_NEXT, "recv");
  if (!real) {
    UTHREAD_ABORT("retrieve system function recv with dlsym failed");
  }
  struct uthread_executor_t *exec = uthread_current_executor();
  if (!exec) {
    // not calling from uthread
    return real(sockfd, buf, len, flags);
  }
  // calling from uthread
  int unread = 0;
  UTHREAD_CHECK(ioctl(sockfd, FIONREAD, &unread) != -1, "ioctl failed");
  if (unread > 0) {
    // if there is unread bytes
    return real(sockfd, buf, len, flags);
  } else {
    // 1. create epoll for the current executor if it doesn't exist
    UTHREAD_CHECK(uimpl_epoll_init(exec), "can not create epoll");
    // 2. add listening socket into epoll's interest list
    uimpl_epoll_add(exec->epoll, uimpl_threadp(exec, exec->current_thread),
                    sockfd, EPOLLIN);
    // 3. switch to other uthread
    uimpl_epoll_switch(exec->epoll, exec);
    // 4. when the control flow goes back, remove the socket from epoll
    // interest list
    if (!uimpl_epoll_del(exec->epoll, sockfd)) {
      UTHREAD_ABORT("uimpl_epoll_del failed");
    }
    // 5. do recv
    return real(sockfd, buf, len, flags);
  }
}

int close(int fd) {
  typedef int (*close_t)(int);
  close_t real = (close_t)dlsym(RTLD_NEXT, "close");
  struct uthread_executor_t *exec = uthread_current_executor();
  if (!exec) {
    // not calling from uthread
    return real(fd);
  }
  // remove fd from epoll
  if (exec->epoll >= 0 && fd != exec->epoll) {
    if (!uimpl_epoll_del(exec->epoll, fd)) {
      if (errno != ENOENT) {
        UTHREAD_ABORT("close failed");
      }
    }
  }
  return real(fd);
}

// END hook implementation

#ifdef __cplusplus
}
#endif
