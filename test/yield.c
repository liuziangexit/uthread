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
  static const size_t CO_COUNT = 300;
  struct uthread_executor_t exec;
  if (uthread_executor(&exec, 0, 0) != OK) {
    printf("create exec failed\n");
    abort();
  }
  size_t uthread_args[CO_COUNT];
  for (size_t i = 0; i < CO_COUNT; i++) {
    uthread_args[i] = i + 1;
    enum uthread_error err;
    uthread(&exec, co, &uthread_args[i], &err);
    if (err != OK) {
      printf("uthread failed\n");
      abort();
    }
  }
  int expect = CO_COUNT * 3;
  enum uthread_error err = uthread_join(&exec);
  if (err != OK) {
    printf("uthread_join failed\n");
    abort();
  }
  uthread_executor_destroy(&exec);
  printf("count is %d(expect %d)\n", count, expect);
  printf("------------\nok\n");
  return 0;
}
