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
#include <sys/socket.h>
#include <unistd.h>

uthread_executor_t *uthread_impl_current_exec();
bool uthread_impl_init_epoll(uthread_executor_t *);

static void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  UTHREAD_CHECK(flags < 0, "fcntl failed");
  if ((flags & O_NONBLOCK) != 0)
    return; // already set to non-block mode
  UTHREAD_CHECK(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0, "fcntl failed");
}

#ifdef __cplusplus
extern "C" {
#endif

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("accept(%d, %" PRIxPTR ", %" PRIxPTR ")\n", sockfd, (uintptr_t)addr,
         (uintptr_t)addrlen);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(!cur_exec, "accept are not calling under uthread");
  UTHREAD_CHECK(!uthread_impl_init_epoll(cur_exec), "can not initialize epoll");

  // set non-block mode
  set_nonblock(sockfd);

  typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
  accept_t real = (accept_t)dlsym(RTLD_NEXT, "accept");
  return real(sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
#ifdef UTHREAD_HOOK_STDOUT
  printf("connect(%d, %" PRIxPTR ", %" PRIxPTR ")\n", sockfd, (uintptr_t)addr,
         (uintptr_t)addrlen);
#endif
  uthread_executor_t *cur_exec = uthread_impl_current_exec();
#ifdef UTHREAD_HOOK_STDOUT
  printf("cur_exec == %" PRIxPTR "\n", (uintptr_t)cur_exec);
#endif
  UTHREAD_CHECK(!cur_exec, "connect are not calling under uthread");
  UTHREAD_CHECK(!uthread_impl_init_epoll(cur_exec), "can not initialize epoll");

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
  UTHREAD_CHECK(!cur_exec, "send are not calling under uthread");

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
  UTHREAD_CHECK(!cur_exec, "recv are not calling under uthread");

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

#ifdef __cplusplus
}
#endif
