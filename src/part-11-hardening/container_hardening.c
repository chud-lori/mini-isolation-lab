#define _GNU_SOURCE

#ifndef __linux__
#error "container_hardening.c requires Linux seccomp and prctl APIs."
#endif

#include <errno.h>
#include <linux/seccomp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void print_status_field(const char *label, const char *field) {
    FILE *status = fopen("/proc/self/status", "r");
    if (!status) {
        fprintf(stderr, "%s: could not open /proc/self/status: %s\n",
                label,
                strerror(errno));
        return;
    }

    char *line = NULL;
    size_t cap = 0;
    size_t field_len = strlen(field);
    while (getline(&line, &cap, status) > 0) {
        if (strncmp(line, field, field_len) == 0) {
            printf("%s: %s", label, line);
            break;
        }
    }

    free(line);
    fclose(status);
}

static void print_status_snapshot(const char *label) {
    print_status_field(label, "NoNewPrivs:");
    print_status_field(label, "Seccomp:");
    print_status_field(label, "CapEff:");
}

static void child_demo(void) {
    print_status_snapshot("child before hardening");

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
        die("prctl PR_SET_NO_NEW_PRIVS");
    }

    print_status_snapshot("child after no_new_privs");

    printf("child: entering seccomp strict mode\n");
    fflush(stdout);
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT) < 0) {
        fprintf(stderr,
                "prctl PR_SET_SECCOMP strict failed: %s\n"
                "hint: strict mode depends on kernel seccomp support. "
                "The no_new_privs status above still shows the first "
                "hardening step.\n",
                strerror(errno));
        _exit(2);
    }

    const char allowed[] = "child: write(2) still works after strict seccomp\n";
    if (write(STDOUT_FILENO, allowed, sizeof(allowed) - 1) < 0) {
        _exit(3);
    }

    syscall(SYS_getpid);

    const char unexpected[] = "child: unexpected, getpid survived strict seccomp\n";
    write(STDOUT_FILENO, unexpected, sizeof(unexpected) - 1);
    _exit(4);
}

int main(void) {
    printf("host: this demo applies hardening in a child process\n");
    print_status_snapshot("parent");

    pid_t child = fork();
    if (child < 0) {
        die("fork");
    }
    if (child == 0) {
        child_demo();
    }

    int status;
    if (waitpid(child, &status, 0) < 0) {
        die("waitpid");
    }

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("parent: child was terminated by signal %d (%s)\n",
               sig,
               strsignal(sig));
        if (sig == SIGKILL) {
            printf("parent: strict seccomp blocked the forbidden syscall\n");
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("parent: child exited with status %d\n", code);
        if (code == 2) {
            fprintf(stderr,
                    "parent: strict seccomp was not applied, so the demo did "
                    "not prove syscall blocking\n");
        }
        return EXIT_FAILURE;
    }

    fprintf(stderr, "parent: child ended in an unexpected wait state\n");
    return EXIT_FAILURE;
}
