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
#ifdef UTHREAD_DEBUG
#include <inttypes.h> //stackoverflow.com/questions/5795978/string-format-for-intptr-t-and-uintptr-t
#include <stdio.h>
#endif
#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

uthread_t *uimpl_next(uthread_t *cur);
void uimpl_switch(uthread_t *cur, uthread_t *to, uthread_state cur_state);

static bool uimpl_epoll_init(uthread_executor_t *exec) {
  if (exec->epoll < 0) {
    int new_epoll = epoll_create(1);
    if (new_epoll < 0) {
      return false;
    }
    exec->epoll = new_epoll;
  }
  return true;
}

static void uimpl_epoll_add(int epoll, uthread_t *handle, int fd, int events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = events;
  ev.data.ptr = handle;
  UTHREAD_CHECK(epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ev) != -1,
                "uimpl_epoll_add epoll_ctl failed");
}

static void uimpl_epoll_del(int epoll, int fd) {
  UTHREAD_CHECK(epoll_ctl(epoll, EPOLL_CTL_DEL, fd, 0) != -1,
                "uimpl_epoll_add epoll_ctl failed");
}

// switch to a uthread that has a epoll event pending on
static void uimpl_epoll_switch(int epoll, uthread_t *cur_uthread) {
  uthread_t *next = uimpl_next(cur_uthread);
  if (!next) {
    cur_uthread->state = WAITING_IO;
    struct epoll_event ev;
#ifdef UTHREAD_DEBUG
    printf("[exec: %" PRIxPTR ", ut: %" PRIxPTR "]: begin epoll_wait\n",
           (uintptr_t)uthread_current(EXECUTOR_CLS),
           (uintptr_t)uthread_current(UTHREAD_CLS));
#endif
    UTHREAD_CHECK(epoll_wait(epoll, &ev, 1, -1) == 1, "epoll_wait failed");
#ifdef UTHREAD_DEBUG
    printf("[exec: %" PRIxPTR ", ut: %" PRIxPTR "]: end epoll_wait\n",
           (uintptr_t)uthread_current(EXECUTOR_CLS),
           (uintptr_t)uthread_current(UTHREAD_CLS));
#endif
    next = ev.data.ptr;
    cur_uthread->state = RUNNING;
  }
  // if another uthread(the fd bind with it) becomes readable, we switch to
  // that thread.
  uimpl_switch(cur_uthread, next, WAITING_IO);
  cur_uthread->state = RUNNING;
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

#ifdef __cplusplus
extern "C" {
#endif

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
#ifdef UTHREAD_DEBUG
  // TODO check the fd is actually on LISTEN mode
#endif
#ifdef UTHREAD_DEBUG
  printf("[exec: %" PRIxPTR ", ut: %" PRIxPTR "]: accept(%d, %" PRIxPTR
         ", %" PRIxPTR ")\n",
         (uintptr_t)uthread_current(EXECUTOR_CLS),
         (uintptr_t)uthread_current(UTHREAD_CLS), sockfd, (uintptr_t)addr,
         (uintptr_t)addrlen);
#endif
  typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
  accept_t real = (accept_t)dlsym(RTLD_NEXT, "accept");
#ifdef UTHREAD_DEBUG
  UTHREAD_CHECK(real, "retrieve system function accept with dlsym failed")
#endif
  uthread_executor_t *cur_exec = uthread_current(EXECUTOR_CLS);
  if (!cur_exec) {
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
    /* otherwise switch to another uthread because there is noting to do for
    this uthread except wait*/
    // 1. create epoll for the current executor if it doesn't exist
    UTHREAD_CHECK(uimpl_epoll_init(cur_exec), "can not create epoll");
    // 2. add listening socket into epoll's interest list
    uimpl_epoll_add(cur_exec->epoll, &cur_exec->threads[cur_exec->current],
                    sockfd, EPOLLIN);
    // 3. switch to other uthread
    uimpl_epoll_switch(cur_exec->epoll, &cur_exec->threads[cur_exec->current]);
    // 4. when the control flow goes back, remove the socket from epoll
    // interest list
    uimpl_epoll_del(cur_exec->epoll, sockfd);
    // 5. do accept
    return real(sockfd, addr, addrlen);
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
#ifdef UTHREAD_DEBUG
  printf("[exec: %" PRIxPTR ", ut: %" PRIxPTR "]: connect(%d, %" PRIxPTR
         ", %d)\n",
         (uintptr_t)uthread_current(EXECUTOR_CLS),
         (uintptr_t)uthread_current(UTHREAD_CLS), sockfd, (uintptr_t)addr,
         (int)addrlen);
#endif
  typedef int (*connect_t)(int, const struct sockaddr *, socklen_t);
  connect_t real = (connect_t)dlsym(RTLD_NEXT, "connect");
#ifdef UTHREAD_DEBUG
  UTHREAD_CHECK(real, "retrieve system function connect with dlsym failed")
#endif
  uthread_executor_t *cur_exec = uthread_current(EXECUTOR_CLS);
  if (!cur_exec) {
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
  UTHREAD_CHECK(uimpl_epoll_init(cur_exec), "can not create epoll");
  // 2. add connecting socket into epoll's interest list
  uimpl_epoll_add(cur_exec->epoll, &cur_exec->threads[cur_exec->current],
                  sockfd, EPOLLOUT);
  // 3. set the fd to non-block mode
  bool nonblock = uimpl_set_nonblock(sockfd, true);
  // 4. call connect
  int ret = real(sockfd, addr, addrlen);
  if (ret != 0 && errno != EAGAIN && errno != EINPROGRESS) {
    return -1;
  }
  // 5. switch to other uthread
  uimpl_epoll_switch(cur_exec->epoll, &cur_exec->threads[cur_exec->current]);
  // 6. when the control flow goes back, remove the socket from epoll interest
  // list
  uimpl_epoll_del(cur_exec->epoll, sockfd);
  // 7. check whether the operation completed successfully
  int err;
  socklen_t errlen = sizeof(err);
  UTHREAD_CHECK(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) != -1,
                "getsockopt failed");
  // 8. recover the previous non-block option
  if (!nonblock) {
    uimpl_set_nonblock(sockfd, false);
  }
  // 9. return the err code of connect operation
  if (err == 0) {
    return 0;
  } else {
    errno = err;
    return -1;
  }
}
/*
ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
#ifdef UTHREAD_DEBUG
  printf("send(%d, %" PRIxPTR ", %zu, %d)\n", sockfd, (uintptr_t)buf, len,
         flags);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_DEBUG
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(cur_exec, "send are not calling under uthread");

  typedef ssize_t (*send_t)(int, const void *, size_t, int);
  send_t real = (send_t)dlsym(RTLD_NEXT, "send");
  return real(sockfd, buf, len, flags);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
#ifdef UTHREAD_DEBUG
  printf("recv(%d, %" PRIxPTR ", %zu, %d)\n", sockfd, (uintptr_t)buf, len,
         flags);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_DEBUG
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(cur_exec, "recv are not calling under uthread");

  typedef ssize_t (*recv_t)(int, void *, size_t, int);
  recv_t real = (recv_t)dlsym(RTLD_NEXT, "recv");
  return real(sockfd, buf, len, flags);
}

int close(int fd) {
#ifdef UTHREAD_DEBUG
  printf("close(%d)\n", fd);
#endif
  typedef int (*close_t)(int);
  close_t real = (close_t)dlsym(RTLD_NEXT, "close");
  return real(fd);
}
*/

#undef CURRENT_EXECUTOR

#ifdef __cplusplus
}
#endif
