#include <stdio.h>

// LD_DEBUG=help ./program

void __attribute__ ((constructor)) my_init(void)
{
  printf("shared lib constructor\n");
}

void __attribute__ ((destructor)) my_fini(void)
{
  printf("shared lib destructor\n");
}

int plugin_func()
{
  printf("hello from shared lib!\n");
  return 42;
}

