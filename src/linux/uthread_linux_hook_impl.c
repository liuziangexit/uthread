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

#define _GNU_SOURCE
#include <dlfcn.h> //for dlsym
#undef _GNU_SOURCE
#ifdef UTHREAD_DEBUG
#include <inttypes.h> //stackoverflow.com/questions/5795978/string-format-for-intptr-t-and-uintptr-t
#include <stdio.h>
#endif
#include "../../include/uthread.h"
#include "../common/assert_helper.h"
#include "../common/vector.h"

#ifdef __cplusplus
}
#endif
