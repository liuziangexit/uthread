#include "../include/uthread.h"
#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TODO 支持在uthread中创建uthread，支持自动扩容

// thread clientt wait until thread servert begins listen
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool listen_has_begun = false;

typedef struct {
  int fd;
  struct sockaddr_in addr;
} client_info;

void server_work_thread(void *arg) {
  client_info *client = (client_info *)arg;
  printf("server handling new client\n");
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

void server_accept_thread(void *arg) {
  int server_fd = (int)(ptrdiff_t)arg;
  struct sockaddr_in remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  while (1) {
    printf("server begin accept\n");
    int client_fd =
        accept(server_fd, (struct sockaddr *)&remote_addr, &remote_addr_len);
    if (client_fd < 0) {
      printf("server accept failed\n");
      abort();
    }
    printf("server end accept\n");

    // client_info *c = malloc(sizeof(client_info));
    // c->fd = client_fd;
    // memcpy(&c->addr, &remote_addr, remote_addr_len);
    // uthread_error err;
    // uthread_create(UTHREAD_CLS, &err, uthread_current(EXECUTOR_CLS),
    //                server_work_thread, c);
  }
}

void *server(void *a) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    printf("server socket creation failed\n");
    pthread_exit(0);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(9844);
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("server binding failed\n");
    pthread_exit(0);
  }
  if (listen(server_fd, 5)) {
    printf("server listen failed\n");
    pthread_exit(0);
  }
  pthread_mutex_lock(&mutex);
  listen_has_begun = true;
  printf("server listen_has_begun has been marked as true\n");
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
  uthread_error err;
  uthread_executor_t *exec = uthread_create(EXECUTOR_CLS, &err, 5, 0);
  if (err != OK)
    abort();
  uthread_create(UTHREAD_CLS, &err, exec, server_accept_thread,
                 (ptrdiff_t)server_fd);
  if (err != OK)
    abort();
  uthread_join(exec);
  uthread_destroy(EXECUTOR_CLS, exec, 0);
  pthread_exit(0);
}

void client_thread(void *a) {
  int sockfd;
  struct sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(9844);
  their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    abort();
  }
  printf("client begin wait\n");
  pthread_mutex_lock(&mutex);
  while (!listen_has_begun) {
    pthread_cond_wait(&cond, &mutex);
  }
  pthread_mutex_unlock(&mutex);
  printf("client end wait\n");

  printf("client begin connect\n");
  if (connect(sockfd, (struct sockaddr *)&their_addr,
              sizeof(struct sockaddr)) == -1) {
    printf("client abort connect with errno = %d\n", errno);
    abort();
  }
  printf("client end connect\n");
  close(sockfd);
  // while (1) {
  //   if (send(sockfd, "Hello, world!\n", 14, 0) == -1) {
  //      abort();
  //   }
  //   printf("After the send function \n");

  //   if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
  //     perror("recv");
  //     exit(1);
  //   }

  //   buf[numbytes] = '\0';

  //   printf("Received in pid=%d, text=: %s \n", getpid(), buf);
  //   sleep(1);
  // }
  uthread_exit();
}

void *client(void *a) {
  uthread_error err;
  uthread_executor_t *exec = uthread_create(EXECUTOR_CLS, &err, 5, 0);
  if (err != OK)
    abort();
  uthread_create(UTHREAD_CLS, &err, exec, client_thread, 0);
  if (err != OK)
    abort();
  uthread_join(exec);
  uthread_destroy(EXECUTOR_CLS, exec, 0);
  pthread_exit(0);
}

int main(int argc, char **args) {
  pthread_t servert, clientt;
  pthread_create(&servert, 0, server, 0);
  pthread_create(&clientt, 0, client, 0);

  pthread_join(servert, 0);
  pthread_join(clientt, 0);
}
