#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void* func(void* param)
{
    const char* msg = (const char*) param;
    printf("%s", msg);
    for(;;) {
        sleep(10);
    }
    return (void*) strlen(msg);
}

int main(int argc, char** argv)
{
    pthread_t t1;

    if (const auto ret = pthread_create(&t1, NULL, func, (void*)"Hello world\n") != 0) {
        perror("pthread_create");
        return ret;
    }

    printf("message from main()\n");

    pthread_exit(0);

    void* response;
    if (const auto ret = pthread_join(t1, &response) != 0) {
        perror("pthread_join");
        return ret;
    }
    else {
        printf("thread returned %ld\n", (long) response);
    }

    return 0;
}
