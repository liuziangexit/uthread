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
  YIELDED,        // uthread_yield has been called
  RUNNING,
  STOPPED
} uthread_state;

int uthread_exec_create(uthread_executor_t *executor,
                        unsigned long max_thread_count,
                        void *(*alloctor)(unsigned int));
int uthread_create(uthread_executor_t *executor, void (*func)(void *),
                   void *func_arg);
int uthread_exec_join(uthread_executor_t *executor);
void uthread_exec_destroy(uthread_executor_t *executor,
                          void (*dealloctor)(void *));

// internal usage
void uthread_resume(uthread_executor_t *executor, uthread_t *thread);

// call from uthread
void uthread_yield();
void uthread_exit();
uthread_executor_t *uthread_get_exec();

#ifdef __cplusplus
}
#endif

#endif
