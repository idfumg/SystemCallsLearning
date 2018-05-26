#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <setjmp.h>

static volatile sig_atomic_t can_jump = 0; // if we locating in handler before setjmp.

#ifdef USE_SIGSETJMP
    static sigjmp_buf senv;
#else
    static jmp_buf env;
#endif

void handler(int signum)
{
    printf("Received signal %d\n", signum);

    if (!can_jump) {
        printf("env buffer not yet set");
        return;
    }

#ifdef USE_SIGSETJMP
    siglongjmp(senv, 1);
#else
    longjmp(env, 1);
#endif
}

int main(int argc, char** argv)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

#ifdef USE_SIGSETJMP
    printf("Calling sigsetjmp()\n");
    if (sigsetjmp(senv, 1) == 0) {
#else
    printf("Calling setjmp()\n");
    if (setjmp(env) == 0) {
#endif
        can_jump = 1;
    }
    else {
        printf("Jumped\n");
    }

    for (;;) {
        pause();
    }

    return 0;
}
