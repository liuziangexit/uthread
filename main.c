//#define haha
#ifndef haha
#define DEBUG
#include "uthread_sysv.c"
#include <stdio.h>

void co1() {
  printf("1\n");
  uthread_yield();
  printf("3\n");
  uthread_exit();
}

void co2() {
  printf("2\n");
  uthread_yield();
  printf("4\n");
  uthread_exit();
}

int main(int argc, char **args) {
  printf("go\n");
  uthread_executor_t *exec = uthread_exec_create(2);
  if (!exec)
    abort();
  if (!uthread_create(exec, co1, 0))
    abort();
  if (!uthread_create(exec, co2, 0))
    abort();
  uthread_exec_join(exec);
  uthread_exec_destroy(exec);
  printf("ok\n");
  return 0;
}
#endif

#ifdef haha
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

static ucontext_t uctx_main, uctx_func1, uctx_func2;

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

static void func1(void) {
  printf("func1: started\n");
  printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
  if (swapcontext(&uctx_func1, &uctx_func2) == -1)
    handle_error("swapcontext");
  printf("func1: returning\n");
}

static void func2(void) {
  printf("func2: started\n");
  printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
  if (swapcontext(&uctx_func2, &uctx_func1) == -1)
    handle_error("swapcontext");
  printf("func2: returning\n");
}

int main(int argc, char *argv[]) {
  char func1_stack[16384];
  char func2_stack[16384];

  if (getcontext(&uctx_func1) == -1)
    handle_error("getcontext");
  uctx_func1.uc_stack.ss_sp = func1_stack;
  uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
  uctx_func1.uc_link = &uctx_main;
  makecontext(&uctx_func1, func1, 0);

  if (getcontext(&uctx_func2) == -1)
    handle_error("getcontext");
  uctx_func2.uc_stack.ss_sp = func2_stack;
  uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
  /* Successor context is f1(), unless argc > 1 */
  uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
  makecontext(&uctx_func2, func2, 0);

  printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
  if (swapcontext(&uctx_main, &uctx_func2) == -1)
    handle_error("swapcontext");

  printf("main: exiting\n");
  exit(EXIT_SUCCESS);
}
#endif