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
#ifdef UTHREAD_HOOK_STDOUT
#include <inttypes.h> //stackoverflow.com/questions/5795978/string-format-for-intptr-t-and-uintptr-t
#include <stdio.h>
#endif
#undef _GNU_SOURCE
#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

uthread_executor_t *uthread_impl_current_exec();
bool uthread_impl_init_epoll(uthread_executor_t *);

/*
static void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  UTHREAD_CHECK(flags >= 0, "fcntl failed");
  if ((flags & O_NONBLOCK) != 0)
    return; // already set to non-block mode
  UTHREAD_CHECK(fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0, "fcntl failed");
}
*/

static void epoll_add(uthread_t *handle, int epoll, int fd, int events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = events;
  ev.data.ptr = handle;
  UTHREAD_CHECK(!epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ev), "epoll_ctl failed");
}

// switch to a uthread that has epoll event pending on
static void epoll_switch(int epoll, uthread_t *cur_uthread) {
  struct epoll_event ev;
  UTHREAD_CHECK(epoll_wait(epoll, &ev, 1, -1) >= 0, "epoll_wait failed");
  // under current implementation we only have EPOLLIN
  assert(ev.events & EPOLLIN != 0);
  if (ev)
}

#ifdef __cplusplus
extern "C" {
#endif

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("accept(%d, %" PRIxPTR ", %" PRIxPTR ")\n", sockfd, (uintptr_t)addr,
         (uintptr_t)addrlen);
#endif

  typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
  accept_t real = (accept_t)dlsym(RTLD_NEXT, "accept");

  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif

  if (!cur_exec) {
    // not calling from uthread
    return real(sockfd, addr, addrlen);
  }

  // calling are from uthread
  int unread = 0;
  UTHREAD_CHECK(ioctl(sockfd, FIONREAD, &unread) != -1, "ioctl failed");
  if (unread > 0) {
    // if there is already an incoming client are waiting
    return real(sockfd, addr, addrlen);
  } else {
    // otherwise, adding the listening socket into epoll
    // 1. create epoll for the current executor if it doesn't exist
    UTHREAD_CHECK(uthread_impl_init_epoll(cur_exec),
                  "can not initialize epoll");
    // 2. add listening socket into epoll's interest list
    epoll_ctl_helper(&cur_exec->threads[cur_exec->current], cur_exec->epoll,
                     sockfd, EPOLLIN);
  }
}

/*
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("connect(%d, %" PRIxPTR ", %" PRIxPTR ")\n", sockfd, (uintptr_t)addr,
         (uintptr_t)addrlen);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(cur_exec, "connect are not calling under uthread");
  UTHREAD_CHECK(uthread_impl_init_epoll(cur_exec), "can not initialize epoll");

  typedef int (*connect_t)(int, const struct sockaddr *, socklen_t);
  connect_t real = (connect_t)dlsym(RTLD_NEXT, "connect");
  return real(sockfd, addr, addrlen);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("send(%d, %" PRIxPTR ", %zu, %d)\n", sockfd, (uintptr_t)buf, len,
         flags);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(cur_exec, "send are not calling under uthread");

  typedef ssize_t (*send_t)(int, const void *, size_t, int);
  send_t real = (send_t)dlsym(RTLD_NEXT, "send");
  return real(sockfd, buf, len, flags);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("recv(%d, %" PRIxPTR ", %zu, %d)\n", sockfd, (uintptr_t)buf, len,
         flags);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(cur_exec, "recv are not calling under uthread");

  typedef ssize_t (*recv_t)(int, void *, size_t, int);
  recv_t real = (recv_t)dlsym(RTLD_NEXT, "recv");
  return real(sockfd, buf, len, flags);
}

int close(int fd) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("close(%d)\n", fd);
#endif
  typedef int (*close_t)(int);
  close_t real = (close_t)dlsym(RTLD_NEXT, "close");
  return real(fd);
}
*/

#ifdef __cplusplus
}
#endif
