#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void* func(void* param)
{
    const char* msg = (const char*) param;
    printf("%s", msg);
    for(;;) {
        sleep(1);
    }
    return (void*) 0;
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
}
