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
  STOPPED,
  ABORTED
} uthread_state;

#ifdef __linux__
#include "uthread_linux_def.h"
#else
#error "not implemented"
#endif

/*
Create an executor with specific capacity for uthread creation.
Executor representd a group of uthread that can be executed by kernel
thread.
------------------------
capacity: initial capacity for uthread
------------------------
return: created executor
 */
uthread_executor_t *uthread_exec_create(size_t capacity);

/*
Create an uthread under specific executor.
------------------------
exec: executor to be holding new uthread
func: uthread's job as void(uthread_t *handle, void* arg)
func_arg: an pointer that will be passing to func
------------------------
return: return 1(true) if the operation success, otherwise return 0(false)
 */
int uthread_create(uthread_executor_t *exec, void (*func)(uthread_t *, void *),
                   void *func_arg);

/*
Execute the specific executor.
This function will return until all the uthreads inside the executor has been
exited or aborted.
------------------------
exec: executor to be execute
------------------------
return: none
*/
void uthread_exec_join(uthread_executor_t *exec);

// destory specific executor
/*
------------------------
------------------------
 */
void uthread_exec_destroy(uthread_executor_t *exec);

/*
switch to another uthread.
This function can only be called by a uthread.
------------------------
handle: current uthread
to: specified thread to switch
------------------------
 */
void uthread_switch(uthread_t *handle, uthread_t *to);

/*
Causes the calling uthread to hang and another uthread that is ready to go will
be resumed.
This function can only be call by a uthread.
------------------------
handle: current uthread
------------------------
return: none
 */
void uthread_yield(uthread_t *handle);

/*
Causes calling uthread to exit.
This function can only be call by a uthread.
------------------------
handle: current uthread
------------------------
return: none
 */
void uthread_exit(uthread_t *handle);

#ifdef __cplusplus
}
#endif

#endif
