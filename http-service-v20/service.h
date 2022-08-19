#ifndef SERVICE_H
#define SERVICE_H

#include "reactor.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// 服务ip
#define SERVICE_IP "0.0.0.0"

// 服务端口
#define SERVICE_PORT 9501

// client最大连接数
#define CLIENT_MAX_NUM 1024

// epoll_wait()的等待时间，-1=阻塞调用，0=非阻塞，>0=等待几秒返回
#define EPOLL_WAIT_TIME 1

typedef struct http_service {
  int app_debug;       //debug模式，1=开启
  int service_running; //服务运行状态，1=运行，0=停止

  int sockfd;               //service的socket句柄
  char ip[INET_ADDRSTRLEN]; //ip，bind()的sockaddr参数
  int port;                 //端口号，bind()的sockaddr参数
  int backlog;              //最大监听数，listen()的backlog参数

  int thread_num; //连接线程的数量
  int call_num;   //http服务被访问的次数，用于线程池轮询调度

  int epfd; //监听service的socket的epoll句柄

  reactor *arr1cell; //连接线程池

  void (*socket_init)(void);
  void (*socket_bind_addr)(void);
  void (*socket_listen)(void);
  void (*service_start)(void);
  void (*on_request)(connection *p1conn);
  void (*service_stop)(void);
} http_service;

extern http_service service;

// 是不是debug模式，1=开启
extern int is_debug();

// socket初始化
extern void socket_init();

// 将地址分配给socket
extern void socket_bind_addr();

// socket开启监听
extern void socket_listen();

// 启动1个线程监听socket，启动thread_num个线程连接socket
extern void service_start();

// 处理业务逻辑
extern void on_request(connection *p1conn);

// 停止服务
extern void service_stop();

#endif