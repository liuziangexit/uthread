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
#include "../../include/uthread_linux_def.h"
#include <stdlib.h> // malloc

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
    uthread_executor_t *exec =
        alloc(sizeof(uthread_executor_t) + sizeof(uthread_t) * init_cap);
    // memory allocation failed
    if (exec == 0) {
      *err = MEMORY_ALLOCATION_ERR;
      return 0;
    }
    exec->threads = (uthread_t *)(exec + 1);
    exec->count = 0;
    exec->capacity = init_cap;
    exec->stopped = 0;
    // epoll file descriptor set to -1 represent NULL
    exec->epoll = -1;
    *err = OK;
    return exec;
  } else if (clsid == UTHREAD_CLS) {
    va_list args;
    va_start(args, err);
//savepoint
    va_end(args);
  } else {
    *err = BAD_ARGUMENTS_ERR;
    return 0;
  }
}

enum uthread_error uthread_join(uthread_executor_t *exec);

void uthread_destroy(enum uthread_clsid clsid, void *obj);

void uthread_switch(uthread_t *to);

void uthread_yield();

void uthread_exit();
