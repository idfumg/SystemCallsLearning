#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

ssize_t readn(const int fd, void* vbuf, const ssize_t n)
{
    ssize_t nleft = n;
    ssize_t nread = 0;
    char* ptr = (char*)vbuf;

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            }
            else {
                return -1;
            }
        }
        else if (nread == 0) {
            break;
        }

        nleft -= nread;
        ptr += nread;
    }

    return n - nleft;
}

ssize_t writen(const int fd, const void* vptr, const ssize_t n)
{
    ssize_t nleft = n;
    ssize_t nwritten = 0;
    char* ptr = (char*)vptr;

    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;
            }
            else {
                return -1;
            }
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return n - nleft;
}

int main()
{
    const char filename[] = "./filename";
    const char data[] = "My ship is solid!";

    {
        const int mode = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH; // 0664
        const int fd = creat(filename, mode);
        if (fd == -1) {
            perror("creat");
            exit(1);
        }

        if (close(fd) == -1) {
            perror("close");
            exit(1);
        }
    }

    {
        const int fd = open(filename, O_RDWR);
        if (fd == -1) {
            perror("open");
            exit(1);
        }

        if (writen(fd, data, sizeof(data) / sizeof(data[0])) == -1) {
            perror("writen");
            exit(1);
        }

        if (fsync(fd) == -1) {
            if (errno == EINVAL) {
                perror("fdatasync");
            }
            perror("fsync");
            exit(1);
        }

        if (close(fd) == -1) {
            perror("close");
            exit(1);
        }
    }

    {
        const int fd = open(filename, O_RDWR);
        if (fd == -1) {
            perror("open");
            exit(1);
        }

        char buff[1000] = {0};
        if (readn(fd, buff,  sizeof(data) / sizeof(data[0])) == -1) {
            perror("readn");
            exit(1);
        }

        if (close(fd) == -1) {
            perror("close");
            exit(1);
        }

        printf("buff = %s\n", buff);
    }

    return 0;
}
