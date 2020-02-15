#include "../include/uthread.h"
#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RECV_BUF_SIZE 128

struct client_info {
  int fd;
  struct sockaddr_in addr;
};

void server_work_thread(void *arg) {
  struct client_info *client = (struct client_info *)arg;
  printf("server_work_thread go\n");
  unsigned char buf[RECV_BUF_SIZE];
  ssize_t numbytes = recv(client->fd, buf, RECV_BUF_SIZE, 0);
  if (numbytes == -1) {
    printf("server_work_thread recv failed\n");
    free(client);
    abort();
  }
  if (send(client->fd, buf, numbytes, 0) == -1) {
    printf("server_work_thread send failed\n");
    free(client);
    abort();
  }
  if (numbytes > 5) {
    buf[5] = '\0';
  } else {
    buf[numbytes] = '\0';
  }
  printf("server_work_thread recv: %s\n", buf);
  close(client->fd);
  free(client);
  printf("server_work_thread exit\n");
  uthread_exit();
}

void server(void *arg) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    printf("server socket creation failed\n");
    uthread_exit();
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(9844);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("server binding failed\n");
    uthread_exit();
  }
  if (listen(server_fd, 5)) {
    printf("server listen failed\n");
    uthread_exit();
  }
  struct sockaddr_in remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  int i = 3;
  while (i--) {
    printf("server begin accept(%d time left)\n", i);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (client_fd < 0) {
      printf("server accept failed\n");
      abort();
    }
    printf("server end accept(%d time left)\n", i);

    struct client_info *c = malloc(sizeof(struct client_info));
    c->fd = client_fd;
    memcpy(&c->addr, &remote_addr, remote_addr_len);
    enum uthread_error err;
    uthread(uthread_current_executor(), server_work_thread, c, &err);
    if (err != OK) {
      printf("create server work thread failed\n");
    } else {
      printf("create server work thread OK\n");
    }
  }
  close(server_fd);
  printf("server accept thread exit\n");
  uthread_exit();
}

void client(void *arg) {
  int sockfd;
  struct sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(9844);
  their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    abort();
  }

  printf("client begin connect\n");
  if (connect(sockfd, (struct sockaddr *)&their_addr,
              sizeof(struct sockaddr)) == -1) {
    printf("client abort connect with errno = %d\n", errno);
    abort();
  }
  printf("client end connect\n");
  if (send(sockfd, "AAAA", 5, 0) == -1) {
    printf("client send failed\n");
    abort();
  }
  char buf[RECV_BUF_SIZE];
  ssize_t numbytes = recv(sockfd, buf, RECV_BUF_SIZE, 0);
  if (numbytes == -1) {
    printf("client recv failed\n");
    abort();
  }
  buf[numbytes] = '\0';
  if (strcmp("AAAA", buf)) {
    printf("client strcmp failed\n");
    abort();
  } else {
    printf("client strcmp OK\n");
  }
  printf("client exit\n");
  close(sockfd);
  uthread_exit();
}

int main(int argc, char **args) {
  struct uthread_executor_t exec;
  if (uthread_executor(&exec, 0, 0) != OK) {
    printf("create exec failed\n");
    abort();
  }
  uthread(&exec, server, 0, 0);
  uthread(&exec, client, 0, 0);
  uthread_join(&exec);
  uthread_executor_destroy(&exec);
}
