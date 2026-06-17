#define _GNU_SOURCE

#ifndef __linux__
#error "mini_container.c requires Linux namespaces and cgroups; build it on Linux."
#endif

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

struct child_config {
    char **argv;
    char *rootfs;
    int sync_fd;
};

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void warn_errno(const char *msg) {
    fprintf(stderr, "warning: %s: %s\n", msg, strerror(errno));
}

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

static void try_apply_cgroup_limits(pid_t child_pid) {
    char dir[256];
    char path[512];
    char value[64];

    snprintf(dir, sizeof(dir), "/sys/fs/cgroup/mini-container-%d", child_pid);
    if (mkdir(dir, 0755) < 0 && errno != EEXIST) {
        warn_errno("could not create cgroup; run as root on a cgroup v2 Linux host");
        return;
    }

    snprintf(path, sizeof(path), "%s/pids.max", dir);
    if (write_file(path, "64\n") < 0) {
        warn_errno("could not set pids.max");
    }

    snprintf(path, sizeof(path), "%s/memory.max", dir);
    if (write_file(path, "134217728\n") < 0) {
        warn_errno("could not set memory.max");
    }

    snprintf(path, sizeof(path), "%s/cgroup.procs", dir);
    snprintf(value, sizeof(value), "%d\n", child_pid);
    if (write_file(path, value) < 0) {
        warn_errno("could not move child into cgroup");
    }
}

static void wait_for_parent(int fd) {
    char byte;
    ssize_t n = read(fd, &byte, 1);
    if (n < 0) {
        die("read sync pipe");
    }
    if (n == 0) {
        fprintf(stderr, "parent exited before container started\n");
        exit(EXIT_FAILURE);
    }
    close(fd);
}

static int child_main(void *arg) {
    struct child_config *config = arg;

    wait_for_parent(config->sync_fd);

    if (sethostname("mini-container", strlen("mini-container")) < 0) {
        die("sethostname");
    }

    /*
     * Stop mount changes inside this process from propagating back to the host.
     * This matters before mounting /proc inside the new root filesystem.
     */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
        die("make mounts private");
    }

    if (chroot(config->rootfs) < 0) {
        die("chroot");
    }
    if (chdir("/") < 0) {
        die("chdir /");
    }

    if (mkdir("/proc", 0555) < 0 && errno != EEXIST) {
        die("mkdir /proc");
    }
    if (mount("proc", "/proc", "proc", 0, "") < 0) {
        die("mount /proc");
    }

    printf("inside container: hostname=mini-container, pid=%d\n", getpid());
    execvp(config->argv[0], config->argv);
    die("execvp");
}

static void usage(const char *program) {
    fprintf(stderr, "usage: sudo %s ROOTFS COMMAND [ARG...]\n", program);
    fprintf(stderr, "example: sudo %s ./rootfs /bin/sh\n", program);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        die("malloc stack");
    }

    int sync_pipe[2];
    if (pipe(sync_pipe) < 0) {
        die("pipe");
    }

    struct child_config config = {
        .rootfs = argv[1],
        .argv = &argv[2],
        .sync_fd = sync_pipe[0],
    };

    int flags = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWIPC |
                CLONE_NEWNET | SIGCHLD;
    pid_t child_pid = clone(child_main, stack + STACK_SIZE, flags, &config);
    if (child_pid < 0) {
        die("clone");
    }

    close(sync_pipe[0]);
    try_apply_cgroup_limits(child_pid);

    if (write(sync_pipe[1], "1", 1) != 1) {
        die("write sync pipe");
    }
    close(sync_pipe[1]);

    int status;
    if (waitpid(child_pid, &status, 0) < 0) {
        die("waitpid");
    }

    char cgroup_dir[256];
    snprintf(cgroup_dir, sizeof(cgroup_dir), "/sys/fs/cgroup/mini-container-%d", child_pid);
    if (rmdir(cgroup_dir) < 0 && errno != ENOENT) {
        warn_errno("could not remove cgroup directory");
    }

    free(stack);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return EXIT_FAILURE;
}
