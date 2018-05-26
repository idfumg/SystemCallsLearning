#include <stdio.h>
#include <sys/utsname.h>

int main()
{
  struct utsname uts;
  const auto uname_ret = uname(&uts);
  if (uname_ret == -1) {
    perror("uname");
  }

  printf("Node name: %s\n", uts.nodename);
  printf("System name: %s\n", uts.sysname);
  printf("Release: %s\n", uts.release);
  printf("Version: %s\n", uts.version);
  printf("Machine: %s\n", uts.machine);

#ifdef _GNU_SOURCE
  printf("Domain name: %s\n", uts.domainname);
#endif

  return 0;
}
