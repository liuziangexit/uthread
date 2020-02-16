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
#include "log.h"
#include <stdlib.h> //abort

#define UTHREAD_ABORT(msg)                                                     \
  {                                                                            \
    UTHREAD_DEBUG_PRINT(msg);                                                  \
    abort();                                                                   \
  }

#ifdef UTHREAD_DEBUG
#define UTHREAD_CHECK(v, msg)                                                  \
  if (!(v)) {                                                                  \
    UTHREAD_DEBUG_PRINT(msg);                                                  \
    abort();                                                                   \
  }
#else
#define UTHREAD_CHECK(v, msg)                                                  \
  if (v) {                                                                     \
  }
#endif

#endif
