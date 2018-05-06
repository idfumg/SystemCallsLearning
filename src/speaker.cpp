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

std::string read_file(const std::string& filename)
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

int main(int argc, char** argv)
{
  if (setvbuf(stdout, NULL, _IONBF, 0) < 0) {
    perror("setvbuf");
    exit(3);
  }


  const auto fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, PATH_MAX, "./logsocket");

  const auto connect_ret = connect(fd,
                                   (struct sockaddr*) &address,
                                   sizeof(address));
  if (connect_ret < 0) {
    perror("connect");
    exit(2);
  }

  int on = 1;
  const auto ioctl_ret = ioctl(fd, FIONBIO, (char*) &on);
  if (ioctl_ret < 0) {
    perror("ioctl");
    exit(3);
  }

  const auto inotify_fd = inotify_init();
  if (inotify_fd < 0) {
    perror("inotify");
    exit(4);
  }

  const auto wd = inotify_add_watch(inotify_fd, msg_filename, IN_ALL_EVENTS);
  if (wd < 0) {
    perror("inotify_add_watch");
    exit(5);
  }

  on = 1;
  const auto ioctl_ret2 = ioctl(inotify_fd, FIONBIO, (char*) &on);
  if (ioctl_ret2 < 0) {
    perror("ioctl#2");
    exit(3);
  }

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
      perror("poll");
      exit(4);
    }

    if (rc == 0) {
      continue;
    }

    for (int i = 0; i < nfds; ++i) {
      if (fds[i].revents == 0) {
        continue;
      }

      if (fds[i].fd == fd) {
        if (fds[0].revents != POLLOUT) {
          fprintf(stderr, "unexpected events on poll!\n");
          exit(5);
        }

        struct iovec vec[1] = {0};
        vec[0].iov_base = (void*) &msg[0];
        vec[0].iov_len = msg.size();

        const auto ret = writev(fds[0].fd, vec, 1);

        if (ret < 0) {
          perror("writev");
          exit(6);
        }

        if (ret == 0) {
          fprintf(stderr, "remote host closed connection. exit...\n");
          exit(7);
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
              perror("read");
              exit(8);
            }
            else {
              break;
            }
          }

          if (ret == 0) {
            close(fds[i].fd);
            fprintf(stderr, "inotify descriptor unexpectedly closed!");
            exit(9);
          }
        }
      }
    }

    sleep(2);
  }

  return 0;
}
