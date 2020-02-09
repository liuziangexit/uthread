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

#ifdef UTHREAD_DEBUG
#include <inttypes.h> //stackoverflow.com/questions/5795978/string-format-for-intptr-t-and-uintptr-t
#include <stdio.h>
#endif

#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include "../common/vector.h"
#include "uthread_linux_impl_cur_ut.c"
#include <stdint.h> //for int32_t etc...
#include <stdlib.h> // malloc
#include <sys/epoll.h>
#include <unistd.h> //close

static void uimpl_functor_wrapper(uint32_t low, uint32_t high) {
  uthread_t *thread = (uthread_t *)((uintptr_t)low | ((uintptr_t)high << 32));
  thread->func(thread->func_arg);
  UTHREAD_ABORT("uthread quit without calling uthread_exit");
}

uthread_t *uimpl_next(uthread_t *cur) {
  uthread_executor_t *exec = cur->exec;
  if (exec->stopped == exec->created - 1) {
    return 0;
  }
  // 1. check the threads that are pending on IO
  if (exec->epoll >= 0) {
    struct epoll_event ev;
    // this call will not block
    int ev_cnt = epoll_wait(exec->epoll, &ev, 1, 0);
    UTHREAD_CHECK(ev_cnt != -1, "epoll_wait failed");
    if (ev_cnt == 1) {
      return ev.data.ptr;
    }
  }
  // 2. look for a thread that has been YIELDED
  size_t next_idx = exec->current + 1;
  uthread_t *next = uthread_vector_get(
      &exec->threads, next_idx++ % uthread_vector_size(&exec->threads));
  while (next != cur && next->state != YIELDED && next->state != CREATED) {
    next = uthread_vector_get(&exec->threads,
                              next_idx++ % uthread_vector_size(&exec->threads));
  }
  if (next == cur)
    return 0; // not found
  return next;
}

void uimpl_switch(size_t cur, size_t to, uthread_state cur_state) {
#ifdef UTHREAD_DEBUG
  printf("[exec: %" PRIxPTR ", ut: %" PRIxPTR "]: switch to %" PRIxPTR "\n",
         (uintptr_t)uthread_current(EXECUTOR_CLS),
         (uintptr_t)uthread_current(UTHREAD_CLS), (uintptr_t)to);
#endif
  if (cur == to)
    return;
#ifdef UTHREAD_DEBUG
  UTHREAD_CHECK(cur->state == RUNNING && to->state != STOPPED &&
                    to->state != RUNNING,
                "uimpl_switch check failed");
#endif
  cur->state = cur_state;
  to->state = RUNNING;
  cur->exec->current = to->idx;
  uimpl_set_cur_ut(to);
  if (swapcontext(&cur->ctx, &to->ctx) != 0) {
    UTHREAD_ABORT("uimpl_switch swapcontext failed");
  }
}

uthread_error uthread_executor_init(struct uthread_executor_t *exec,
                                    void *(*alloc)(size_t),
                                    void (*dealloc)(void *)) {
  // if custom allocator==0 use malloc
  if (!alloc) {
    alloc = malloc;
    dealloc = free;
  }
  uthread_vector_init(exec->threads, 15, sizeof(uthread_t), alloc, dealloc);
  uthread_vector_init(exec->threads_gaps, 15, sizeof(size_t), alloc, dealloc);
  exec->created = 0;
  exec->stopped = 0;
  // epoll file descriptor set to -1 represent NULL
  exec->epoll = -1;
  return OK;
}

void uthread_executor_destroy(struct uthread_executor_t *exec) {
  // 1.check all uthread are quited
  UTHREAD_CHECK(exec->stopped == exec->created,
                "some uthreads are not stopped yet");

  if (exec->epoll >= 0) {
    // 2.close epoll
    close(exec->epoll);
  }
  // 3.destroy vectors
  uthread_vector_destroy(&exec->threads);
  uthread_vector_destroy(&exec->threads_gaps);
}

uthread_error uthread_new(struct uthread_executor_t *exec, void (*func)(void *),
                          void *func_arg, size_t **idx_out) {
  // 1.setting uthread_t
  uthread_t new_thread;
  new_thread->func = func;
  new_thread->func_arg = func_arg;
  new_thread->state = CREATED;
  new_thread->exec = exec;
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

  size_t new_thread_idx;
  // 2.check is there any gap inside uthread vector
  if (uthread_vector_size(&exec->threads_gaps) != 0) {
    // using gap
    new_thread_idx = *(size_t *)uthread_vector_get(
        &exec->threads_gaps, uthread_vector_size(&exec->threads_gaps) - 1);
    uthread_vector_remove(&exec->threads_gaps,
                          uthread_vector_size(&exec->threads_gaps) - 1);
    memcpy(uthread_vector_get(&exec->uthreads, new_thread_idx), &new_thread,
           sizeof(uthread_t));
  } else {
    // no gap
    if (!uthread_vector_add(&exec->uthreads, &new_thread))
      return MEMORY_ALLOCATION_ERR;
  }
  exec->created++;
  if (idx_out)
    *idx_out = new_thread_idx;
  return OK;
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
  uthread_executor_t *exec = cur->exec;
  uthread_vector_add(&exec->uthreads_gaps, exec->current);
  if (exec->created == exec->stopped + 1) {
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
      return cur_ut->exec;
    else
      return 0;
  } else {
    abort();
  }
}
