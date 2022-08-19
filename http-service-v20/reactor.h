#ifndef REACTOR_H
#define REACTOR_H

#include "connection.h"

// reactor设计模式是event-driven architecture（事件驱动的体系架构）的一种实现方式
// 用于处理多个客户端并发的向服务端请求服务的场景，每种服务在服务端可能由多个方法组成。
// reactor会解析客户端请求的服务，并分发给对应的事件处理器来处理。

// 这里表示连接线程池中的连接线程
typedef struct reactor {
  int cell_index;                       //本连接线程在连接线程池中的index
  int cell_running;                     //本连接线程运行状态，1=运行，0=停止
  int client_num_current;               //当前client连接数
  int client_num_max;                   //最大client连接数
  struct connection *arr1clients;       //client连接区，正在处理的连接
  struct connection *arr1clientsBuffer; //client连接缓冲区，未处理的连接
  int epfd;                             //监听client的socket的epoll句柄
} reactor;

// 连接线程添加client连接到client连接缓冲区
extern void connection_add(reactor *p1cell, connection conn);

// 连接线程的处理函数
extern void *conn_thread(void *arg);

// 通过client的socket句柄反向找到是client连接区中的哪个client连接
extern connection *connection_find(reactor *p1cell, int connfd);

// 连接线程删除client连接区中的client连接
extern void connection_del(reactor *p1cell, connection *p1conn);

#endif