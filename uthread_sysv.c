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

// prevent compile under darwin kernel
#ifdef __APPLE__
#error "NO DARWIN"
#endif

/* Defining this macro to 600 causes header files to expose definitions for
SUSv3 (UNIX 03; i.e., the POSIX.1-2001 base specification plus the XSI
extension)*/
#define _XOPEN_SOURCE 600

#include "uthread.h"
#include <assert.h>
#include <limits.h> //for CHAR_BIT
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h> //for size_t
#include <stdint.h> //for uintptr_t
#include <stdlib.h> //for abort/malloc
#include <string.h> //for memset
#include <ucontext.h>

struct uthread_executor_t {
  uthread_t *threads;
  ucontext_t ctlr_ctx;
  size_t thread_count;
  size_t max_count;
  size_t stopped_count;
};

struct uthread_t {
  void (*func)(void *, void *);
  void *func_arg;
  uthread_executor_t *pexec;
  uthread_state state;
  ucontext_t ctx;
  unsigned char stack[1024 * 4]; // 4kb
};

static int uthread_impl_resume(uthread_t *);
static void uthread_impl_mark_aborted(uthread_t *);
static void uthread_impl_functor_wrapper(uint32_t, uint32_t);

uthread_executor_t *uthread_exec_create(unsigned int max_thread_count) {
  uthread_executor_t *exec = malloc(sizeof(uthread_executor_t) +
                                    sizeof(uthread_t) * (max_thread_count + 1));
  if (exec == 0)
    return 0;
  exec->threads = (uthread_t *)exec + 1;
  exec->thread_count = 0;
  exec->max_count = max_thread_count;
  exec->stopped_count = 0;
  return exec;
}

_Static_assert(CHAR_BIT == 8 // make sure a byte == 8 bit
                   && ((sizeof(void *) == 4 &&
                        sizeof(int) == sizeof(void *)) // 32bit
                       || (sizeof(void *) == 8 &&
                           sizeof(void *) / sizeof(int) == 2)), // 64bit
               "currently we only support 32bit and 64bit platforms");

int uthread_create(uthread_executor_t *executor, void (*func)(void *, void *),
                   void *func_arg) {
  if (executor->thread_count == executor->max_count)
    return 0;
  uthread_t *new_thread = &executor->threads[executor->thread_count];
  new_thread->func = func;
  new_thread->func_arg = func_arg;
  new_thread->pexec = executor;
  new_thread->state = CREATED;
  // create context
  if (getcontext(&new_thread->ctx) != 0)
    return 0;
  // set up stack
  new_thread->ctx.uc_stack.ss_sp = &new_thread->stack;
  new_thread->ctx.uc_stack.ss_size = sizeof(new_thread->stack);
  // ctx will begin with func
  makecontext(&new_thread->ctx, (void (*)())uthread_impl_functor_wrapper, 2,
              (uint32_t)(uintptr_t)new_thread,
              (uint32_t)((uintptr_t)new_thread >> 32));
  executor->thread_count++;
  return 1;
}

void uthread_exec_join(uthread_executor_t *executor) {
  for (size_t i = 0;; i = (i + 1) % executor->thread_count) {
    uthread_t *thread = &executor->threads[i];
    if (thread->state == STOPPED || thread->state == ABORTED) {
      if (executor->stopped_count == executor->thread_count)
        return;
      else
        continue;
    }
    if (thread->state == CREATED || thread->state == YIELDED) {
      if (!uthread_impl_resume(&executor->threads[i]))
        uthread_impl_mark_aborted(&executor->threads[i]);
      continue;
    }
    abort();
  }
}

void uthread_exec_destroy(uthread_executor_t *exec) { free(exec); }

void uthread_yield(void *handle) {
  ((uthread_t *)handle)->state = YIELDED;
  if (swapcontext(&((uthread_t *)handle)->ctx,
                  &((uthread_t *)handle)->pexec->ctlr_ctx) != 0) {
    uthread_impl_mark_aborted((uthread_t *)handle);
    abort();
  }
}

void uthread_exit(void *handle) {
  ((uthread_t *)handle)->state = STOPPED;
  ((uthread_t *)handle)->pexec->stopped_count++;
  setcontext(&((uthread_t *)handle)->pexec->ctlr_ctx);
}

static int uthread_impl_resume(uthread_t *handle) {
  uthread_state prev_state = handle->state;
  handle->state = RUNNING;
  if (prev_state == CREATED || prev_state == YIELDED) {
    if (swapcontext(&handle->pexec->ctlr_ctx, &handle->ctx) != 0)
      return 0;
    else
      return 1;
  }
  abort();
}

static void uthread_impl_mark_aborted(uthread_t *handle) {
  handle->state = ABORTED;
  handle->pexec->stopped_count++;
}

static void uthread_impl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread, thread->func_arg);
}

#undef _XOPEN_SOURCE
