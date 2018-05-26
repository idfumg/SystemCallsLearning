#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

// g++ -std=c++11 -Wall -pedantic catch_SIGHUP.cpp -o catch_SIGHUP && ./catch_SIGHUP > samegroup.log 2>&1 &
// g++ -std=c++11 -Wall -pedantic catch_SIGHUP.cpp -o catch_SIGHUP && ./catch_SIGHUP x > diffgroup.log 2>&1 &
// echo $$

// no signal SIGHUP for new process group

static void handler(int sig)
{

}

int main(int argc, char** argv)
{
    setbuf(stdout, NULL);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    if (const auto ret = sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    if (const auto pid = fork() < 0) {
        perror("fork");
        return 2;
    }
    else if (pid == 0 && argc > 1) {
        if (const auto ret = setpgid(0, 0) < 0) { // move to new process group
            perror("setpgid");
            return 3;
        }
    }

    printf("PID=%ld; PPID=%ld; PGID=%ld; SID=%ld\n",
           (long) getpid(),
           (long) getppid(),
           (long) getpgrp(),
           (long) getsid(0));

    alarm(60);

    for (;;) {
        pause();
        printf("%ld: caught SIGHUP\n", (long) getpid());
    }

    return 0;
}
