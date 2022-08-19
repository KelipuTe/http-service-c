#ifndef THREAD_H
#define THREAD_H

// 创建线程
extern void create_thread(void *(*func)(void *), void *arg);

// 关闭监听线程
extern void stop_listen_thread(http_service *p1service);

// 关闭连接线程
extern void stop_conn_thread(reactor *p1cell);

// 线程通知，和上面两个关闭函数一起用
extern void notify_thread();

#endif