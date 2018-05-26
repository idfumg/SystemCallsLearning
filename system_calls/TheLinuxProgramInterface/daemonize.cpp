#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int daemonize()
{
    switch (fork()) {
    case -1: return -1;
    case 0: break;
    default: _exit(0);
    }

    if (setsid() < 0) {
        return -1;
    }

    switch (fork()) {
    case -1: return -1;
    case 0: break;
    default: _exit(0);
    }

    chdir(".");
    chroot(".");

    int maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd < 0) {
        maxfd = 8192;
    }

    for (int i = 0; i < maxfd; ++i) {
        close(i);
    }

    if (open("/dev/null", O_RDONLY) < 0) {
        return -1;
    }

    if (open("/dev/null", O_RDWR) < 0) {
        return -1;
    }

    if (open("/dev/null", O_RDWR) < 0) {
        return -1;
    }

    umask(S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH);
}

int main()
{
    if (const auto ret = daemonize() < 0) {
        return ret;
    }

    for (;;) {
        sleep(1);
    }

    return 0;
}
