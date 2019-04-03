#include <stdio.h>
#include <sys/epoll.h>
#include "../include/ethreadpool.h"


int set_nonblocking_mode(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

void addfd(int epollfd, int fd, bool one_shot) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
  if (one_shot) {
    event.events |= EPOLLONESHOT;
  }
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  set_nonblocking_mode(fd);
}

void *work(void *args) {
  ethread_pool_t *pool = (ethread_pool_t *)args;
  int pipe_read_fd;
  for (int i = 0; i < pool->ethread_count; ++i) {
    if (pool->ethread_info[i].ethread == pthread_self()) {
      pipe_read_fd = pool->ethread_info[i].pipefd[0];
      break;
    }
  }
  int epollfd = epoll_create(1);
  addfd(epollfd, pipe_read_fd, false);
  epoll_event events[5];
  printf("start thread id : %d\n", pthread_self());
  while (1) {
    int number = epoll_wait(epollfd, events, 5, -1);
    if (number < 0) {
      printf("epoll failure\n");
      break;
    }
    for (int i = 0; i < number; i++) {
      int curr_sockfd = events[i].data.fd;
      if (events[i].data.fd == pipe_read_fd) {
        char re;
        while (1) {
          if (read(pipe_read_fd, &re, 1) <= 0) break;
        }
        printf("in thread id : %d\n", pthread_self());
      }
    }
  }
}

int main() {
  ethread_pool_t *pool = ethread_create(3, 8, work);
  while (1) {
    char ch = getchar();
    if (ch == 'a') ethread_sig_trigger(pool);
  }

  return 0;
}