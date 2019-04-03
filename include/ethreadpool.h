#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct ethread_pool_t ethread_pool_t;

ethread_pool_t *ethread_create(int listenfd, int thread_num,
                               void *(*function)(void *));
void ethread_sig_trigger(ethread_pool_t *pool);
void ethread_destory(ethread_pool_t *pool);


#ifdef __cplusplus
}
#endif

static int set_nonblocking_mode(int fd);


typedef struct ethread_info_t ethread_info_t;
struct ethread_info_t {
  pthread_t ethread;
  int pipefd[2];
};

struct ethread_pool_t {
  ethread_info_t *ethread_info;
  int ethread_count;
  int listenfd;

  int ready_trig;
};

ethread_pool_t *ethread_create(int listenfd, int thread_num,
                               void *(*function)(void *)) {
  ethread_pool_t *pool;
  pool = (ethread_pool_t *)malloc(sizeof(ethread_pool_t));

  pool->ethread_count = 0;
  pool->ready_trig = 0;
  pool->listenfd = listenfd;
  pool->ethread_info =
      (ethread_info_t *)malloc(sizeof(ethread_info_t) * thread_num);

  for (int i = 0; i < thread_num; ++i) {
    int ret = pipe(pool->ethread_info[i].pipefd);
    if (ret < 0) {
      return NULL;
    }

    set_nonblocking_mode(pool->ethread_info[i].pipefd[0]);
  }

  for (int i = 0; i < thread_num; ++i) {
    pool->ethread_count++;
    int ret =
        pthread_create(&(pool->ethread_info[i].ethread), NULL, function, pool);
    if (ret != 0) {
      return NULL;
    }
  }

  return pool;
}

void ethread_destory(ethread_pool_t *pool) {
  for (int i = 0; i < pool->ethread_count; ++i) {
    if (pthread_join(pool->ethread_info[i].ethread, NULL) != 0) {
      return;
    }
  }
  free(pool);
}

void ethread_sig_trigger(ethread_pool_t *pool) {
  char on = 1;
  pool->ready_trig %= pool->ethread_count;
  int rpipe_fd =
      pool->ethread_info[pool->ready_trig].pipefd[1];  //向管道写端写消息；
  pool->ready_trig += 1;
  write(rpipe_fd, &on, sizeof(char));
}

static int set_nonblocking_mode(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}