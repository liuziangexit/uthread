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

#ifndef __UTHREAD_ASSERT_HELPER_H__
#define __UTHREAD_ASSERT_HELPER_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UTHREAD_CHECK(v, msg)                                                  \
  if (v) {                                                                     \
    size_t msglen = strlen(msg);                                               \
    if (*(msg + msglen) != '\n' && msglen <= 256 - 1 - 1) {                    \
      char tmp[256];                                                           \
      memcpy(tmp, msg, msglen);                                                \
      tmp[msglen] = '\n';                                                      \
      tmp[msglen + 1] = 0;                                                     \
      fprintf(stderr, tmp);                                                    \
    } else {                                                                   \
      fprintf(stderr, msg);                                                    \
    }                                                                          \
    abort();                                                                   \
  }

#endif
