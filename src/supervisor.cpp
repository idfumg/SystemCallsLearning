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

static void print_pids()
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

static void save_pid(const pid_t& pid, const char* process_name)
{
  printf("save_pid: %d\n", pid);
  shared.pids.insert(pid);
  shared.pidnames[pid] = process_name;
  print_pids();
}

static void start_new_process(const char* buff)
{
  printf("starting process `%s`\n", buff);

  const auto pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }
  else if (pid == 0) {
    char path[PATH_MAX] = {0};
    strncpy(path, "./", 2);
    strncpy(path + 2, buff, strlen(buff));

    if (execlp(path, buff, NULL) < 0) {
      perror("execlp");
      exit(-1);
    }
  }
  else {
    save_pid(pid, buff);
  }
}

static void remove_pid(const pid_t& pid)
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
    start_new_process(shared.pidnames[pid].c_str());
  }

  if (pidExists) {
    shared.pidnames.erase(nameit);
  }
}

static void close_pids()
{
  for (const auto& pid : shared.pids) {
    shared.killpids.insert(pid);
    const auto ret = kill(pid, SIGINT);
    if (ret == -1) {
      perror("kill");
    }
  }
}

static bool is_file_exists(const char* filename)
{
  struct stat statbuf = {0};
  return stat(filename, &statbuf) != -1;
}

static void create_new_dir(const char* filename)
{
  const auto mkdir_ret = mkdir(filename, 0777);
  if (mkdir_ret == -1) {
    perror("mkdir");
    exit(2);
  }
}

static void create_config_dir(const char* filename)
{
  if (!is_file_exists(filename)) {
    create_new_dir(filename);
  }
}

static void create_new_file(const char* filename)
{
  const auto fd = open(filename,
                       O_RDWR | O_CREAT | O_TRUNC,
                       S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
  if (fd == -1) {
    perror("open");
    exit(3);
  }

  if (close(fd) == -1) {
    perror("close");
    exit(4);
  }
}

static void create_config_file(const char* filename)
{
  if (!is_file_exists(filename)) {
    create_new_file(filename);
  }
}

static void prepare_config()
{
  printf("prepare config...\n");

  if (setvbuf(stdout, NULL, _IONBF, 0) < 0) {
    perror("setvbuf");
    exit(5);
  }

  create_config_dir(config_dir);
  create_config_file(config_file);
}

static void read_config(const char* filename)
{
  printf("read config...\n");

  const auto file = fopen(filename, "r");
  if (!file) {
    perror("fopen");
    exit(6);
  }

  char buff[BUFSIZ] = {0};
  while (fgets(buff, sizeof(buff), file) != NULL) {
    const auto buff_len = strlen(buff);
    if (buff_len > 0 && buff[buff_len - 1] == '\n') {
      buff[buff_len - 1] = 0;
    }
    if (strlen(buff) != 0) {
      start_new_process(buff);
    }
  }

  if (fclose(file) < 0) {
    perror("fclose");
    exit(7);
  }
}

static int prepare_signalfd(const int flags)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, flags);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    perror("sigprocmask");
    exit(8);
  }

  const auto sfd = signalfd(-1, &mask, 0);
  if (sfd < 0) {
    perror("signal");
    exit(9);
  }

  int on = 1;
  const auto ioctl_ret = ioctl(sfd, FIONBIO, (char*) &on);
  if (ioctl_ret < 0) {
    perror("ioctl");
    exit(10);
  }

  return sfd;
}

struct SignalData {
  int64_t signo;
  int64_t pid;
};

static SignalData read_signalData(const int fd)
{
  struct signalfd_siginfo fdsi;
  const auto nread = read(fd, &fdsi, sizeof(struct signalfd_siginfo));

  if (nread < 0) {
    if (errno != EWOULDBLOCK) {
      perror("read");
      exit(11);
    }
    else {
      return SignalData { -1, -1 };
    }
  }

  printf("signal %d received from %d\n", fdsi.ssi_signo, fdsi.ssi_pid);

  return SignalData { fdsi.ssi_signo, fdsi.ssi_pid };
}

static void wait_loop()
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
      break;
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

        if (fds[i].fd == usrfd) {
          close_pids();
          read_config(config_file);
        }
        else if (fds[i].fd == chldfd) {
          int status = 0;
          waitpid(data.pid, &status, WNOHANG);
          printf("process %d exited with status: %d\n", (int) data.pid, status);
          remove_pid(data.pid);
        }
      }
    }
  }
}

int main(int argc, char** argv)
{
  printf("initial process pid: %d\n", getpid());
  prepare_config();
  wait_loop();

  return 0;
}
