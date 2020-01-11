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

#ifndef __UTHREAD_SYSV_DEF_H__
#define __UTHREAD_SYSV_DEF_H__

/* Defining this macro to 600 causes header files to expose definitions for
SUSv3 (UNIX 03; i.e., the POSIX.1-2001 base specification plus the XSI
extension)*/
#define _XOPEN_SOURCE 600
#include <ucontext.h>
#undef _XOPEN_SOURCE

struct uthread_executor_t {
  uthread_t *threads;
  ucontext_t join_ctx;
  size_t current;
  size_t count;
  size_t capacity;
  size_t stopped;
};

struct uthread_t {
  void (*func)(uthread_t *, void *);
  void *func_arg;
  uthread_executor_t *exec;
  uthread_state state;
  ucontext_t ctx;
  unsigned char stack[1024 * 4]; // 4kb
  int epoll;
};
#endif