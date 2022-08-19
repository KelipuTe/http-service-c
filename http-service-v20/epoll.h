#ifndef EPOLL_H
#define EPOLL_H

// 添加epoll监听事件
extern void epoll_add(int epfd, int op, int sockfd);

// 对于调用过添加的，需要重新设置epoll监听事件
extern void epoll_set(int epfd, int op, int sockfd);

// 删除epoll监听事件
extern void epoll_del(int epfd, int op, int sockfd);

#endif