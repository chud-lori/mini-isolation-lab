#define _GNU_SOURCE

#ifndef __linux__
#error "cgroup_limit.c is intended for Linux cgroup v2."
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    ssize_t len = (ssize_t)strlen(value);
    ssize_t written = write(fd, value, len);
    if (written != len) {
        int saved = written < 0 ? errno : EIO;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}

static int write_filef(const char *path, long value) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%ld\n", value);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        errno = EINVAL;
        return -1;
    }
    return write_file(path, buf);
}

static int path_join(char *buf, size_t buf_size, const char *dir, const char *file) {
    int n = snprintf(buf, buf_size, "%s/%s", dir, file);
    if (n < 0 || (size_t)n >= buf_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static void explain_cgroup_error(const char *action, const char *path) {
    fprintf(stderr, "%s %s: %s\n", action, path, strerror(errno));
    if (errno == EACCES || errno == EPERM || errno == EROFS) {
        fprintf(stderr,
                "hint: run as root on a writable cgroup v2 mount "
                "(for example: sudo ./part-06-cgroups).\n");
    } else if (errno == ENOENT) {
        fprintf(stderr,
                "hint: this demo expects cgroup v2 at /sys/fs/cgroup.\n");
    } else if (errno == EBUSY) {
        fprintf(stderr,
                "hint: the demo cgroup still has member processes; wait for "
                "them to exit or remove them before retrying.\n");
    }
}

static int set_limit(const char *dir, const char *file, const char *value) {
    char path[256];
    if (path_join(path, sizeof(path), dir, file) < 0) {
        perror("build cgroup path");
        return -1;
    }
    if (write_file(path, value) < 0) {
        explain_cgroup_error("write", path);
        return -1;
    }
    return 0;
}

static int cleanup_cgroup(const char *dir) {
    if (rmdir(dir) == 0 || errno == ENOENT) {
        return 0;
    }
    explain_cgroup_error("remove", dir);
    fprintf(stderr,
            "cleanup: remove the stale demo cgroup manually when it is empty: "
            "rmdir %s\n",
            dir);
    return -1;
}

int main(void) {
    const char *dir = "/sys/fs/cgroup/part-06-cgroup-demo";
    char path[256];

    if (mkdir(dir, 0755) < 0 && errno != EEXIST) {
        explain_cgroup_error("mkdir", dir);
        return 1;
    }

    if (set_limit(dir, "pids.max", "8\n") < 0) {
        cleanup_cgroup(dir);
        return 1;
    }

    if (set_limit(dir, "memory.max", "67108864\n") < 0) {
        cleanup_cgroup(dir);
        return 1;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        cleanup_cgroup(dir);
        return 1;
    }

    if (child == 0) {
        if (path_join(path, sizeof(path), dir, "cgroup.procs") < 0) {
            perror("build cgroup path");
            _exit(1);
        }
        if (write_filef(path, (long)getpid()) < 0) {
            explain_cgroup_error("write", path);
            _exit(1);
        }

        printf("child process %ld is limited by %s\n", (long)getpid(), dir);
        printf("pids.max=8 memory.max=64MiB\n");
        fflush(stdout);
        _exit(0);
    }

    int status = 1;
    int child_reaped = 0;
    for (;;) {
        pid_t waited = waitpid(child, &status, 0);
        if (waited == child) {
            child_reaped = 1;
            break;
        }
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    }

    if (cleanup_cgroup(dir) < 0) {
        return 1;
    }
    printf("cleaned up %s\n", dir);

    if (child_reaped && WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (child_reaped && WIFSIGNALED(status)) {
        fprintf(stderr, "child terminated by signal %d\n", WTERMSIG(status));
    }
    return 1;
}
