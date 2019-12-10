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

#ifndef __UTHREAD_H__
#define __UTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uthread_executor_t uthread_executor_t;
typedef struct uthread_t uthread_t;
typedef enum uthread_state {
  CREATED,
  WAITING_SIGNAL, // waiting for a condition/monitor lock
  WAITING_IO,     // waiting for an IO operation
  YIELDED,        // uthread_yield has just been called
  RUNNING,
  STOPPED,
  ABORTED
} uthread_state;

// create an executor
uthread_executor_t *uthread_exec_create(unsigned int max_thread_count);

// create an uthread under specific executor
int uthread_create(uthread_executor_t *executor, void (*func)(void *),
                   void *func_arg);

// causes uthreads under the executor to be executed
// the call will be blocked until all the uthreads exit or abort
void uthread_exec_join(uthread_executor_t *executor);

// destory specific executor
void uthread_exec_destroy(uthread_executor_t *executor);

// causes the calling uthread to yield execution to another uthread that is
// ready to run on the current executor
void uthread_yield();

// causes the calling uthread to exit
void uthread_exit();

#ifdef __cplusplus
}
#endif

#endif
