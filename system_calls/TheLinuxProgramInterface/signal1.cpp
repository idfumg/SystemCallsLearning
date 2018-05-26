#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void sigHandler(int sig)
{
    printf("Ouch!!!\n"); // unsafe
}

int main()
{
    if (signal(SIGINT, sigHandler) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    for (int i = 0; ; ++i) {
        printf("%d\n", i);
        sleep(3);
    }

    return 0;
}
