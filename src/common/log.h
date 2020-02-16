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

#ifndef __UTHREAD_LOG_H__
#define __UTHREAD_LOG_H__
#include <stdarg.h>
#include <stdio.h>

/*
注意，在这里不使用stderr的原因是，stderr是所谓“unbuffered”模式的，
对这种模式的stream进行输出最终会调用到glibc中的buffered_vfprintf函数，
这个函数会在栈上开辟8192字节的空间，而目前我们uthread的栈空间是4k，因此会导致栈溢出

出于这样的考虑，我们使用stdout
 */

#ifdef UTHREAD_DEBUG
#define UTHREAD_DEBUG_PRINT(fmt, ...)                                          \
  do {                                                                         \
    fprintf(stdout, "%s - line %d:  " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#else
#define UTHREAD_DEBUG_PRINT(fmt, ...)                                          \
  do {                                                                         \
  } while (0)
#endif

#endif
