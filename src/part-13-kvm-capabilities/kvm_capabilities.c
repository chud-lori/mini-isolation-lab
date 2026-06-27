#define _GNU_SOURCE

#ifndef __linux__
#error "kvm_capabilities.c requires Linux KVM; build and run it on Linux."
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

enum cap_kind {
    CAP_BOOLEAN,
    CAP_COUNT,
};

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

static int open_kvm_device(void) {
    int kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        explain_open_kvm_error(errno);
        exit(EXIT_FAILURE);
    }
    return kvm_fd;
}

static int checked_ioctl(int fd,
                         unsigned long request,
                         unsigned long arg,
                         const char *name) {
    int ret = ioctl(fd, request, arg);
    if (ret < 0) {
        fprintf(stderr, "%s: %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return ret;
}

static int check_capability(int kvm_fd,
                            int capability,
                            const char *name,
                            enum cap_kind kind,
                            bool required,
                            const char *why) {
    int value = checked_ioctl(kvm_fd,
                              KVM_CHECK_EXTENSION,
                              (unsigned long)capability,
                              "KVM_CHECK_EXTENSION");

    if (kind == CAP_COUNT) {
        printf("%-24s value=%d", name, value);
    } else {
        printf("%-24s %s", name, value > 0 ? "yes" : "no");
        if (value > 1) {
            printf(" (value=%d)", value);
        }
    }

    printf(" - %s", why);
    if (required && value <= 0) {
        printf(" [required for this path]");
    }
    putchar('\n');

    return value;
}

static void print_missing_header_cap(const char *name, const char *why) {
    printf("%-24s header-missing - %s\n", name, why);
}

int main(void) {
    int kvm_fd = open_kvm_device();

    int api_version = checked_ioctl(kvm_fd,
                                    KVM_GET_API_VERSION,
                                    0,
                                    "KVM_GET_API_VERSION");
    printf("KVM API version: %d\n", api_version);
    printf("expected by headers: %d\n\n", KVM_API_VERSION);
    if (api_version != KVM_API_VERSION) {
        fprintf(stderr,
                "KVM API mismatch: kernel returned %d but these headers expect %d.\n",
                api_version,
                KVM_API_VERSION);
        close(kvm_fd);
        return EXIT_FAILURE;
    }

    bool ready = true;
    printf("microVM/VMM capability checklist:\n");

#ifdef KVM_CAP_USER_MEMORY
    int user_memory = check_capability(
        kvm_fd,
        KVM_CAP_USER_MEMORY,
        "KVM_CAP_USER_MEMORY",
        CAP_BOOLEAN,
        true,
        "userspace-backed guest RAM via KVM_SET_USER_MEMORY_REGION");
    ready = ready && user_memory > 0;
#else
    print_missing_header_cap("KVM_CAP_USER_MEMORY",
                             "old headers cannot name userspace memory support");
    ready = false;
#endif

#ifdef KVM_CAP_NR_MEMSLOTS
    int memslots = check_capability(
        kvm_fd,
        KVM_CAP_NR_MEMSLOTS,
        "KVM_CAP_NR_MEMSLOTS",
        CAP_COUNT,
        true,
        "maximum memory regions the VMM can register");
    ready = ready && memslots > 0;
#else
    print_missing_header_cap("KVM_CAP_NR_MEMSLOTS",
                             "old headers cannot ask for the memory-slot count");
    ready = false;
#endif

#ifdef KVM_CAP_IRQCHIP
    check_capability(kvm_fd,
                     KVM_CAP_IRQCHIP,
                     "KVM_CAP_IRQCHIP",
                     CAP_BOOLEAN,
                     false,
                     "in-kernel interrupt controller support");
#else
    print_missing_header_cap("KVM_CAP_IRQCHIP",
                             "headers do not expose the irqchip capability");
#endif

#ifdef KVM_CAP_IOEVENTFD
    check_capability(kvm_fd,
                     KVM_CAP_IOEVENTFD,
                     "KVM_CAP_IOEVENTFD",
                     CAP_BOOLEAN,
                     false,
                     "eventfd notification path used by virtio-style devices");
#else
    print_missing_header_cap("KVM_CAP_IOEVENTFD",
                             "headers do not expose ioeventfd capability");
#endif

#ifdef KVM_CAP_IRQFD
    check_capability(kvm_fd,
                     KVM_CAP_IRQFD,
                     "KVM_CAP_IRQFD",
                     CAP_BOOLEAN,
                     false,
                     "eventfd-based interrupt injection used by VMM devices");
#else
    print_missing_header_cap("KVM_CAP_IRQFD",
                             "headers do not expose irqfd capability");
#endif

#ifdef KVM_CAP_MP_STATE
    check_capability(kvm_fd,
                     KVM_CAP_MP_STATE,
                     "KVM_CAP_MP_STATE",
                     CAP_BOOLEAN,
                     false,
                     "read/write vCPU multiprocessing state when supported");
#else
    print_missing_header_cap("KVM_CAP_MP_STATE",
                             "headers do not expose vCPU MP state capability");
#endif

    putchar('\n');
    if (ready) {
        printf("baseline result: this host exposes the core KVM pieces needed "
               "to map guest RAM for a tiny VMM.\n");
    } else {
        printf("baseline result: missing a core capability for the tiny VMM "
               "memory path checked by this lesson.\n");
    }

    close(kvm_fd);
    return ready ? EXIT_SUCCESS : EXIT_FAILURE;
}
