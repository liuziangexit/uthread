#define DEBUG
#include "../include/uthread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//#define NO_PRINT

int count = 0;

void co(void *arg) {
  count++;
#ifndef NO_PRINT
  printf("%zu go\n", *(size_t *)arg);
#endif
  uthread_yield();
  count++;
#ifndef NO_PRINT
  printf("%zu mid\n", *(size_t *)arg);
#endif
  uthread_yield();
  count++;
#ifndef NO_PRINT
  printf("%zu ok\n", *(size_t *)arg);
#endif
  uthread_exit();
}

int main(int argc, char **args) {
  printf("go\n------------\n");
  static const size_t CO_COUNT = 5;
  static const size_t CO_CAP = 5;
  assert(CO_CAP >= CO_COUNT);
  struct uthread_executor_t exec;
  if (uthread_executor_init(&exec, 0, 0) != OK) {
    printf("create exec failed\n");
    abort();
  }

  size_t uthread_args[CO_COUNT];
  for (size_t i = 0; i < CO_COUNT; i++) {
    uthread_args[i] = i + 1;
    if (uthread_new(&exec, co, &uthread_args[i], 0) != OK)
      abort();
  }

  int expect = CO_COUNT * 3;
  uthread_join(&exec);
  uthread_executor_destroy(&exec);
  printf("count is %d(expect %d)\n", count, expect);
  printf("------------\nok\n");
  return 0;
}
