#define _GNU_SOURCE

#ifndef __linux__
#error "userns_map.c requires Linux user namespaces and /proc uid/gid maps."
#endif

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

struct child_args {
    int ready_fd;
};

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void explain_clone_userns_error(int err) {
    fprintf(stderr, "clone(CLONE_NEWUSER): %s\n", strerror(err));
    if (err == EPERM || err == EACCES) {
        fprintf(stderr,
                "hint: this host may disable unprivileged user namespaces. "
                "Try a Linux host where user namespaces are enabled, or run "
                "the lesson with appropriate privileges.\n");
    } else if (err == EINVAL) {
        fprintf(stderr,
                "hint: this kernel may not support user namespaces or may "
                "have been built without CONFIG_USER_NS.\n");
    }
}

static void write_text_file(const char *path, const char *text) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(text);
    ssize_t written = write(fd, text, len);
    if (written < 0 || (size_t)written != len) {
        fprintf(stderr, "write %s: %s\n", path,
                written < 0 ? strerror(errno) : "short write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        die(path);
    }
}

static void deny_setgroups_if_available(pid_t child) {
    char path[128];
    snprintf(path, sizeof(path), "/proc/%ld/setgroups", (long)child);

    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOENT) {
            printf("parent: %s is not available on this kernel\n", path);
            return;
        }
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    const char text[] = "deny\n";
    ssize_t written = write(fd, text, sizeof(text) - 1);
    if (written < 0 || (size_t)written != sizeof(text) - 1) {
        fprintf(stderr, "write %s: %s\n", path,
                written < 0 ? strerror(errno) : "short write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        die(path);
    }
    printf("parent: wrote deny to %s\n", path);
}

static void write_id_maps(pid_t child, uid_t host_uid, gid_t host_gid) {
    char path[128];
    char map[128];

    snprintf(path, sizeof(path), "/proc/%ld/uid_map", (long)child);
    snprintf(map, sizeof(map), "0 %lu 1\n", (unsigned long)host_uid);
    write_text_file(path, map);
    printf("parent: mapped namespace uid 0 to host uid %lu\n",
           (unsigned long)host_uid);

    deny_setgroups_if_available(child);

    snprintf(path, sizeof(path), "/proc/%ld/gid_map", (long)child);
    snprintf(map, sizeof(map), "0 %lu 1\n", (unsigned long)host_gid);
    write_text_file(path, map);
    printf("parent: mapped namespace gid 0 to host gid %lu\n",
           (unsigned long)host_gid);
}

static void print_map_file(const char *label, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "%s: could not open %s: %s\n",
                label, path, strerror(errno));
        return;
    }

    char line[128];
    if (fgets(line, sizeof(line), file)) {
        printf("%s: %s", label, line);
    }
    fclose(file);
}

static int child_main(void *arg) {
    struct child_args *args = arg;
    char token;

    ssize_t n = read(args->ready_fd, &token, 1);
    if (n < 0) {
        perror("child read sync pipe");
        _exit(EXIT_FAILURE);
    }
    if (n == 0) {
        fprintf(stderr, "child: parent closed sync pipe before mapping ids\n");
        _exit(EXIT_FAILURE);
    }

    close(args->ready_fd);

    printf("child: running in new user namespace\n");
    printf("child: namespace uid=%ld gid=%ld\n", (long)getuid(), (long)getgid());
    print_map_file("child: /proc/self/uid_map", "/proc/self/uid_map");
    print_map_file("child: /proc/self/gid_map", "/proc/self/gid_map");
    return EXIT_SUCCESS;
}

int main(void) {
    uid_t host_uid = getuid();
    gid_t host_gid = getgid();
    printf("parent: host uid=%lu gid=%lu\n",
           (unsigned long)host_uid,
           (unsigned long)host_gid);
    fflush(stdout);

    int sync_pipe[2];
    if (pipe(sync_pipe) < 0) {
        die("pipe");
    }

    struct child_args args = {.ready_fd = sync_pipe[0]};
    void *stack = malloc(STACK_SIZE);
    if (!stack) {
        die("malloc child stack");
    }

    char *stack_top = (char *)stack + STACK_SIZE;
    pid_t child = clone(child_main, stack_top, CLONE_NEWUSER | SIGCHLD, &args);
    if (child < 0) {
        int err = errno;
        close(sync_pipe[0]);
        close(sync_pipe[1]);
        free(stack);
        explain_clone_userns_error(err);
        return EXIT_FAILURE;
    }

    close(sync_pipe[0]);
    write_id_maps(child, host_uid, host_gid);

    if (write(sync_pipe[1], "x", 1) != 1) {
        die("write sync pipe");
    }
    close(sync_pipe[1]);

    int status;
    if (waitpid(child, &status, 0) < 0) {
        die("waitpid");
    }

    free(stack);

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("parent: child exited with status %d\n", code);
        return code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (WIFSIGNALED(status)) {
        printf("parent: child was terminated by signal %d\n", WTERMSIG(status));
    } else {
        fprintf(stderr, "parent: child ended in an unexpected wait state\n");
    }
    return EXIT_FAILURE;
}
