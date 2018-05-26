#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void handler(int sig)
{
    int x;
    printf("Caught signal %d\n", sig);
    printf("Top of handler stack near %10p\n", (void*)&x);
    fflush(NULL);
    _exit(EXIT_FAILURE);
}

static void overflow_stack(int callnum)
{
    char a[100000];
    printf("Call %4d - top of stack near %10p\n", callnum, &a[0]);
    overflow_stack(callnum + 1);
}

int main(int argc, char** argv)
{
    stack_t sigstack;
    sigstack.ss_sp = malloc(SIGSTKSZ);
    if (sigstack.ss_sp == NULL) {
        perror("malloc");
        return 1;
    }
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;

    if (sigaltstack(&sigstack, NULL) == -1) {
        perror("sigaltstack");
        return 2;
    }

    printf("Alternate stack is at %10p - %p\n", sigstack.ss_sp, (char*) sbrk(0) - 1);

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK; // Handler uses alternate stack
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        return 3;
    }

    overflow_stack(1);

    return 0;
}
