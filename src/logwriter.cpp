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

constexpr char filename[] = "log";

static void log(const std::string& data)
{
    const auto file = fopen(filename, "aw");
    if (file == NULL) {
      perror("fopen");
      exit(2);
    }

    if (setvbuf(file, NULL, _IONBF, 0) < 0) {
      perror("setvbuf");
      exit(3);
    }

    if (fputs((data + "\n").c_str(), file) < 0) {
      perror("fputs");
      exit(4);
    }

    if (fclose(file) < 0) {
      perror("fclose");
      exit(5);
    }
}

int main(int argc, char** argv)
{
  log(argv[0] + std::string(" started"));

  const auto fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(6);
  }

  unlink("./logsocket");

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, PATH_MAX, "./logsocket");

  const auto bind_ret = bind(fd,
                             (struct sockaddr*) &address,
                             sizeof(address));
  if (bind_ret < 0) {
    perror("bind");
    exit(7);
  }

  const auto listen_ret = listen(fd, 256);
  if (listen_ret < 0) {
    perror("listen");
    exit(8);
  }

  int on = 1;
  const auto setsockopt_ret = setsockopt(fd,
                                         SOL_SOCKET,
                                         SO_REUSEADDR,
                                         (char*) &on,
                                         sizeof(on));
  if (setsockopt_ret < 0) {
    perror("setsockopt");
    exit(9);
  }

  on = 1;
  const auto ioctl_ret = ioctl(fd, FIONBIO, (char*) &on);
  if (ioctl_ret < 0) {
    perror("ioctl");
    exit(10);
  }

  struct pollfd fds[256];
  memset(fds, 0, sizeof(fds));
  fds[0].fd = fd;
  fds[0].events = POLLIN;

  int nfds = 1;
  bool compress_array = false;

  for (;;) {
    const auto rc = poll(fds, nfds, -1);

    if (rc < 0) {
      perror("poll");
      exit(11);
    }

    if (rc == 0) {
      continue;
    }

    const int current_size = nfds;
    for (int i = 0; i < current_size; ++i) {
      if (fds[i].revents == 0) {
        continue;
      }

      if (fds[i].revents != POLLIN) {
        continue;
      }

      if (fds[i].fd == fd) {
        for (;;) {
          printf("new connection accepted\n");
          const auto new_fd = accept(fd, NULL, NULL);
          if (new_fd < 0) {
            if (errno != EWOULDBLOCK) {
              perror("accept");
              exit(12);
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
        printf("some data received\n");


        for (;;) {
          printf("read data from socket\n");
          std::string data;

          char buffer[BUFSIZ] = {0};
          const auto rc = recv(fds[i].fd, buffer, sizeof(buffer) -1, 0);

          if (rc < 0) {
            if (errno != EWOULDBLOCK) {
              perror("recv");
              exit(13);
            }
            else {
              break;
            }
          }

          if (rc == 0) {
            close(fds[i].fd);
            fds[i].fd = -1;
            compress_array = true;
            break;
          }

          data = buffer;

          if (not data.empty()) {
            printf("new data received: %s\n", data.c_str());
            log(data);
          }
        }
      }
    }

    if (compress_array) {
      for (int i = 0; i < nfds; ++i) {
        if (fds[i].fd == -1) {
          for (int j = i; j < nfds; ++j) {
            fds[j].fd = fds[j+1].fd;
          }
          --nfds;
        }
      }
      compress_array = false;
    }
  }

  for (int i = 0; i < nfds; ++i) {
    if (fds[i].fd >= 0) {
      close(fds[i].fd);
    }
  }

  close(fd);

  return 0;
}
