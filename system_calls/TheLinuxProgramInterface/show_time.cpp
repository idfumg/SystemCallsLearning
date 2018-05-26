#include <time.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

constexpr int BUFSIZE = 100;

int main()
{
  const clock_t clock_start = clock();

  if (setlocale(LC_ALL, "") == NULL) {
    perror("setlocale");
    return 1;
  }

  const time_t time_value = time(NULL);
  printf("time() returned %ld secs\n", (long)time_value);

  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1) {
    perror("gettimeofday");
    return 2;
  }

  printf("gettimeofday() returned %ld secs, %ld microsecs\n", (long)tv.tv_sec, (long)tv.tv_usec);
  printf("ctime() of time() values is: %s", ctime(&time_value));

  struct tm* localtime_value = localtime(&time_value);
  if (localtime_value == NULL) {
    perror("localtime");
    return 3;
  }

  printf("asctime() of local time is: %s", asctime(localtime_value));

  char buff[BUFSIZE] = {0};
  if (strftime(buff, sizeof(buff), "%A, %d %B %Y, %H:%M:%S %Z", localtime_value) == 0) {
    fprintf(stderr, "Error! strftime returned 0");
    return 4;
  }
  printf("strftime() of local time is: %s\n", buff);

  const clock_t clock_stop = clock();
  printf("Process pid is: %ld\n"
         "Real process time is: %lld clocks and %f secs\n",
         (long) getpid(),
         (long long)(clock_stop - clock_start),
         (double)(clock_stop - clock_start) / CLOCKS_PER_SEC);

  return 0;
}
