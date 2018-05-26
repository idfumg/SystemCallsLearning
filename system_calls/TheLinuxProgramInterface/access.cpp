#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("Usage: %s filename\n", argv[0]);
    return 1;
  }

  const auto isExists = access(argv[1], F_OK);
  if (isExists == -1) {
    printf("File not exists\n");
    return 0;
  }

  const auto isCanBeRead = access(argv[1], R_OK);
  if (isCanBeRead != -1) {
    printf("File can be read\n");
  }

  const auto isCanBeWrite = access(argv[1], W_OK);
  if (isCanBeWrite != -1) {
    printf("File can be write\n");
  }

  const auto isCanBeExecuted = access(argv[1], X_OK);
  if (isCanBeExecuted != -1) {
    printf("File can be executed\n");
  }

  return 0;
}
