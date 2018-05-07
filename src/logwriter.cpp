#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/poll.h>

#include <linux/limits.h>

#include <string>

using namespace std;

constexpr char filename[] = "log";

static int log(FILE* file, const std::string& data) noexcept
{
    if (const auto ret = fputs((data + "\n").c_str(), file) < 0) {
        return ret;
    }

    return 0;
}

static int create_socket() noexcept
{
    const auto fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }

    unlink("./logsocket");

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, PATH_MAX, "./logsocket");

    if (const auto ret = bind(fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        return ret;
    }

    if (const auto ret = listen(fd, 256) < 0) {
        return ret;
    }

    int on = 1;
    if (const auto ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on)) < 0) {
        return ret;
    }

    on = 1;
    if (const auto ret = ioctl(fd, FIONBIO, (char*) &on) < 0) {
        return ret;
    }

    return fd;
}

static void compress_array(struct pollfd fds[], int& nfds) noexcept
{
    for (auto i = 0; i < nfds; ++i) {
        if (fds[i].fd == -1) {
            for (auto j = i; j < nfds; ++j) {
                fds[j].fd = fds[j+1].fd;
            }
            --nfds;
        }
    }
}

static int polling_socket(const int fd, FILE* file) noexcept
{
    struct pollfd fds[256];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    int nfds = 1;
    bool is_compress_array = false;

    for (;;) {
        const auto rc = poll(fds, nfds, -1);

        if (rc < 0) {
            return rc;
        }

        if (rc == 0) {
            continue;
        }

        const auto current_size = nfds;
        for (auto i = 0; i < current_size; ++i) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].revents != POLLIN) {
                continue;
            }

            if (fds[i].fd == fd) {
                for (;;) {
                    const auto new_fd = accept(fd, NULL, NULL);
                    if (new_fd < 0) {
                        if (errno != EWOULDBLOCK) {
                            return new_fd;
                        }
                        else {
                            break;
                        }
                    }

                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    ++nfds;
                }
            }
            else {
                for (;;) {
                    std::string data;

                    char buffer[BUFSIZ] = {0};
                    const auto rc = recv(fds[i].fd, buffer, sizeof(buffer) -1, 0);

                    if (rc < 0) {
                        if (errno != EWOULDBLOCK) {
                            return rc;
                        }
                        else {
                            break;
                        }
                    }

                    if (rc == 0) {
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        is_compress_array = true;
                        break;
                    }

                    data = buffer;

                    if (not data.empty()) {
                        if (const auto ret = log(file, data) < 0) {
                            return ret;
                        }
                    }
                }
            }
        }

        if (is_compress_array) {
            compress_array(fds, nfds);
            is_compress_array = false;
        }
    }

    for (int i = 0; i < nfds; ++i) {
        if (fds[i].fd >= 0) {
            close(fds[i].fd);
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    const auto file = fopen(filename, "aw");
    if (file == NULL) {
        return -1;
    }

    if (const auto ret = setvbuf(file, NULL, _IONBF, 0) < 0) {
        return ret;
    }

    if (const auto ret = log(file, argv[0] + std::string(" started")) < 0) {
        return ret;
    }

    const auto socket = create_socket();
    if (socket < 0) {
        return socket;
    }

    if (const auto ret = polling_socket(socket, file)) {
        return ret;
    }

    close(socket);

    if (const auto ret = fclose(file) < 0) {
        return ret;
    }

    return 0;
}
