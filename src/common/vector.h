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

struct uthread_vector {
  void *data;
  size_t capacity;
  size_t used;
  size_t item_size;
  void *(*alloc)(size_t);
  void (*dealloc)(void *);
};

bool uthread_vector_init(struct uthread_vector *vec, size_t init_cap,
                         size_t item_size, void *(*alloc)(size_t),
                         void (*dealloc)(void *));

void uthread_vector_destroy(struct uthread_vector *vec);

bool uthread_vector_reserve(struct uthread_vector *vec, size_t new_cap);

bool uthread_vector_add(struct uthread_vector *vec, void *item);

bool uthread_vector_expand(struct uthread_vector *vec, size_t cnt);

void uthread_vector_remove(struct uthread_vector *vec, size_t index);

void uthread_vector_clear(struct uthread_vector *vec);

void *uthread_vector_get(struct uthread_vector *vec, size_t index);

size_t uthread_vector_size(struct uthread_vector *vec);

#endif
