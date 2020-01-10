#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int c, char **v) {
  printf("hook test go\n");
  accept(0, 0, 0);
  connect(0, 0, 0);
  send(0, 0, 0, 0);
  recv(0, 0, 0, 0);
  printf("hook test stop\n");
  return 0;
}
