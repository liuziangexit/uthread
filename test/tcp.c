#include "../include/uthread.h"
#include <memory.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct {
  int fd;
  struct sockaddr_in addr;
} client_info;

void server_client_thread(uthread_t *current_thread, void *arg) {
  client_info *client = (client_info *)arg;
  printf("server_co handling new client\n");
  /*
  char buffer[256];
  bzero(buffer, 256);
  n = read(newsockfd, buffer, 255);
  if (n < 0)
    error("ERROR reading from socket");
  printf("Here is the message: %s\n", buffer);
  n = write(newsockfd, "I got your message", 18);
  if (n < 0)
    error("ERROR writing to socket");
  return 0;
  */
}

void server_accept_thread(uthread_t *current_thread, void *arg) {
  int server_fd = (int)(ptrdiff_t)arg;
  struct sockaddr_in remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  while (1) {
    int client_fd =
        accept(server_fd, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (client_fd < 0) {
      printf("error: accept failed\n");
      uthread_exit(current_thread);
    }

    client_info *c = malloc(sizeof(client_info));
    c->fd = client_fd;
    memcpy(&c->addr, &remote_addr, remote_addr_len);
    uthread_create(current_thread->exec, server_client_thread, c);
  }
}

void *server(void *a) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    printf("error: socket creation failed\n");
    pthread_exit(0);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(9842);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("error: binding failed\n");
    pthread_exit(0);
  }
  if (listen(server_fd, 5)) {
    printf("error: listen failed\n");
    pthread_exit(0);
  }
  uthread_executor_t *exec = uthread_exec_create(5);
  uthread_create(exec, server_accept_thread, (ptrdiff_t)server_fd);
  uthread_exec_join(exec);
  uthread_exec_destroy(exec);
}

void *client(void *a) { printf("c\n"); }

int main(int argc, char **args) {
  pthread_t servert, clientt;
  pthread_create(&servert, 0, server, 0);
  pthread_create(&clientt, 0, client, 0);

  pthread_join(servert, 0);
  pthread_join(clientt, 0);
}