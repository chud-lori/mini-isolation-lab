#define _GNU_SOURCE

#ifndef __linux__
#error "kvm_memory_slots.c requires Linux KVM; build and run it on Linux."
#endif

#include <errno.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LOW_MEM_SIZE (1 * 1024 * 1024)
#define HIGH_MEM_SIZE (1 * 1024 * 1024)
#define HIGH_GPA 0x100000000ULL

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

static void *map_host_memory(size_t size, const char *label) {
    void *addr = mmap(NULL,
                      size,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                      -1,
                      0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "mmap %s: %s\n", label, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return addr;
}

static void set_memory_slot(int vm_fd,
                            uint32_t slot,
                            uint64_t guest_phys_addr,
                            void *userspace_addr,
                            uint64_t size,
                            const char *label) {
    struct kvm_userspace_memory_region region;
    memset(&region, 0, sizeof(region));
    region.slot = slot;
    region.guest_phys_addr = guest_phys_addr;
    region.memory_size = size;
    region.userspace_addr = (uint64_t)(uintptr_t)userspace_addr;

    checked_ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, (unsigned long)&region,
                  "KVM_SET_USER_MEMORY_REGION");

    printf("registered slot %u: guest 0x%llx..0x%llx -> host %p (%s)\n",
           slot,
           (unsigned long long)guest_phys_addr,
           (unsigned long long)(guest_phys_addr + size - 1),
           userspace_addr,
           label);
}

static void remove_memory_slot(int vm_fd, uint32_t slot, uint64_t guest_phys_addr) {
    struct kvm_userspace_memory_region region;
    memset(&region, 0, sizeof(region));
    region.slot = slot;
    region.guest_phys_addr = guest_phys_addr;
    region.memory_size = 0;

    checked_ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, (unsigned long)&region,
                  "KVM_SET_USER_MEMORY_REGION remove");
    printf("removed slot %u\n", slot);
}

int main(void) {
    int kvm_fd = open_kvm_device();

    int api_version = checked_ioctl(kvm_fd, KVM_GET_API_VERSION, 0,
                                    "KVM_GET_API_VERSION");
    printf("KVM API version: %d\n", api_version);
    if (api_version != KVM_API_VERSION) {
        fprintf(stderr,
                "KVM API mismatch: kernel returned %d but these headers expect %d.\n",
                api_version,
                KVM_API_VERSION);
        close(kvm_fd);
        return EXIT_FAILURE;
    }

#ifdef KVM_CAP_NR_MEMSLOTS
    int max_slots = checked_ioctl(kvm_fd, KVM_CHECK_EXTENSION,
                                  KVM_CAP_NR_MEMSLOTS,
                                  "KVM_CHECK_EXTENSION(KVM_CAP_NR_MEMSLOTS)");
    printf("KVM reports %d memory slots available\n", max_slots);
    if (max_slots < 2) {
        fprintf(stderr, "need at least two memory slots for this lesson\n");
        close(kvm_fd);
        return EXIT_FAILURE;
    }
#else
    fprintf(stderr,
            "these Linux headers do not define KVM_CAP_NR_MEMSLOTS; "
            "install newer kernel headers for this lesson\n");
    close(kvm_fd);
    return EXIT_FAILURE;
#endif

    int vm_fd = checked_ioctl(kvm_fd, KVM_CREATE_VM, 0, "KVM_CREATE_VM");

    uint8_t *low_mem = map_host_memory(LOW_MEM_SIZE, "low guest RAM");
    uint8_t *high_mem = map_host_memory(HIGH_MEM_SIZE, "high guest RAM");
    memcpy(low_mem, "slot 0: low guest RAM", sizeof("slot 0: low guest RAM"));
    memcpy(high_mem, "slot 1: high guest RAM", sizeof("slot 1: high guest RAM"));

    set_memory_slot(vm_fd, 0, 0, low_mem, LOW_MEM_SIZE, "boot/low memory");
    set_memory_slot(vm_fd, 1, HIGH_GPA, high_mem, HIGH_MEM_SIZE,
                    "device or high memory window");

    printf("host check: slot 0 says \"%s\"\n", low_mem);
    printf("host check: slot 1 says \"%s\"\n", high_mem);
    printf("no vCPU was created; this lesson only shapes the guest physical map\n");

    remove_memory_slot(vm_fd, 1, HIGH_GPA);
    remove_memory_slot(vm_fd, 0, 0);

    munmap(high_mem, HIGH_MEM_SIZE);
    munmap(low_mem, LOW_MEM_SIZE);
    close(vm_fd);
    close(kvm_fd);
    return EXIT_SUCCESS;
}
