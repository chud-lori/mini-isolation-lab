#define _GNU_SOURCE

#ifndef __linux__
#error "kvm_api_version.c requires Linux KVM."
#endif

#include <fcntl.h>
#include <linux/kvm.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void) {
    int kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        perror("open /dev/kvm");
        return 1;
    }

    int version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
    if (version < 0) {
        perror("KVM_GET_API_VERSION");
        close(kvm_fd);
        return 1;
    }

    printf("KVM API version: %d\n", version);
    printf("expected by headers: %d\n", KVM_API_VERSION);

    close(kvm_fd);
    return version == KVM_API_VERSION ? 0 : 1;
}
