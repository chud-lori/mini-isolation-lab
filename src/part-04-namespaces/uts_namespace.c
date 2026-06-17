#define _GNU_SOURCE

#ifndef __linux__
#error "uts_namespace.c uses Linux namespaces; build it on Linux."
#endif

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

static int child_main(void *arg) {
    (void)arg;

    if (sethostname("part-04-namespace", strlen("part-04-namespace")) < 0) {
        perror("sethostname");
        return 1;
    }

    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
        return 1;
    }

    printf("child sees hostname: %s\n", hostname);
    return 0;
}

int main(void) {
    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
        return 1;
    }
    printf("parent sees hostname before clone: %s\n", hostname);

    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return 1;
    }

    pid_t child = clone(child_main, stack + STACK_SIZE, CLONE_NEWUTS | SIGCHLD, NULL);
    if (child < 0) {
        perror("clone");
        free(stack);
        return 1;
    }

    waitpid(child, NULL, 0);

    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
        free(stack);
        return 1;
    }
    printf("parent still sees hostname: %s\n", hostname);

    free(stack);
    return 0;
}
