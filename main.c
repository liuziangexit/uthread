#define DEBUG
#include "uthread.c"
#include <assert.h>
#include <stdio.h>

int count = 0;

void co(void *arg) {
  count++;
  // printf("%d go\n", (int)arg);
  uthread_yield();
  count++;
  // printf("%d mid\n", (int)arg);
  uthread_yield();
  count++;
  // printf("%d ok\n", (int)arg);
  uthread_exit();
}

int main(int argc, char **args) {
  printf("go\n------------\n");
  static const size_t CO_COUNT = 80000;
  static const size_t CO_CAP = 80000;
  assert(CO_CAP >= CO_COUNT);
  uthread_executor_t *exec = uthread_exec_create(CO_CAP);
  if (!exec) {
    printf("create exec failed\n");
    abort();
  }

  for (size_t i = 0; i < CO_COUNT; i++) {
    uthread_create(exec, co, i + 1);
  }

  int expect = CO_COUNT * 3;
  uthread_exec_join(exec);
  uthread_exec_destroy(exec);
  printf("count is %d(expect %d)\n", count, expect);
  printf("------------\nok\n");
  return 0;
}
