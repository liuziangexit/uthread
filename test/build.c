#include "../include/uthread.h"
#include <stdio.h>

int main(int argc, char **argv) {
  enum uthread_error err;
  void *obj = uthread_create(EXECUTOR_CLS, &err, 5, NULL);
  // uthread_destroy(EXECUTOR_CLS, obj);
  printf("ok\n");
}
