#include <unistd.h>
#include <stdio.h>

int main()
{
  const uid_t euid = geteuid();

  if (const auto ret = seteuid(getuid()) < 0) {
    perror("seteuid");
    return ret;
  }

  // do unprivileged work with real user id

  if (const auto ret = seteuid(euid) < 0) {
    perror("seteuid");
    return ret;
  }

  // do privileged work with restored set-user-ID if it was exists

  return 0;
}
