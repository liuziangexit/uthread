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

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

struct uthread_vector {
  void *data;
  size_t capacity;
  size_t used;
  size_t item_size;
  void *(*alloc)(size_t);
  void (*dealloc)(void *);
};

bool uthread_vector_init(uthread_vector *vec, size_t init_cap, size_t item_size,
                         void *(*alloc)(size_t), void (*dealloc)(void *)) {
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

bool uthread_vector_add(uthread_vector *vec, void *item) {
  if (vec->used == vec->capacity) {
    // make more room
    void *new_room =
        vec->alloc(((size_t)(vec->capacity * 1.5)) * vec->item_size);
    if (!new_room)
      return false;
    memcpy(new_room, vec->data, vec->used * vec->item_size);
    vec->dealloc(vec->data);
    vec->data = new_room;
    vec->capacity *= 1.5;
  }
  memcpy(vec->data + (vec->used * vec->item_size), item, vec->item_size);
  vec->used++;
  return true;
}

void uthread_vector_remove(uthread_vector *vec, size_t index) {
#ifdef UTHREAD_DEBUG
#endif
  if (index != vec->used - 1) {
    memcpy(vec->data + (index * vec->item_size),
           vec->data + ((index + 1) * vec->item_size),
           (vec->used - index - 1) * vec->item_size);
  }
  vec->used--;
}

void uthread_vector_clear(uthread_vector *vec) {
#ifdef UTHREAD_DEBUG
#endif
}

void *uthread_vector_get(uthread_vector *vec, size_t index) {
#ifdef UTHREAD_DEBUG
#endif
  return vec->data + (index * vec->item_size);
}

#endif
