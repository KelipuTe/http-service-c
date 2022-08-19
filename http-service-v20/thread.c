#include "service.h"
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int service_stauts = 0;

void create_thread(void *(*func)(void *), void *arg) {
  pthread_mutex_lock(&mutex);

  pthread_t tid;
  int rtvl = -1;
  rtvl = pthread_create(&tid, NULL, func, arg);
  if (is_debug() == 1) {
    printf("[debug]:create_thread,tid=%p\r\n", tid);
  }
  if (rtvl != 0) {
    printf("[error]:create_thread(),pthread_create(),rtvl!=0");
    printf("[error]:errno=%d,errstr%s\r\n", errno, strerror(errno));
    exit(0);
  }
  pthread_detach(tid);

  pthread_mutex_unlock(&mutex);
}

void stop_listen_thread(http_service *p1service) {
  pthread_mutex_lock(&mutex);

  service_stauts = 0;
  p1service->service_running = 0;
  while (service_stauts == 0) {
    pthread_cond_wait(&cond, &mutex);
  }

  pthread_mutex_unlock(&mutex);
}

void stop_conn_thread(reactor *p1cell) {
  pthread_mutex_lock(&mutex);

  service_stauts = 0;
  p1cell->cell_running = 0;
  while (service_stauts == 0) {
    pthread_cond_wait(&cond, &mutex);
  }

  pthread_mutex_unlock(&mutex);
}

void notify_thread() {
  pthread_mutex_lock(&mutex);

  service_stauts = 1;

  pthread_cond_broadcast(&cond);

  pthread_mutex_unlock(&mutex);
}