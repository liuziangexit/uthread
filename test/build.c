#include "../include/uthread.h"
#include <stdio.h>

int main(int argc, char **argv) {
  enum uthread_error err;
  void *exec = uthread_create(EXECUTOR_CLS, &err, 5, NULL);
  void *uth1 = uthread_create(UTHREAD_CLS, &err, exec, 0, 0);
  void *uth2 = uthread_create(UTHREAD_CLS, &err, exec, 0, 0);

  uthread_destroy(UTHREAD_CLS, uth2, 0);
  uthread_destroy(UTHREAD_CLS, uth1, 0);
  uthread_destroy(EXECUTOR_CLS, exec, 0);
  printf("ok\n");
}
