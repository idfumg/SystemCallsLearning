#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  const char filename[] = "/tmp/testXXXXXX";

  setbuf(stdout, NULL);

  const int fd = mkstemp((char*) filename);
  if (fd < 0) {
    perror("mkstemp");
    return fd;
  }

  printf("File offset before fork(): %lld\n", (long long) lseek(fd, 0, SEEK_CUR));

  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    perror("fcntl");
    return flags;
  }

  printf("O_APPEND flag before fork(): %s\n", (flags & O_APPEND) ? "on" : "off");

  switch (fork()) {
  case -1:
    perror("fork");
    return 3;
  case 0:
    if (const auto ret = lseek(fd, 1000, SEEK_SET) < 0) {
      perror("lseek");
      return ret;
    }
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
      perror("fcntl");
      return flags;
    }
    flags |= O_APPEND;
    if (const auto ret = fcntl(fd, F_SETFL, flags) < 0) {
      perror("fcntl");
      return ret;
    }
    _exit(EXIT_SUCCESS);
  default:
    if (const auto ret = wait(NULL) < 0) {
      perror("wait");
      return ret;
    }
    printf("Child exited\n");
    printf("File offset in parent: %lld\n", (long long) lseek(fd, 0, SEEK_CUR));
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
      perror("fcntl");
      return flags;
    }
    printf("O_APPEND flag in parent is: %s\n", (flags | O_APPEND) ? "on" : "off");
  }

  return 0;
}
