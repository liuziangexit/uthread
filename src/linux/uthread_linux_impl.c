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

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 8)
#error "linux kernel before 2.6.8 is not supported"
#endif

#include "../../include/uthread.h"
#include "../common/log.h"
#include "../common/vector.h"
#include <stdbool.h>
#include <stdlib.h> // malloc
#include <sys/epoll.h>
#include <ucontext.h>
#include <unistd.h> // close

// BEGIN tls store
#include <pthread.h>

static pthread_key_t uthread_tls_key;
static int uthread_tls_key_created = 0;

static void uimpl_del_tls() {
  if (pthread_key_delete(uthread_tls_key))
    UTHREAD_ABORT("uimpl_del_tls failed");
}
static void uimpl_set_current(struct uthread_t *ut) {
  if (!uthread_tls_key_created) {
    if (pthread_key_create(&uthread_tls_key, uimpl_del_tls))
      UTHREAD_ABORT("uimpl_set_current failed");
    uthread_tls_key_created = 1;
  }
  if (pthread_setspecific(uthread_tls_key, (void *)ut))
    UTHREAD_ABORT("uimpl_set_current failed");
}
struct uthread_t *uimpl_current() {
  if (!uthread_tls_key_created)
    return 0;
  return (struct uthread_t *)pthread_getspecific(uthread_tls_key);
}
// END tls store

// BEGIN internal functions
static inline bool uimpl_has_flag(enum uthread_state state, int flag) {
  return state & flag;
}

static void uimpl_wrapper() {
  struct uthread_t *thread = uimpl_current();
  printf("uimpl_wrapper %zu\n", thread->id);
  thread->job(thread->job_arg);
  UTHREAD_ABORT("uthread quit without calling uthread_exit");
}

struct uthread_t *uimpl_threadp(struct uthread_executor_t *exec,
                                uthread_id_t id) {
  UTHREAD_CHECK(id != UTHREAD_INVALID_ID, "uimpl_threadp check failed");
  return (struct uthread_t *)uthread_vector_get(&exec->threads, id);
}

bool uimpl_switch_to(struct uthread_executor_t *exec, struct uthread_t *thread,
                     enum uthread_state prev_state) {
  // log支持格式化字符串
  fprintf(stdout, "switch from %zu to %zu\n", exec->current_thread, thread->id);
  UTHREAD_CHECK(thread->exec == exec, "uimpl_switch_to check failed");
  if (exec->current_thread == thread->id) {
    return true;
  }
  if (exec->current_thread != UTHREAD_INVALID_ID) {
    struct uthread_t *prev = uimpl_threadp(exec, exec->current_thread);
    prev->state = prev_state;
    thread->state = RUNNING;
    exec->current_thread = thread->id;
    uimpl_set_current(thread);
    if (swapcontext(&prev->ctx, &thread->ctx) != 0) {
      return false;
    }
  } else {
    thread->state = RUNNING;
    exec->current_thread = thread->id;
    uimpl_set_current(thread);
    if (swapcontext(&exec->join_ctx, &thread->ctx) != 0) {
      return false;
    }
  }
  return true;
}

struct uthread_t *uimpl_next(struct uthread_executor_t *exec, bool allow_back) {
  // 1. check the threads that are pending on IO
  if (exec->epoll >= 0) {
    struct epoll_event ev;
    // this call will not block
    int ev_cnt = epoll_wait(exec->epoll, &ev, 1, 0);
    if (ev_cnt == -1) {
      UTHREAD_ABORT("epoll_wait failed");
    }
    if (ev_cnt == 1) {
      return uimpl_threadp(exec, (uthread_id_t)ev.data.u64);
    }
  }
  // 2. look for a thread that has been YIELDED
  uthread_id_t next_idx = exec->current_thread + 1;
  struct uthread_t *next =
      uimpl_threadp(exec, next_idx++ % uthread_vector_size(&exec->threads));
  while (next->id != exec->current_thread &&
         !uimpl_has_flag(next->state, UTHREAD_FLAG_SCHEDULABLE)) {
    next =
        uimpl_threadp(exec, next_idx++ % uthread_vector_size(&exec->threads));
  }
  if (next->id != exec->current_thread)
    return next;
  if (allow_back)
    return uimpl_threadp(exec, exec->current_thread);
  // 3.waiting on epoll
  if (exec->epoll >= 0) {
    struct epoll_event ev;
    // this call will block
    int ev_cnt = epoll_wait(exec->epoll, &ev, 1, -1);
    if (ev_cnt != 1) {
      UTHREAD_ABORT("epoll_wait failed");
    }
    return uimpl_threadp(exec, (uthread_id_t)ev.data.u64);
  }
  UTHREAD_ABORT("illegal state")
}
// END internal functions

