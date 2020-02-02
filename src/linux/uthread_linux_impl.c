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

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 8)
#error "linux kernel before 2.6.8 is not supported"
#endif

#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include "uthread_linux_impl_cur_ut.c"
#include <stdint.h> //for int32_t etc...
#include <stdlib.h> // malloc
#include <unistd.h> //close

static void uimpl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread->func_arg);
  UTHREAD_ABORT("uthread quit without calling uthread_exit");
}

uthread_executor_t *uimpl_exec(uthread_t *ut) {
  void *vut = ut;
  vut -= ut->idx * sizeof(uthread_t);
  vut -= sizeof(uthread_executor_t);
  return (uthread_executor_t *)vut;
}

static uthread_t *uimpl_next(uthread_t *cur) {
  uthread_executor_t *exec = uimpl_exec(cur);
#ifdef UTHREAD_DEBUG
  UTHREAD_CHECK(exec->current == cur->idx, "uimpl_next check failed");
#endif
  if (exec->stopped == exec->count - 1) {
    return 0;
  }
  size_t next_idx = cur->idx + 1;
  uthread_t *next = &exec->threads[next_idx++ % exec->count];
  while (next->state == STOPPED) {
    next = &exec->threads[next_idx++ % exec->count];
  }
#ifdef UTHREAD_DEBUG
  UTHREAD_CHECK(next != cur, "uimpl_next check failed");
#endif
  return next;
}

static void uimpl_switch(uthread_t *cur, uthread_t *to,
                         uthread_state cur_state) {
#ifdef UTHREAD_STRICTLY_CHECK
  UTHREAD_CHECK(cur->state == RUNNING && to->state != STOPPED &&
                    to->state != RUNNING,
                "uimpl_switch check failed");
#endif
  cur->state = cur_state;
  to->state = RUNNING;
  uimpl_exec(cur)->current = to->idx;
  uimpl_set_cur_ut(to);
  if (swapcontext(&cur->ctx, &to->ctx) != 0) {
    UTHREAD_ABORT("uimpl_switch swapcontext failed");
  }
}

_Static_assert(_Alignof(uthread_executor_t) == _Alignof(uthread_t),
               "the alignment requirement of uthread_executor_t and "
               "uthread_t can not be matched");
void *uthread_create(enum uthread_clsid clsid, enum uthread_error *err, ...) {
  if (clsid == EXECUTOR_CLS) {
    va_list args;
    va_start(args, err);
    size_t init_cap = va_arg(args, size_t);
    void *(*alloc)(size_t) = va_arg(args, void *(*)(size_t));
    va_end(args);

    // if custom allocator==0 use malloc
    if (!alloc) {
      alloc = malloc;
    }
    uthread_executor_t *new_exec =
        alloc(sizeof(uthread_executor_t) + sizeof(uthread_t) * init_cap);
    if (new_exec == 0) {
      // memory allocation failed
      *err = MEMORY_ALLOCATION_ERR;
      return 0;
    }
    new_exec->threads = (uthread_t *)(new_exec + 1);
    new_exec->count = 0;
    new_exec->capacity = init_cap;
    new_exec->stopped = 0;
    // epoll file descriptor set to -1 represent NULL
    new_exec->epoll = -1;
    *err = OK;
    return new_exec;
  } else if (clsid == UTHREAD_CLS) {
    va_list args;
    va_start(args, err);
    uthread_executor_t *exec = va_arg(args, uthread_executor_t *);
    void (*job)(void *) = va_arg(args, void (*)(void *));
    void *job_arg = va_arg(args, void *);
    va_end(args);

    if (exec->capacity == exec->count) {
      *err = BAD_ARGUMENTS_ERR;
      return 0;
    }
    uthread_t *new_thread = &exec->threads[exec->count];
    new_thread->func = job;
    new_thread->func_arg = job_arg;
    new_thread->state = CREATED;
    new_thread->idx = exec->count;
    // create context
    if (getcontext(&new_thread->ctx) != 0) {
      *err = SYSTEM_CALL_ERR;
      return 0;
    }
    // setup context stack
    new_thread->ctx.uc_stack.ss_sp = &new_thread->stack;
    new_thread->ctx.uc_stack.ss_size = sizeof(new_thread->stack);
    new_thread->ctx.uc_link = 0;
    // cxt will begin with job_func
    makecontext(&new_thread->ctx, (void (*)())uimpl_functor_wrapper, 2,
                (uint32_t)(uintptr_t)new_thread,
                (uint32_t)((uintptr_t)new_thread >> 32));
    exec->count++;
    *err = OK;
    return new_thread;
  } else {
    *err = BAD_ARGUMENTS_ERR;
    return 0;
  }
}

enum uthread_error uthread_join(uthread_executor_t *exec) {
  if (exec->count > 0) {
    exec->current = 0;
    exec->threads[0].state = RUNNING;
    if (!uimpl_set_cur_ut(&exec->threads[0]))
      return SYSTEM_CALL_ERR;
    if (swapcontext(&exec->join_ctx, &exec->threads[0].ctx) == -1)
      return SYSTEM_CALL_ERR;
  }
  return OK;
}

void uthread_destroy(enum uthread_clsid clsid, void *obj,
                     void (*dealloc)(void *)) {
  if (clsid == EXECUTOR_CLS) {
    uthread_executor_t *obj_typed = (uthread_executor_t *)obj;
    // 1.check all uthread are quited
    UTHREAD_CHECK(obj_typed->stopped == obj_typed->count,
                  "some uthreads are not stopped yet");

    if (obj_typed->epoll >= 0) {
      // 2.close epoll
      close(obj_typed->epoll);
    }
    // 3.free
    if (dealloc == 0)
      dealloc = free;
    dealloc(obj);
  } else {
    UTHREAD_ABORT("uthread_destroy wrong clsid");
  }
}

void uthread_switch(uthread_t *to) {
  uimpl_switch(uimpl_cur_ut(), to, YIELDED);
}

void uthread_yield() {
  uthread_t *next = uimpl_next(uimpl_cur_ut());
  if (next)
    uthread_switch(next);
}

void uthread_exit() {
  uthread_t *cur = uimpl_cur_ut();
  uthread_executor_t *exec = uimpl_exec(cur);
  if (exec->count == exec->stopped + 1) {
    cur->state = STOPPED;
    exec->stopped++;
    UTHREAD_CHECK(uimpl_set_cur_ut(0), "uthread_exit uimpl_set_cur_ut failed");
    setcontext(&exec->join_ctx);
  } else {
    uthread_t *next = uimpl_next(cur);
    exec->stopped++;
    uimpl_switch(cur, next, STOPPED);
  }
}

void *uthread_current(uthread_clsid clsid) {
  if (clsid == UTHREAD_CLS) {
    return uimpl_cur_ut();
  } else if (clsid == EXECUTOR_CLS) {
    uthread_t *cur_ut = uimpl_cur_ut();
    if (cur_ut)
      return uimpl_exec(cur_ut);
    else
      return 0;
  } else {
    abort();
  }
}
