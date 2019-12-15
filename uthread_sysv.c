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
#include <stdint.h> //for uintptr_t
#include <stdlib.h> //for abort/malloc
#include <string.h> //for memset
#include <ucontext.h>

struct uthread_executor_t {
  uthread_t *threads;
  ucontext_t join_ctx;
  size_t current;
  size_t count;
  size_t capacity;
  size_t stopped;
};

struct uthread_t {
  void (*func)(void *, void *);
  void *func_arg;
  uthread_executor_t *pexec;
  uthread_state state;
  ucontext_t ctx;
  unsigned char stack[1024 * 4]; // 4kb
};

static uthread_t *uthread_impl_update_next(uthread_t *current_thread);
static void uthread_impl_mark_aborted(uthread_t *);
static void uthread_impl_functor_wrapper(uint32_t, uint32_t);

uthread_executor_t *uthread_exec_create(size_t capacity) {
  uthread_executor_t *exec =
      malloc(sizeof(uthread_executor_t) + sizeof(uthread_t) * (capacity + 1));
  if (exec == 0)
    return 0;
  exec->threads = (uthread_t *)exec + 1;
  exec->count = 0;
  exec->capacity = capacity;
  exec->stopped = 0;
  return exec;
}

_Static_assert(CHAR_BIT == 8 // make sure a byte == 8 bit
                   && ((sizeof(void *) == 4 &&
                        sizeof(int) == sizeof(void *)) // 32bit
                       || (sizeof(void *) == 8 &&
                           sizeof(void *) / sizeof(int) == 2)), // 64bit
               "currently we only support 32bit and 64bit platforms");

int uthread_create(uthread_executor_t *exec, void (*func)(void *, void *),
                   void *func_arg) {
  if (exec->capacity == exec->count)
    return 0;
  uthread_t *new_thread = &exec->threads[exec->count];
  new_thread->func = func;
  new_thread->func_arg = func_arg;
  new_thread->pexec = exec;
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
  exec->count++;
  return 1;
}

void uthread_exec_join(uthread_executor_t *exec) {
  if (exec->count > 0) {
    exec->current = 0;
    exec->threads[0].state = RUNNING;
    swapcontext(&exec->join_ctx, &exec->threads[0].ctx);
  }
}

void uthread_exec_destroy(uthread_executor_t *exec) { free(exec); }

void uthread_yield(void *handle) {
  uthread_t *current_thread = (uthread_t *)handle;
  assert(current_thread->pexec->count - current_thread->pexec->stopped != 0);
  if (current_thread->pexec->count - current_thread->pexec->stopped == 1)
    return;

  current_thread->state = YIELDED;
  uthread_t *next = uthread_impl_update_next(current_thread);
  next->state = RUNNING;
  if (swapcontext(&current_thread->ctx, &next->ctx) != 0) {
    uthread_impl_mark_aborted(current_thread);
    abort();
  }
}

void uthread_exit(void *handle) {
  uthread_t *current_thread = (uthread_t *)handle;
  current_thread->state = STOPPED;
  current_thread->pexec->stopped++;

  if (current_thread->pexec->count == current_thread->pexec->stopped) {
    setcontext(&current_thread->pexec->join_ctx);
  } else {
    uthread_t *next = uthread_impl_update_next(current_thread);
    next->state = RUNNING;
    setcontext(&next->ctx);
  }
}

static uthread_t *uthread_impl_update_next(uthread_t *current_thread) {
  size_t *current_index = &current_thread->pexec->current;
  *current_index = (*current_index + 1) % current_thread->pexec->count;
  return &current_thread->pexec->threads[*current_index];
}

static void uthread_impl_mark_aborted(uthread_t *handle) {
  handle->state = ABORTED;
  handle->pexec->stopped++;
}

static void uthread_impl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread, thread->func_arg);
}

#undef _XOPEN_SOURCE
