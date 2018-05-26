#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  const auto fd = open("/proc/sys/kernel/pid_max", O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  char line[100] = {0};
  const auto n = read(fd, line, 100);
  if (n == -1) {
    perror("read");
    return 2;
  }

  printf("%.*s", (int) n, line);

  close(fd);

  return 0;
}
