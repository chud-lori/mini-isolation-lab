#define _GNU_SOURCE

#ifndef __linux__
#error "clone_process.c uses Linux clone; build it on Linux."
#endif

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

static int child_main(void *arg) {
    (void)arg;
    printf("child: pid=%d parent=%d\n", getpid(), getppid());
    return 0;
}

int main(void) {
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return 1;
    }

    pid_t child = clone(child_main, stack + STACK_SIZE, SIGCHLD, NULL);
    if (child < 0) {
        perror("clone");
        free(stack);
        return 1;
    }

    printf("parent: pid=%d child=%d\n", getpid(), child);
    waitpid(child, NULL, 0);
    free(stack);
    return 0;
}
