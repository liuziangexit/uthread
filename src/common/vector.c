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

#include "vector.h"
#include "assert_helper.h"
#include <string.h>

bool uthread_vector_init(struct uthread_vector *vec, size_t init_cap,
                         size_t item_size, void *(*alloc)(size_t),
                         void (*dealloc)(void *)) {
  if (init_cap <= 1)
    init_cap = 15;
  vec->data = alloc(init_cap * item_size);
  if (!vec->data)
    return false;
  vec->capacity = init_cap;
  vec->used = 0;
  vec->item_size = item_size;
  vec->alloc = alloc;
  vec->dealloc = dealloc;
  return true;
}

void uthread_vector_destroy(struct uthread_vector *vec) {
#ifdef UTHREAD_DEBUG
  memset(vec->data, 0, vec->capacity * vec->item_size);
#endif
  vec->dealloc(vec->data);
  vec->data = 0;
  vec->used = 0;
  vec->capacity = 0;
}

bool uthread_vector_reserve(struct uthread_vector *vec, size_t new_cap) {
  if (vec->capacity >= new_cap) {
    return true;
  }
  void *new_room = vec->alloc(new_cap * vec->item_size);
  if (!new_room) {
    return false;
  }
  memcpy(new_room, vec->data, vec->used * vec->item_size);
#ifdef UTHREAD_DEBUG
  memset(vec->data, 0, vec->capacity * vec->item_size);
#endif
  vec->dealloc(vec->data);
  vec->data = new_room;
  vec->capacity = new_cap;
  return true;
}

bool uthread_vector_add(struct uthread_vector *vec, void *item) {
  if (vec->used == vec->capacity) {
    if (!uthread_vector_reserve(vec, vec->capacity * 1.5))
      return false;
  }
  memcpy(vec->data + (vec->used * vec->item_size), item, vec->item_size);
  vec->used++;
  return true;
}

bool uthread_vector_expand(struct uthread_vector *vec, size_t cnt) {
  if (vec->used + cnt > vec->capacity) {
    size_t new_cap = vec->capacity * 1.5;
    while (vec->used + cnt > new_cap) {
      new_cap *= 1.5;
    }
    if (!uthread_vector_reserve(vec, new_cap))
      return false;
  }
  vec->used += cnt;
  return true;
}

void uthread_vector_remove(struct uthread_vector *vec, size_t index) {
  UTHREAD_CHECK(index < vec->used, "index out of range");
  if (index != vec->used - 1) {
    memcpy(vec->data + (index * vec->item_size),
           vec->data + ((index + 1) * vec->item_size),
           (vec->used - index - 1) * vec->item_size);
  }
  vec->used--;
}

void uthread_vector_clear(struct uthread_vector *vec) { vec->used = 0; }

void *uthread_vector_get(struct uthread_vector *vec, size_t index) {
  UTHREAD_CHECK(index < vec->used, "index out of range");
  return vec->data + (index * vec->item_size);
}

size_t uthread_vector_size(struct uthread_vector *vec) { return vec->used; }
