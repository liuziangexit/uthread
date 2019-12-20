#define DEBUG
#include "../include/uthread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//#define NO_PRINT

int count = 0;

void co(void *handle, void *arg) {
  count++;
#ifndef NO_PRINT
  if (sizeof(size_t) == 4)
    printf("%d go\n", *(size_t *)arg);
  else
    printf("%ld go\n", *(size_t *)arg);
#endif
  uthread_yield(handle);
  count++;
#ifndef NO_PRINT
  if (sizeof(size_t) == 4)
    printf("%d mid\n", *(size_t *)arg);
  else
    printf("%ld mid\n", *(size_t *)arg);
#endif
  uthread_yield(handle);
  count++;
#ifndef NO_PRINT
  if (sizeof(size_t) == 4)
    printf("%d ok\n", *(size_t *)arg);
  else
    printf("%ld ok\n", *(size_t *)arg);
#endif
  uthread_exit(handle);
}

int main(int argc, char **args) {
  printf("go\n------------\n");
  static const size_t CO_COUNT = 5;
  static const size_t CO_CAP = 80000;
  assert(CO_CAP >= CO_COUNT);
  uthread_executor_t *exec = uthread_exec_create(CO_CAP);
  if (!exec) {
    printf("create exec failed\n");
    abort();
  }

  size_t uthread_args[CO_COUNT];
  for (size_t i = 0; i < CO_COUNT; i++) {
    uthread_args[i] = i + 1;
    uthread_create(exec, co, &uthread_args[i]);
  }

  int expect = CO_COUNT * 3;
  uthread_exec_join(exec);
  uthread_exec_destroy(exec);
  printf("count is %d(expect %d)\n", count, expect);
  printf("------------\nok\n");
  return 0;
}
