#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/inotify.h>
#include <unistd.h>

constexpr int BUF_LEN = (10 * (sizeof(struct inotify_event) + NAME_MAX + 1));

void display(const struct inotify_event& event)
{
    printf("    wd = %2d\n", event.wd);
    if (event.cookie > 0) {
        printf("cookie = %4d\n", event.cookie);
    }

    printf("mask = ");

    if (event.mask & IN_ACCESS) printf("IN_ACCESS ");
    if (event.mask & IN_ATTRIB) printf("IN_ATTRIB ");
    if (event.mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (event.mask & IN_CLOSE_WRITE) printf("IN_CLOSE_WRITE ");
    if (event.mask & IN_CREATE) printf("IN_CREATE ");
    if (event.mask & IN_DELETE) printf("IN_DELETE ");
    if (event.mask & IN_DELETE_SELF) printf("IN_DELETE_SELF ");
    if (event.mask & IN_IGNORED) printf("IN_IGNORED ");
    if (event.mask & IN_ISDIR) printf("IN_ISDIR ");
    if (event.mask & IN_MODIFY) printf("IN_MODIFY ");
    if (event.mask & IN_MOVE_SELF) printf("IN_MOVE_SELF ");
    if (event.mask & IN_MOVED_FROM) printf("IN_MOVED_FROM ");
    if (event.mask & IN_MOVED_TO) printf("IN_MOVED_TO ");
    if (event.mask & IN_OPEN) printf("IN_OPEN ");
    if (event.mask & IN_Q_OVERFLOW) printf("IN_Q_OVERFLOW ");
    if (event.mask & IN_UNMOUNT) printf("IN_UNMOUNT ");

    printf("\n");

    if (event.len > 0) {
        printf("        name = %s\n", event.name);
    }
}

int main(int argc, char** argv)
{
    const auto inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify");
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        const auto wd = inotify_add_watch(inotify_fd, argv[i], IN_ALL_EVENTS);
        if (wd == -1) {
            perror("inotify_add_watch");
            return 2;
        }

        printf("Watching `%s` using wd %d\n", argv[i], wd);
    }

    char buf[BUF_LEN] = {0};
    for (;;) {
        const auto readn = read(inotify_fd, buf, BUF_LEN);

        if (readn == 0) {
            fprintf(stderr, "read() from inotify() returned 0!");
            return 3;
        }

        if (readn == -1) {
            perror("read");
            return 4;
        }

        printf("Read %ld bytes from inotify fd\n", readn);

        for (auto p = buf; p < buf + readn;) {
            const auto event = (struct inotify_event*) p;
            display(*event);
            p += sizeof(struct inotify_event) + event->len;
        }
    }

    return 0;
}
