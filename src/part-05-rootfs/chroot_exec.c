#define _GNU_SOURCE

#ifndef __linux__
#error "chroot_exec.c is intended for Linux."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: sudo %s ROOTFS COMMAND [ARG...]\n", argv[0]);
        fprintf(stderr, "example: sudo %s ./rootfs /bin/sh\n", argv[0]);
        return 1;
    }

    if (chroot(argv[1]) < 0) {
        perror("chroot");
        return 1;
    }
    if (chdir("/") < 0) {
        perror("chdir");
        return 1;
    }

    execvp(argv[2], &argv[2]);
    perror("execvp");
    return 1;
}
