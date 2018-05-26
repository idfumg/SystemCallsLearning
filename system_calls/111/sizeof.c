#include <stdio.h>

int main()
{
    int a = 10;
    const size_t size = sizeof(a++);
    printf("size = %ld, a = %d\n", size, a);

    return 0;
}
