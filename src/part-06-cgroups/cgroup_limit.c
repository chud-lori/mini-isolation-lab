#define _GNU_SOURCE

#ifndef __linux__
#error "cgroup_limit.c is intended for Linux cgroup v2."
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int write_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    ssize_t len = (ssize_t)strlen(value);
    if (write(fd, value, len) != len) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}

int main(void) {
    const char *dir = "/sys/fs/cgroup/part-06-cgroup-demo";
    char path[256];
    char pid[64];

    if (mkdir(dir, 0755) < 0 && errno != EEXIST) {
        perror("mkdir cgroup");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/pids.max", dir);
    if (write_file(path, "8\n") < 0) {
        perror("write pids.max");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/memory.max", dir);
    if (write_file(path, "67108864\n") < 0) {
        perror("write memory.max");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/cgroup.procs", dir);
    snprintf(pid, sizeof(pid), "%d\n", getpid());
    if (write_file(path, pid) < 0) {
        perror("write cgroup.procs");
        return 1;
    }

    printf("this process is now limited by %s\n", dir);
    printf("pids.max=8 memory.max=64MiB\n");
    return 0;
}
