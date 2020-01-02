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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> //read & write

/*
hook the following system calls:
accept, connect, send, recv
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  accept_t real_accept = (accept_t)dlsym(RTLD_NEXT, "accept");
  printf("hook ok!!!\n");
  //return real_accept(sockfd, addr, addrlen);
  return 9710;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t send(int sockfd, const void *buf, size_t len, int flags);

ssize_t recv(int sockfd, void *buf, size_t len, int flags);

#ifdef __cplusplus
}
#endif
