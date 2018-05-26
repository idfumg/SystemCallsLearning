#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void* thread_func(void* arg)
{
  printf("Thread started\n");
  for (auto i = 0; ; ++i) {
    printf("Thread loop: %d\n", i);
    sleep(1);
  }

  return NULL;
}

int main()
{
  pthread_t thread;
  if (const auto ret = pthread_create(&thread, NULL, thread_func, NULL) != 0) {
    perror("pthread_create");
    return ret;
  }

  sleep(3);

  if (const auto ret = pthread_cancel(thread) != 0) {
    perror("pthread_cancel");
    return ret;
  }

  void* res;
  if (const auto ret = pthread_join(thread, &res) != 0) {
    perror("pthread_join");
    return ret;
  }

  if (res == PTHREAD_CANCELED) {
    printf("Thread was canceled\n");
  }
  else {
    printf("Thread was not canceled (should not happen!)\n");
  }

  return 0;
}
