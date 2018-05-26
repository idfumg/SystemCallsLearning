#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

int main(int argc, char** argv)
{
    const auto signum = std::stoi(argv[2]);
    const auto pid = std::stoi(argv[1]);

    const auto ret = kill(pid, signum);
    if (signum != 0) {
        if (ret == -1) {
            perror("kill");
            return 1;
        }
    }
    else {
        if (ret == 0) {
            printf("Process exists and we can send it a signal\n");
        }
        else {
            if (errno == EPERM) {
                printf("Process exists, but we dont have permission to send it a signal\n");
            }
            else if (errno == ESRCH) {
                printf("Process does not exists");
            }
            else {
                perror("kill");
                return 2;
            }
        }
    }
}