// BEGIN uthread.h implementation
enum uthread_error uthread_executor(struct uthread_executor_t *exec,
                                    void *(*alloc)(size_t),
                                    void (*dealloc)(void *)) {
  if (!alloc) {
    alloc = malloc;
    dealloc = free;
  }
  if (!uthread_vector_init(&exec->threads, 15, sizeof(struct uthread_t), alloc,
                           dealloc))
    return MEMORY_ALLOCATION_FAILED;
  exec->current_thread = UTHREAD_INVALID_ID;
  exec->schedulable_cnt = 0;
  exec->epoll = -1;
  return OK;
}

void uthread_executor_destroy(struct uthread_executor_t *exec) {
  // 1.check all uthread are quited
  UTHREAD_CHECK(exec->schedulable_cnt == 0,
                "some uthreads are not stopped yet");
  // 2.close epoll
  if (exec->epoll >= 0) {
    close(exec->epoll);
  }
  // 3.free vector
  uthread_vector_destroy(&exec->threads);
}

uthread_id_t uthread(struct uthread_executor_t *exec, void (*job)(void *),
                     void *job_arg, enum uthread_error *err) {
  size_t new_thread_idx = exec->threads.used;
  // search for STOPPED uthread and take its place
  for (size_t i = 0; i < exec->threads.used; i++) {
    struct uthread_t *slot = uimpl_threadp(exec, i);
    if (slot->state == STOPPED) {
      new_thread_idx = i;
      break;
    }
  }
  // if there is no STOPPED uthread then we add new place
  if (new_thread_idx == exec->threads.used) {
    if (!uthread_vector_expand(&exec->threads, 1)) {
      if (err)
        *err = MEMORY_ALLOCATION_FAILED;
      return UTHREAD_INVALID_ID;
    }
  }
  // initial uthread
  struct uthread_t *new_thread = uimpl_threadp(exec, new_thread_idx);
  new_thread->job = job;
  new_thread->job_arg = job_arg;
  new_thread->state = CREATED;
  new_thread->exec = exec;
  new_thread->id = new_thread_idx;
  if (getcontext(&new_thread->ctx) != 0) {
    new_thread->state = STOPPED;
    if (err)
      *err = SYSTEM_CALL_FAILED;
    return UTHREAD_INVALID_ID;
  }
  new_thread->ctx.uc_stack.ss_sp = &new_thread->stack;
  new_thread->ctx.uc_stack.ss_size = sizeof(new_thread->stack);
  new_thread->ctx.uc_link = 0;
  makecontext(&new_thread->ctx, uimpl_wrapper, 0);
  // ok
  exec->schedulable_cnt++;
  if (err)
    *err = OK;
  return new_thread_idx;
}

enum uthread_error uthread_join(struct uthread_executor_t *exec) {
  if (exec->schedulable_cnt > 0) {
    if (!uimpl_switch_to(exec, uimpl_threadp(exec, 0), 0)) {
      return SYSTEM_CALL_FAILED;
    }
  }
  return OK;
}

void uthread_yield() {
  struct uthread_executor_t *exec = uthread_current_executor();
  struct uthread_t *next = uimpl_next(exec, true);
  UTHREAD_CHECK(next, "uthread_yield failed");
  if (!uimpl_switch_to(exec, next, YIELDED))
    UTHREAD_ABORT("uthread_yield failed");
}

void uthread_exit() {
  struct uthread_executor_t *exec = uthread_current_executor();
  if (exec->schedulable_cnt == 1) {
    uimpl_threadp(exec, exec->current_thread)->state = STOPPED;
    exec->schedulable_cnt--;
    uimpl_set_current(0);
    setcontext(&exec->join_ctx);
  } else {
    struct uthread_t *next = uimpl_next(exec, false);
    UTHREAD_CHECK(next, "uthread_exit failed");
    exec->schedulable_cnt--;
    uimpl_switch_to(exec, next, STOPPED);
    UTHREAD_ABORT("illegal state");
  }
}

struct uthread_executor_t *uthread_current_executor() {
  struct uthread_t *ut = uimpl_current();
  if (ut)
    return ut->exec;
  return 0;
}

struct uthread_t *uthread_current_thread() {
  return uimpl_current();
}
// END uthread.h implementation
