#include "../include/uthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

void uthread_func(uthread_t *handle, void *data) {
  accept(0, 0, 0);
  connect(0, 0, 0);
  send(0, 0, 0, 0);
  recv(0, 0, 0, 0);
}

int main(int c, char **v) {
  printf("hook test go\n");

  uthread_executor_t *exec = uthread_exec_create(15);
  if (!uthread_create(exec, uthread_func, 0))
    abort();
  uthread_exec_join(exec);
  uthread_exec_destroy(exec);

  printf("hook test stop\n");
  return 0;
}
