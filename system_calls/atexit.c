#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void out()
{
    printf("atexit() succeeded!\n");
}

int my_system(const char* cmd)
{
    const pid_t pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) {
        char* argv[4];
        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = (char*)cmd;
        argv[3] = NULL;
        execv("/bin/sh", argv);
        exit(-1);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }
    else if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}

int main()
{
    if (atexit(out)) {
        fprintf(stderr, "atexit() failed!");
    }

    printf("end of main\n");

    my_system("ls");

    return 0;
}
