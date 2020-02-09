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
#include <stdarg.h>
#include <stddef.h> //for size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uthread_executor_t uthread_executor_t;
// uthread_t always have: uthread_executor_t *exec; uthread_state state;
typedef struct uthread_t uthread_t;
typedef enum uthread_state {
  CREATED,
  WAITING_SIGNAL, // waiting for a condition/monitor lock
  WAITING_IO,     // waiting for an IO operation
  YIELDED,        // uthread_yield has just been called
  RUNNING,
  STOPPED
} uthread_state;

typedef enum uthread_clsid {
  EXECUTOR_CLS, // uthread_executor_t
  UTHREAD_CLS   // uthread_t
} uthread_clsid;

typedef enum uthread_error {
  OK,
  BAD_ARGUMENTS_ERR,
  MEMORY_ALLOCATION_ERR,
  SYSTEM_CALL_ERR
} uthread_error;

// type definition
#ifdef __linux__
#include "uthread_linux_def.h"
#else
#error "not implemented"
#endif

uthread_error uthread_executor_init(struct uthread_executor_t *exec,
                                    void *(*alloc)(size_t),
                                    void (*dealloc)(void *));

void uthread_executor_destroy(struct uthread_executor_t *exec);

uthread_error uthread_new(struct uthread_executor_t *exec, void (*func)(void *),
                          void *func_arg, size_t **idx_out);

/*
Run the specific executor.
This function will block until all the uthreads inside the executor has been
exited.
------------------------
exec: executor to run
------------------------
return: OK|SYSTEM_CALL_ERR
*/
enum uthread_error uthread_join(uthread_executor_t *exec);

/*
Yield the current control flow to specified uthread.
------------------------
to: thread to switch
------------------------
return: none
 */
void uthread_switch(uthread_t *to);

/*
Yield the current control flow to unspecified uthread.
------------------------
------------------------
return: none
 */
void uthread_yield();

/*
Causes the calling uthread to exit.
------------------------
------------------------
return: none
 */
void uthread_exit();

/*
retrieve current uthread or executor
------------------------
clsid: EXECUTOR_CLS | UTHREAD_CLS
------------------------
return: pointer to required object if there is, otherwise 0
 */
void *uthread_current(uthread_clsid clsid);

#ifdef __cplusplus
}
#endif

#endif
