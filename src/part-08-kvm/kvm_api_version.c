#define _GNU_SOURCE

#ifndef __linux__
#error "kvm_api_version.c requires Linux KVM."
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void explain_open_kvm_error(int err) {
    fprintf(stderr, "open /dev/kvm: %s\n", strerror(err));
    if (err == ENOENT || err == ENODEV) {
        fprintf(stderr,
                "hint: /dev/kvm is missing. Enable KVM virtualization in the "
                "host kernel/firmware, load the kvm module, or run this on a "
                "Linux host with hardware virtualization.\n");
    } else if (err == EACCES || err == EPERM) {
        fprintf(stderr,
                "hint: permission denied. Add your user to the kvm group, fix "
                "/dev/kvm permissions, or run with sudo for this demo.\n");
    }
}

int main(void) {
    int kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        explain_open_kvm_error(errno);
        return 1;
    }

    int version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
    if (version < 0) {
        fprintf(stderr, "KVM_GET_API_VERSION: %s\n", strerror(errno));
        close(kvm_fd);
        return 1;
    }

    printf("KVM API version: %d\n", version);
    printf("expected by headers: %d\n", KVM_API_VERSION);

    close(kvm_fd);
    if (version != KVM_API_VERSION) {
        fprintf(stderr,
                "KVM API mismatch: kernel returned %d but these headers expect %d. "
                "Build against headers matching the running kernel or use a "
                "compatible host.\n",
                version,
                KVM_API_VERSION);
        return 1;
    }
    return 0;
}
