#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/inotify.h>

#include <linux/limits.h>

#include <string>

constexpr char msg_filename[] = "msg_data";

std::string read_file(const std::string& filename) noexcept
{
    const auto fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(-1);
    }

    const uint64_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    std::string buff;
    buff.resize(size);

    const auto read_ret = read(fd, &buff[0], buff.size());
    if (read_ret < 0) {
        perror("read");
        exit(-1);
    }

    return buff;
}

int create_socket() noexcept
{
    printf("speaker: create connection to logwriter\n");

    const auto fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, PATH_MAX, "./logsocket");

    if (const auto ret = connect(fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        return ret;
    }

    int on = 1;
    if (const auto ret = ioctl(fd, FIONBIO, (char*) &on) < 0) {
        return ret;
    }

    return fd;
}

int create_inotify() noexcept
{
    const auto inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        return inotify_fd;
    }

    if (const auto wd = inotify_add_watch(inotify_fd, msg_filename, IN_ALL_EVENTS) < 0) {
        return wd;
    }

    int on = 1;
    if (const auto ret = ioctl(inotify_fd, FIONBIO, (char*) &on) < 0) {
        return ret;
    }

    return inotify_fd;
}

int polling_sockets(const int fd, const int inotify_fd) noexcept
{
    struct pollfd fds[256];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = fd;
    fds[0].events = POLLOUT;
    fds[1].fd = inotify_fd;
    fds[1].events = POLLIN;

    int nfds = 2;

    auto msg = read_file(msg_filename);

    for (;;) {
        const auto rc = poll(fds, nfds, -1);

        if (rc < 0) {
            return rc;
        }

        if (rc == 0) {
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].fd == fd) {
                struct iovec vec[1] = {0};
                vec[0].iov_base = (void*) &msg[0];
                vec[0].iov_len = msg.size();

                const auto ret = writev(fds[0].fd, vec, 1);

                if (ret < 0) {
                    return ret;
                }

                if (ret == 0) {
                    return -1;
                }
            }
            else { // inofity (config touched)
                printf("msg file changed. reread...\n");

                msg = read_file(msg_filename);

                for (;;) {
                    char buffer[256] = {0};
                    const auto ret = read(fds[i].fd, buffer, sizeof(buffer));

                    if (ret < 0) {
                        if (errno != EWOULDBLOCK) {
                            return ret;
                        }
                        else {
                            break;
                        }
                    }

                    if (ret == 0) {
                        close(fds[i].fd);
                        fprintf(stderr, "inotify descriptor unexpectedly closed!");
                        return -1;
                    }
                }
            }
        }

        sleep(4);
    }
}

int main(int argc, char** argv)
{
    if (const auto ret = setvbuf(stdout, NULL, _IONBF, 0) < 0) {
        perror("setvbuf");
        return ret;
    }

    printf("speaker started\n");

    if (const auto ret = setvbuf(stdout, NULL, _IONBF, 0) < 0) {
        return ret;
    }

    const auto fd = create_socket();
    if (fd < 0) {
        return fd;
    }

    const auto inotify_fd = create_inotify();
    if (inotify_fd < 0) {
        return inotify_fd;
    }

    if (const auto ret = polling_sockets(fd, inotify_fd) < 0) {
        return ret;
    }

    return 0;
}
