#define DEBUG
#include "../include/uthread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int quit_flag = 0;
struct uthread_executor_t exec;
enum uthread_error err;
void co(void *arg) {
  int count = 1;
  while (!quit_flag) {
    printf("**********\n线程%zu已被调入，共计调入%"
           "d次\n输入1创建新线程，输入2退出当前线程，输入其他进行调度\n",
           uthread_current_thread()->id, count);
    int choice = getc(stdin);
    if (choice == '1') {
      uthread_id_t id = uthread(&exec, co, 0, &err);
      printf("新线程%zu已创建\n", id);
    } else if (choice == '2') {
      printf("线程%zu已退出\n", uthread_current_thread()->id);
      uthread_exit();
    } else {
      count++;
      uthread_yield();
    }
  }
  printf("线程%zu已退出\n", uthread_current_thread()->id);
  uthread_exit();
}

int main(int argc, char **args) {
  if (uthread_executor(&exec, 0, 0) != OK) {
    printf("create exec failed\n");
    abort();
  }

  uthread(&exec, co, 0, &err);

  err = uthread_join(&exec);
  if (err != OK) {
    printf("uthread_join failed\n");
    abort();
  }
  uthread_executor_destroy(&exec);
  printf("调度器已退出\n");
  return 0;
}
