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
#include "uthread_linux_impl_cur_ut.c"
#include <stdint.h> //for int32_t etc...
#include <stdlib.h> // malloc

static void uimpl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread, thread->func_arg);
  // if func doesn't call uthread_exit, then abort()
  abort();
}

static uthread_executor_t *uimpl_exec(uthread_t *ut) {
  void *vut = ut;
  vut -= ut->idx * sizeof(uthread_t);
  vut -= sizeof(uthread_executor_t);
  return (uthread_executor_t *)vut;
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
    void (*job)(uthread_t *, void *) =
        va_arg(args, void (*)(uthread_t *, void *));
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
    if (obj_typed->stopped != obj_typed->count)
      abort();

    if (obj_typed->epoll != -1) {
      /*
      2.check there is no fd are currently combined on the epoll(fd
       will be automatically unregister from the epoll when the system call
      'close' has been called)
      */

      // 3.close epoll
      // how to close the epoll?
    }
    // 4.free
    if (dealloc == 0)
      dealloc = free;
    dealloc(obj);
  } else if (clsid == UTHREAD_CLS) {
    abort(); // you can't destory a uthread!
  } else {
    // what?
    abort();
  }
}

void uthread_switch(uthread_t *to);

void uthread_yield();

void uthread_exit();
