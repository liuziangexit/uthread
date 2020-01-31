/**
 * Copyright (C) 2020 liuziangexit. All rights reserved.
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

#include "../../include/uthread.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h> //for abort

static pthread_key_t __cur_ut_key;
// 0 indicates the key has not been created yet
static int __cur_ut_key_created_flag = 0;

static void uimpl_del_key() {
  if (pthread_key_delete(__cur_ut_key))
    abort();
}
bool uimpl_set_cur_ut(uthread_t *ut) {
  if (!__cur_ut_key_created_flag) {
    if (pthread_key_create(&__cur_ut_key, uimpl_del_key))
      return false;
    __cur_ut_key_created_flag = 1;
  }
  if (pthread_setspecific(__cur_ut_key, (void *)ut))
    return false;
  return true;
}

uthread_t *uimpl_cur_ut() {
  if (!__cur_ut_key_created_flag)
    return 0;
  return (uthread_t *)pthread_getspecific(__cur_ut_key);
}
