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

#ifndef __UTHREAD_VECTOR_H__
#define __UTHREAD_VECTOR_H__

#include <stddef.h>

struct uthread_vector {
  void *data;
  size_t capacity;
  size_t used;
};

void uthread_vector_create() {}

void uthread_vector_destroy() {}

void uthread_vector_add(uthread_vector *vec, void *item, size_t item_size) {}

void uthread_vector_insert(uthread_vector *vec, size_t index, void *item,
                           size_t item_size) {}

void uthread_vector_remove(uthread_vector *vec, size_t index, void *item_out,
                           size_t item_size) {}

void uthread_vector_clear(uthread_vector *vec) {}

void uthread_vector_get(uthread_vector *vec, size_t index, void *item_out,
                        size_t item_size) {}

void uthread_vector_reserve(uthread_vector *vec, size_t new_cap) {}

void uthread_vector_shrink(uthread_vector *vec, size_t new_cap) {}

size_t uthread_vector_size(uthread_vector *vec) { return 0; }

#endif
