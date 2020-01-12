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

#ifndef __linux__
#error "this implementation is only for linux"
#endif

#include "../../include/uthread.h"
#include <assert.h>
#include <limits.h> //for CHAR_BIT
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h> //for uintptr_t
#include <stdlib.h> //for abort/malloc
#include <string.h> //for memset
#include <sys/epoll.h>

// BEGIN internal functions
static uthread_t *uthread_impl_update_next(uthread_t *current_thread) {
  size_t *current_index = &current_thread->exec->current;
  *current_index = (*current_index + 1) % current_thread->exec->count;
  return &current_thread->exec->threads[*current_index];
}

static void uthread_impl_mark_aborted(uthread_t *handle) {
  handle->state = ABORTED;
  handle->exec->stopped++;
}

static void uthread_impl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread, thread->func_arg);
  // if fuc doesn't call uthread_exit, then abort()
  abort();
}

// OS Thread to current running uthread
static pthread_key_t __cur_uthread_key;
static pthread_once_t __cur_uthread_key_once = PTHREAD_ONCE_INIT;

static void uthread_impl_del_key() {
  if (pthread_key_delete(__cur_uthread_key))
    abort();
}
static void uthread_impl_init_key() {
  if (pthread_key_create(&__cur_uthread_key, uthread_impl_del_key))
    abort();
}
static void uthread_impl_set_current_uthread(uthread_t *c) {
  if (pthread_once(&__cur_uthread_key_once, uthread_impl_init_key))
    abort();
  if (pthread_setspecific(__cur_uthread_key, (void *)c))
    abort();
}

// expose for the access from hook_impl.c
uthread_t *uthread_impl_current_uthread() {
  if (pthread_once(&__cur_uthread_key_once, uthread_impl_init_key))
    abort();
  return (uthread_t *)pthread_getspecific(__cur_uthread_key);
}
// END internal functions

// BEGIN API implementations
uthread_executor_t *uthread_exec_create(size_t capacity) {
  uthread_executor_t *exec =
      malloc(sizeof(uthread_executor_t) + sizeof(uthread_t) * (capacity + 1));
  if (exec == 0)
    return 0;
  _Static_assert(_Alignof(uthread_executor_t) == _Alignof(uthread_t),
                 "the alignment requirement of uthread_executor_t and "
                 "uthread_t can not be matched");
  exec->threads = (uthread_t *)(exec + 1);
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

int uthread_create(uthread_executor_t *exec, void (*func)(uthread_t *, void *),
                   void *func_arg) {
  if (exec->capacity == exec->count)
    return 0;

  uthread_t *new_thread = &exec->threads[exec->count];
  new_thread->func = func;
  new_thread->func_arg = func_arg;
  new_thread->exec = exec;
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
  // epoll file descriptor set to -1 represent NULL
  new_thread->epoll = -1;

  exec->count++;
  return 1;
}

void uthread_exec_join(uthread_executor_t *exec) {
  if (exec->count > 0) {
    exec->current = 0;
    exec->threads[0].state = RUNNING;
    uthread_impl_set_current_uthread(&exec->threads[0]);
    swapcontext(&exec->join_ctx, &exec->threads[0].ctx);
  }
}

void uthread_exec_destroy(uthread_executor_t *exec) { free(exec); }

void uthread_yield(uthread_t *current_thread) {
  assert(current_thread->exec->count - current_thread->exec->stopped != 0);
  if (current_thread->exec->count - current_thread->exec->stopped == 1)
    return;

  current_thread->state = YIELDED;
  uthread_t *next = uthread_impl_update_next(current_thread);
  next->state = RUNNING;
  uthread_impl_set_current_uthread(next);
  if (swapcontext(&current_thread->ctx, &next->ctx) != 0) {
    uthread_impl_mark_aborted(current_thread);
    abort();
  }
}

void uthread_exit(uthread_t *current_thread) {
  current_thread->state = STOPPED;
  current_thread->exec->stopped++;

  if (current_thread->exec->count == current_thread->exec->stopped) {
    uthread_impl_set_current_uthread(0);
    setcontext(&current_thread->exec->join_ctx);
  } else {
    uthread_t *next = uthread_impl_update_next(current_thread);
    next->state = RUNNING;
    uthread_impl_set_current_uthread(next);
    setcontext(&next->ctx);
  }
}
// END API implementations
