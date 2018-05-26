#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static int glob = 0;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void cleanup_handler(void* arg)
{
  printf("cleanup: freeing block at %p\n", arg);
  free(arg);

  printf("cleanup: unlocking mutex\n");
  if (const auto ret = pthread_mutex_unlock(&mutex) != 0) {
    perror("pthread_mutex_unlock");
    pthread_exit((void*)ret);
  }
}

static void* thread_func(void* arg)
{
  const auto buff = malloc(0x10000);
  printf("thread: allocated memory at %p\n", buff);

  if (const auto ret = pthread_mutex_lock(&mutex) != 0) {
    perror("pthread_mutex_lock");
    pthread_exit((void*)ret);
  }

  pthread_cleanup_push(cleanup_handler, buff);

  while (glob == 0) {
    if (const auto ret = pthread_cond_wait(&cond, &mutex) != 0) {
      perror("pthread_cond_wait");
      pthread_exit((void*)ret);
    }
  }

  pthread_cleanup_pop(1);

  return NULL;
}

int main()
{
  pthread_t thread;
  if (const auto ret = pthread_create(&thread, NULL, thread_func, NULL) != 0) {
    perror("pthread_create");
    return ret;
  }

  sleep(2);

  // if (const auto ret = pthread_cancel(thread) != 0) {
  //   perror("pthread_cancel");
  //   return ret;
  // }

  glob = 1;
  if (const auto ret = pthread_cond_signal(&cond) != 0) {
    perror("pthread_signal");
    return ret;
  }

  void* res;
  if (const auto ret = pthread_join(thread, &res) != 0) {
    perror("pthread_join");
    return ret;
  }

  if (res == PTHREAD_CANCELED) {
    printf("main: thread was canceled\n");
  }
  else {
    printf("main: thread terminated normally\n");
  }

  return 1;
}
