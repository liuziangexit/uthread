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
#include <stdbool.h>
#include <stddef.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif

#define UTHREAD_FLAG_SCHEDULABLE 4096 // available to resume
#define UTHREAD_FLAG_WAITING 8192     // waiting for something
enum uthread_state {
  CREATED = UTHREAD_FLAG_SCHEDULABLE + 1, //
  YIELDED = UTHREAD_FLAG_SCHEDULABLE + 2, //
  WAITING_IO = UTHREAD_FLAG_WAITING + 1,  //
  RUNNING = 1,                            //
  STOPPED = 0
};

struct uthread_executor_t;
struct uthread_t;

#ifdef __linux__
#include "../src/common/vector.h"
#include <stdint.h> // SIZE_MAX
#include <ucontext.h>
typedef size_t uthread_id_t;
uthread_id_t UTHREAD_INVALID_ID = SIZE_MAX;
struct uthread_executor_t {
  struct uthread_vector threads;
  uthread_id_t current_thread;
  size_t schedulable_cnt;
  int epoll;
  struct ucontext_t join_ctx;
};
struct uthread_t {
  void (*job)(void *);
  void *job_arg;
  enum uthread_state state;
  struct uthread_executor_t *exec;
  uthread_id_t id;
  struct ucontext_t ctx;
  unsigned char stack[1024 * 4];
};
#else
#error "not implemented"
#endif

enum uthread_error {
  OK,
  BAD_ARGUMENT,
  MEMORY_ALLOCATION_FAILED,
  SYSTEM_CALL_FAILED
};

enum uthread_error uthread_executor(struct uthread_executor_t *exec,
                                    void *(*alloc)(size_t),
                                    void (*dealloc)(void *));
void uthread_executor_destroy(struct uthread_executor_t *exec);
uthread_id_t uthread(struct uthread_executor_t *exec, void (*job)(void *),
                     void *job_arg, enum uthread_error *err);
enum uthread_error uthread_join(struct uthread_executor_t *exec);
void uthread_yield();
void uthread_exit();
struct uthread_executor_t *uthread_current_executor();
struct uthread_t *uthread_current_thread();

#ifdef __cplusplus
}
#endif

#endif
