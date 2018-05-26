#include <stdio.h>
#include <pthread.h>

const auto NTHREADS = 10;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void* thread_function(void*)
{
    printf("Thead number: %ld\n", pthread_self());
    pthread_mutex_lock(&mutex1);
    counter++;
    pthread_mutex_unlock(&mutex1);
}

int main()
{
    pthread_t thread_id[NTHREADS];

    for (int i = 0; i < NTHREADS; ++i) {
        pthread_create(&thread_id[i], NULL, thread_function, NULL);
    }

    for (int i = 0; i < NTHREADS; ++i) {
        pthread_join(thread_id[i], NULL);
    }

    printf("Final counter value: %d\n", counter);

    return 0;
}
