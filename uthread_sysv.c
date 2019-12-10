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

/* Defining those macro causes header files to expose definitions for
SUSv3\ (UNIX 03; i.e., the POSIX.1-2001 base specification plus the XSI
extension)*/
#define _XOPEN_SOURCE 600

#include "uthread.h"
#include <assert.h>
#include <stddef.h> //for size_t
#include <stdio.h>
#include <stdlib.h> //for abort
#include <string.h> //for memset
#include <ucontext.h>

struct uthread_t {
  uthread_state state;
  ucontext_t ctx;
  unsigned char stack[1024 * 16]; // 16kb
};

struct uthread_executor_t {
  unsigned int thread_count;
  unsigned int max_count;
  unsigned int stopped_count;
  uthread_t *running;
  ucontext_t ctlr_ctx;
  // uthread_t *threads;
  uthread_t threads[2];
};

static uthread_executor_t *pexec;
// static enum { BACKTO_UTHREAD, BACKTO_CONTROLLER } switch_state;
int uthread_resume(uthread_executor_t *executor, uthread_t *thread);
void mark_aborted(uthread_executor_t *executor, uthread_t *thread);

uthread_executor_t *uthread_exec_create(unsigned long max_thread_count,
                                        void *(*alloctor)(size_t)) {
  // uthread_executor_t *exec = alloctor(sizeof(uthread_executor_t) +
  //                                  sizeof(uthread_t) * max_thread_count);
  uthread_executor_t *exec = alloctor(sizeof(uthread_executor_t));
  if (exec == 0)
    return 0;
  // exec->threads = (uthread_t *)(&exec->threads) + 1;
  exec->thread_count = 0;
  exec->max_count = max_thread_count;
  exec->stopped_count = 0;
  exec->running = 0;
  memset(&exec->ctlr_ctx, 0, sizeof(ucontext_t));
  return exec;
}

int uthread_create(uthread_executor_t *executor, void (*func)(void *),
                   void *func_arg) {
  if (executor->thread_count == executor->max_count)
    return 0;
  uthread_t *new_thread = &executor->threads[executor->thread_count];
  new_thread->state = CREATED;
  memset(&new_thread->ctx, 0, sizeof(ucontext_t));
  // create context
  if (getcontext(&new_thread->ctx) != 0)
    return 0;
  // set up stack
  new_thread->ctx.uc_stack.ss_sp = &new_thread->stack;
  new_thread->ctx.uc_stack.ss_size = sizeof(new_thread->stack);
  new_thread->ctx.uc_link = 0;
  // ctx will begin with func
  makecontext(&new_thread->ctx, func, 0);
  executor->thread_count++;
  return 1;
}

void uthread_exec_join(uthread_executor_t *executor) {
  pexec = executor;
  for (int i = 0;; i == executor->thread_count ? i = 0 : i++) {
    uthread_t *thread = &executor->threads[i];
    if (thread->state == STOPPED || thread->state == ABORTED) {
      if (executor->stopped_count == executor->thread_count) {
        return;
      }
      continue;
    }

    if (thread->state == CREATED || thread->state == YIELDED) {
      if (!uthread_resume(executor, thread))
        abort();
      continue;
    } else {
      abort();
    }
  }
}

void uthread_exec_destroy(uthread_executor_t *executor,
                          void (*dealloctor)(void *)) {
  dealloctor(executor);
}

int uthread_resume(uthread_executor_t *exec, uthread_t *thread) {
  uthread_state prev_state = thread->state;
  thread->state = RUNNING;
  exec->running = thread;
  if (prev_state == CREATED || prev_state == YIELDED) {
    printf("resume begin\n");
    if (swapcontext(&exec->ctlr_ctx, &thread->ctx) != 0)
      abort();
    printf("resume end\n");
    return 1;
  }
  return 0;
}

void mark_aborted(uthread_executor_t *executor, uthread_t *thread) {
  thread->state = ABORTED;
  executor->stopped_count++;
}

void uthread_yield() {
  printf("yield begin\n");
  pexec->running->state = YIELDED;
  if (swapcontext(&pexec->running->ctx, &pexec->ctlr_ctx) != 0)
    abort();
  printf("yield end\n");
}

void uthread_exit() {
  pexec->running->state = STOPPED;
  pexec->stopped_count++;
  setcontext(&pexec->ctlr_ctx);
}
