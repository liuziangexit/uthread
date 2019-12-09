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

#include "uthread.h"
#include <stdlib.h> //for abort
#include <string.h> //for memset

#ifdef __APPLE__
/* Defining this macro causes header files to expose definitions for SUSv3\
(UNIX 03; i.e., the POSIX.1-2001 base specification plus the XSI extension)*/
#define _XOPEN_SOURCE 600
#include "ucontext.h"
#undef _XOPEN_SOURCE
#endif

struct uthread_executor_t {
  uthread_t *threads;
  unsigned long thread_count;
  unsigned long max_thread_count;
  unsigned long stopped_thread_count;
  ucontext_t controller_context;
};

struct uthread_t {
  void (*func)(void *);
  void *func_arg;
  ucontext_t context;
  uthread_state state;
};

int uthread_exec_create(uthread_executor_t *executor,
                        unsigned long max_thread_count,
                        void *(*alloctor)(unsigned int)) {
  void *threads = alloctor(sizeof(uthread_t) * max_thread_count);
  if (threads == 0)
    return 1;
  executor->threads = threads;
  executor->thread_count = 0;
  executor->max_thread_count = max_thread_count;
  executor->stopped_thread_count = 0;
  memset(&(executor->controller_context), 1, sizeof(ucontext_t));
  return 0;
}

int uthread_create(uthread_executor_t *executor, void (*func)(void *),
                   void *func_arg) {
  if (executor->thread_count == executor->max_thread_count)
    return 1;
  uthread_t *new_thread = &(executor->threads[executor->thread_count++]);
  new_thread->func = func;
  new_thread->func_arg = func_arg;
  new_thread->state = CREATED;
  return 0;
}

int uthread_exec_join(uthread_executor_t *executor) {
  for (int i = 0; i < executor->thread_count; i++) {
    uthread_t *thread = &(executor->threads[i]);
    if (thread->state == STOPPED) {
      if (executor->stopped_thread_count == executor->thread_count) {
        return 0;
      }
      continue;
    }

    if (thread->state == CREATED || thread->state == YIELDED) {
      uthread_resume(executor, thread);
      continue;
    } else {
      abort();
    }
  }
}

void uthread_exec_destroy(uthread_executor_t *executor,
                          void (*dealloctor)(void *)) {
  dealloctor(executor->threads);
}

// internal usage
void uthread_resume(uthread_executor_t *executor, uthread_t *thread) {}

// call from uthread
void uthread_yield() {}
void uthread_exit() {}
uthread_executor_t *uthread_get_exec() {}
