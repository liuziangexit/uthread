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

// prevent darwin kernel
#ifdef __APPLE__
//#error "NO DARWIN"
#endif

/* Defining this macro to 600 causes header files to expose definitions for
SUSv3 (UNIX 03; i.e., the POSIX.1-2001 base specification plus the XSI
extension)*/
#define _XOPEN_SOURCE 600

#include "uthread.h"
#include <assert.h>
#include <stddef.h> //for size_t
#include <stdlib.h> //for abort/malloc
#include <string.h> //for memset
#include <ucontext.h>

struct uthread_executor_t {
  uthread_t *threads;
  uthread_t *running;
  ucontext_t ctlr_ctx;
  size_t thread_count;
  size_t max_count;
  size_t stopped_count;
};

struct uthread_t {
  uthread_state state;
  ucontext_t ctx;
  unsigned char stack[1024 * 4]; // 4kb
};

static _Thread_local uthread_executor_t *pexec;
int uthread_impl_resume(uthread_t *thread);
void uthread_impl_mark_aborted(uthread_t *thread);

uthread_executor_t *uthread_exec_create(unsigned int max_thread_count) {
  uthread_executor_t *exec = malloc(sizeof(uthread_executor_t) +
                                    sizeof(uthread_t) * (max_thread_count + 1));
  if (exec == 0)
    return 0;
  exec->threads = (uthread_t *)exec + 1;
  exec->running = 0;
  exec->thread_count = 0;
  exec->max_count = max_thread_count;
  exec->stopped_count = 0;
  return exec;
}

int uthread_create(uthread_executor_t *executor, void (*func)(void *),
                   void *func_arg) {
  if (executor->thread_count == executor->max_count)
    return 0;
  uthread_t *new_thread = &executor->threads[executor->thread_count];
  new_thread->state = CREATED;
  // create context
  if (getcontext(&new_thread->ctx) != 0)
    return 0;
  // set up stack
  new_thread->ctx.uc_stack.ss_sp = &new_thread->stack;
  new_thread->ctx.uc_stack.ss_size = sizeof(new_thread->stack);
  // ctx will begin with func
  makecontext(&new_thread->ctx, (void (*)())func, 1, func_arg);
  executor->thread_count++;
  return 1;
}

void uthread_exec_join(uthread_executor_t *executor) {
  pexec = executor;
  for (size_t i = 0;; i = (i + 1) % executor->thread_count) {
    uthread_t *thread = &executor->threads[i];
    if (thread->state == STOPPED || thread->state == ABORTED) {
      if (executor->stopped_count == executor->thread_count)
        return;
      else
        continue;
    }
    if (thread->state == CREATED || thread->state == YIELDED) {
      if (!uthread_impl_resume(thread))
        uthread_impl_mark_aborted(thread);
      continue;
    }
    abort();
  }
}

void uthread_exec_destroy(uthread_executor_t *exec) { free(exec); }

void uthread_yield() {
  pexec->running->state = YIELDED;
  if (swapcontext(&pexec->running->ctx, &pexec->ctlr_ctx) != 0) {
    uthread_impl_mark_aborted(pexec->running);
    abort();
  }
}

void uthread_exit() {
  pexec->running->state = STOPPED;
  pexec->stopped_count++;
  setcontext(&pexec->ctlr_ctx);
}

int uthread_impl_resume(uthread_t *thread) {
  uthread_state prev_state = thread->state;
  thread->state = RUNNING;
  pexec->running = thread;
  if (prev_state == CREATED || prev_state == YIELDED) {
    if (swapcontext(&pexec->ctlr_ctx, &thread->ctx) != 0)
      return 0;
    else
      return 1;
  }
  abort();
}

void uthread_impl_mark_aborted(uthread_t *thread) {
  thread->state = ABORTED;
  pexec->stopped_count++;
}
