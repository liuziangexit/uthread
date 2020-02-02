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

/*
注意，在这里不使用stderr的原因是，stderr是所谓“unbuffered”模式的，
对这种模式的stream进行输出最终会调用到glibc中的buffered_vfprintf函数，
这个函数会在栈上开辟8192字节的空间，而目前我们uthread的栈空间是4k，因此会导致栈溢出

出于这样的考虑，我们使用stdout
 */

#define UTHREAD_CHECK(v, msg)                                                  \
  if (!(v)) {                                                                  \
    fprintf(stdout, "%s\n", (const char *)msg);                                \
    abort();                                                                   \
  }

#define UTHREAD_ABORT(msg)                                                     \
  {                                                                            \
    fprintf(stdout, "%s\n", (const char *)msg);                                \
    abort();                                                                   \
  }

#endif
