#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <map>
#include <set>
#include <string>

constexpr char config_dir[] = "config";
constexpr char config_file[] = "config/config.dat";

static struct {
    std::set<pid_t> pids;
    std::set<pid_t> killpids;
    std::map<pid_t, std::string> pidnames;
} shared;

static void print_pids() noexcept
{
    printf("Saved pids: ");
    for (const auto& pid : shared.pids) {
        printf("%d ", (int) pid);
    }
    printf("\n");

    printf("Kill pids: ");
    for (const auto& pid : shared.killpids) {
        printf("%d ", (int) pid);
    }
    printf("\n");

    printf("Pidnames pids: ");
    for (const auto& pid : shared.pidnames) {
        printf("%d->%s ", (int) pid.first, pid.second.c_str());
    }
    printf("\n");
}

static void save_pid(const pid_t& pid, const char* process_name) noexcept
{
    printf("save_pid: %d\n", pid);
    shared.pids.insert(pid);
    shared.pidnames[pid] = process_name;
    print_pids();
}

static int start_new_process(const char* buff) noexcept
{
    printf("starting process `%s`\n", buff);

    const auto pid = fork();
    if (pid < 0) {
        perror("fork");
        return pid;
    }
    else if (pid == 0) {
        char path[PATH_MAX] = {0};
        strncpy(path, "./", 2);
        strncpy(path + 2, buff, strlen(buff));

        if (const auto ret = execlp(path, buff, NULL) < 0) {
            perror("execlp");
            return ret;
        }
    }
    else {
        save_pid(pid, buff);
    }

    return 0;
}

static int remove_pid(const pid_t& pid) noexcept
{
    const auto nameit = shared.pidnames.find(pid);
    const auto pidExists = nameit != shared.pidnames.end();

    shared.pids.erase(pid);

    const auto it = shared.killpids.find(pid);
    if (it != shared.killpids.end()) {
        shared.killpids.erase(pid);
    }
    else if (pidExists) {
        printf("unexpected child termination. restart process\n");
        if (const auto ret = start_new_process(shared.pidnames[pid].c_str()) < 0) {
            return ret;
        }
    }

    if (pidExists) {
        shared.pidnames.erase(nameit);
    }

    return 0;
}

static void close_pids() noexcept
{
    for (const auto& pid : shared.pids) {
        shared.killpids.insert(pid);
        if (const auto ret = kill(pid, SIGINT) < 0) {
            perror("kill");
        }
    }
}

static bool is_file_exists(const char* filename) noexcept
{
    struct stat statbuf = {0};
    return stat(filename, &statbuf) != -1;
}

static int create_new_dir(const char* filename) noexcept
{
    if (const auto ret = mkdir(filename, 0777) < 0) {
        perror("mkdir");
        return ret;
    }

    return 0;
}

static int create_config_dir(const char* filename) noexcept
{
    if (!is_file_exists(filename)) {
        if (const auto ret = create_new_dir(filename) < 0) {
            return ret;
        }
    }
    return 0;
}

static int create_new_file(const char* filename) noexcept
{
    const auto fd = open(filename,
                         O_RDWR | O_CREAT | O_TRUNC,
                         S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
    if (fd < 0) {
        perror("open");
        return fd;
    }

    if (const auto ret = close(fd) < 0) {
        perror("close");
        return ret;
    }

    return 0;
}

static int create_config_file(const char* filename) noexcept
{
    if (!is_file_exists(filename)) {
        if (const auto ret = create_new_file(filename) < 0) {
            return ret;
        }
    }

    return 0;
}

static int prepare_config() noexcept
{
    printf("prepare config...\n");

    if (const auto ret = setvbuf(stdout, NULL, _IONBF, 0) < 0) {
        perror("setvbuf");
        return ret;
    }

    if (const auto ret = create_config_dir(config_dir) < 0) {
        return ret;
    }

    create_config_file(config_file);

    return 0;
}

static int read_config(const char* filename) noexcept
{
    printf("read config...\n");

    const auto file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return -1;
    }

    char buff[BUFSIZ] = {0};
    while (fgets(buff, sizeof(buff), file) != NULL) {
        const auto buff_len = strlen(buff);
        if (buff_len > 0 && buff[buff_len - 1] == '\n') {
            buff[buff_len - 1] = 0;
        }
        if (strlen(buff) != 0) {
            if (const auto ret = start_new_process(buff) < 0) {
                return ret;
            }
        }
    }

    if (const auto ret = fclose(file) < 0) {
        perror("fclose");
        return ret;
    }

    return 0;
}

static int prepare_signalfd(const int flags) noexcept
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, flags);

    if (const auto ret = sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        return ret;
    }

    const auto sfd = signalfd(-1, &mask, 0);
    if (sfd < 0) {
        perror("signal");
        return sfd;
    }

    int on = 1;
    if (const auto ret = ioctl(sfd, FIONBIO, (char*) &on) < 0) {
        perror("ioctl");
        return ret;
    }

    return sfd;
}

struct SignalData {
    int64_t signo;
    int64_t pid;
};

static SignalData read_signalData(const int fd) noexcept
{
    struct signalfd_siginfo fdsi;
    const auto nread = read(fd, &fdsi, sizeof(struct signalfd_siginfo));

    if (nread < 0) {
        if (errno != EWOULDBLOCK) {
            perror("read");
            return SignalData { -2, -1 };
        }
        else {
            return SignalData { -1, -1 };
        }
    }

    printf("signal %d received from %d\n", fdsi.ssi_signo, fdsi.ssi_pid);

    return SignalData { fdsi.ssi_signo, fdsi.ssi_pid };
}

static int wait_loop() noexcept
{
    printf("wait events...\n");

    const auto usrfd = prepare_signalfd(SIGUSR1);
    const auto chldfd = prepare_signalfd(SIGCHLD);

    struct pollfd fds[2];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = usrfd;
    fds[0].events = POLLIN;
    fds[1].fd = chldfd;
    fds[1].events = POLLIN;

    int nfds = 2;

    for (;;) {
        const auto rc = poll(fds, nfds, -1);

        if (rc < 0) {
            perror("poll");
            return rc;
        }

        if (rc == 0) { // timeout
            continue;
        }

        for (auto i = 0; i < nfds; ++i) {
            if (fds[i].revents != POLLIN) {
                continue;
            }

            if (fds[i].fd != usrfd && fds[i].fd != chldfd) {
                continue;
            }

            for (;;) {
                const auto data = read_signalData(fds[i].fd);

                if (data.signo == -1) { // EWOULDBLOCK, nodata
                    break;
                }

                if (data.signo == -2) {
                    return -2;
                }

                if (fds[i].fd == usrfd) {
                    close_pids();
                    read_config(config_file);
                }
                else if (fds[i].fd == chldfd) {
                    int status = 0;
                    waitpid(data.pid, &status, WNOHANG);
                    printf("process %d exited with status: %d\n", (int) data.pid, status);
                    if (const auto ret = remove_pid(data.pid) < 0) {
                        return ret;
                    }
                }
            }
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    printf("initial process pid: %d\n", getpid());

    if (const auto ret = prepare_config() < 0) {
        return ret;
    }

    if (const auto ret = wait_loop() < 0) {
        return ret;
    }

    return 0;
}
