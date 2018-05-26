#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#define TIMEOUT 5
#define BUFLEN 1024

int main()
{
    fd_set readfs;
    FD_ZERO(&readfs);
    FD_SET(STDIN_FILENO, &readfs);

    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    const int ret = select(STDIN_FILENO + 1, &readfs, NULL, NULL, &tv);
    if (ret == -1) {
        perror("select");
        return 1;
    }
    else if (!ret) {
        printf("%d seconds elapsed.\n", TIMEOUT);
        return 0;
    }

    if (FD_ISSET(STDIN_FILENO, &readfs)) {
        char buf[BUFLEN + 1];
        const int len = read(STDIN_FILENO, buf, BUFLEN);
        if (len == -1) {
            perror("read");
            return 1;
        }
        if (len) {
            buf[len] = '\0';
            printf("read: %s\n", buf);
        }
        return 0;
    }

    fprintf(stderr, "Warning! Unexcpected behaviour!");
    return 1;
}
