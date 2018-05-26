#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
  const auto page_size = sysconf(_SC_PAGESIZE);
  printf("virtual page size = %ld\n", page_size);

  const auto max_descriptors = sysconf(_SC_OPEN_MAX);
  printf("max file descriptors size = %ld\n", max_descriptors);

  const auto max_name = sysconf(_PC_NAME_MAX);
  printf("max name size = %ld\n", max_name);

  const auto max_path = sysconf(_PC_PATH_MAX);
  printf("max path size = %ld\n", max_path);

  const auto max_pipe = sysconf(_PC_PIPE_BUF);
  printf("max pipe size = %ld\n", max_pipe);

  return 0;
}
