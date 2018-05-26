#include <dlfcn.h>
#include <stdio.h>

void print()
{
  printf("hello from shared lib!\n");
}

int main(int argc, char** argv)
{
  void* handle = dlopen("./libshlib.so", RTLD_LAZY);
  if (handle == NULL) {
    printf("dlopen: %s\n", dlerror());
    return -1;
  }

  void (*funcp)(void);
  *(void**) (&funcp) = dlsym(handle, "plugin_func");

  if (funcp != NULL) {
    (*funcp)();
  }

  dlclose(handle);

  return 0;
}
