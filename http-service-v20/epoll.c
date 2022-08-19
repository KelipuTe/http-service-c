#include "service.h"

void epoll_add(int epfd, int op, int sockfd) {
  struct epoll_event event;
  event.events = op;
  event.data.fd = sockfd;

  int rtvl = -1;
  rtvl = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
  if (rtvl != 0) {
    printf("epoll_add,epoll_ctl,failed,errno=%d,%s\r\n", errno, strerror(errno));
    exit(0);
  }
}

void epoll_set(int epfd, int op, int sockfd) {
  struct epoll_event event;
  event.events = op;
  event.data.fd = sockfd;

  int rtvl = -1;
  rtvl = epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
  if (rtvl != 0) {
    printf("event_set,epoll_ctl,failed,errno=%d,%s\r\n", errno, strerror(errno));
    exit(0);
  }
}

void epoll_del(int epfd, int op, int sockfd) {
  struct epoll_event event;
  event.events = op;
  event.data.fd = sockfd;

  int rtvl = -1;
  rtvl = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &event);
  if (rtvl != 0) {
    printf("event_del,epoll_ctl,failed,errno=%d,%s\r\n", errno, strerror(errno));
    exit(0);
  }
}
