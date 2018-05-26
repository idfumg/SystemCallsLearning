#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void* functionC(void*)
{
    pthread_mutex_lock(&mutex1);
    counter++;
    printf("Counter value: %d\n", counter);
    pthread_mutex_unlock(&mutex1);
}


int main()
{
    pthread_t thread1;
    pthread_t thread2;

    const int rc1 = pthread_create(&thread1, NULL, &functionC, NULL);

    if (rc1) {
        printf("Thread 1 creation failed: %d\n", rc1);
    }

    const int rc2 = pthread_create(&thread2, NULL, &functionC, NULL);

    if (rc2) {
        printf("Thread 2 creation failed: %d\n", rc2);
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
