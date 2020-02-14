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
#include <stdio.h>

//TODO 支持传格式化字符串和参数进来

void uthread_info(FILE *fp, const char *msg) {
#ifdef UTHREAD_DEBUG
  fprintf(fp, "[info]%s:%d: %s\n", __FILE__, __LINE__, msg);
#endif
}

void uthread_warn(FILE *fp, const char *msg) {
#ifdef UTHREAD_DEBUG
  fprintf(fp, "[warn]%s:%d: %s\n", __FILE__, __LINE__, msg);
#endif
}

void uthread_error(FILE *fp, const char *msg) {
#ifdef UTHREAD_DEBUG
  fprintf(fp, "[error]%s:%d: %s\n", __FILE__, __LINE__, msg);
#endif
}

void uthread_fault(FILE *fp, const char *msg) {
  fprintf(fp, "[fault]%s:%d: %s\n", __FILE__, __LINE__, msg);
}

#endif
