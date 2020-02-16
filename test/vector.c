#include "../src/common/vector.c"
#include "../src/common/assert_helper.h"
#include <memory.h>
#include <stdio.h>

int main(int argc, char **argv) {
  struct uthread_vector vec;
  UTHREAD_CHECK(uthread_vector_init(&vec, 5, sizeof(int), malloc, free),
                "vector init failed");
  UTHREAD_CHECK(uthread_vector_expand(&vec, 40), "vector expand failed");
  for (int i = 0; i < 40; i++) {
    *(int *)uthread_vector_get(&vec, i) = i;
  }
  for (int i = 40; i < 80; i++) {
    UTHREAD_CHECK(uthread_vector_add(&vec, &i), "vector add failed");
  }
  for (int i = 0; i < 80; i++) {
    UTHREAD_CHECK(*(int *)uthread_vector_get(&vec, i) == i,
                  "vector check failed");
  }
  for (int i = 0; i < 5; i++) {
    uthread_vector_remove(&vec, 0);
  }
  for (int i = 0; i < 75; i++) {
    UTHREAD_CHECK(*(int *)uthread_vector_get(&vec, i) == i + 5,
                  "vector check2 failed");
  }
  uthread_vector_destroy(&vec);
  printf("test passed\n");
}
