#define _GNU_SOURCE

#ifndef __linux__
#error "mini_firecracker.c requires Linux KVM; build and run it on Linux."
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define GUEST_MEM_SIZE (2 * 1024 * 1024)
#define GUEST_ENTRY 0x1000
#define DEBUG_PORT 0xE9

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static int kvm_ioctl(int fd, unsigned long request, void *arg) {
    int ret = ioctl(fd, request, arg);
    if (ret < 0) {
        die("ioctl");
    }
    return ret;
}

/*
 * Tiny 16-bit x86 guest program:
 *
 *   for each byte in message:
 *       al = byte
 *       out 0xe9, al
 *   hlt
 *
 * Port 0xe9 is a common debug-console convention. Real Firecracker uses virtio
 * devices instead, but the VMM pattern is the same: guest performs I/O, KVM
 * exits to userspace, userspace emulates a device, then resumes the vCPU.
 */
static const uint8_t guest_code[] = {
    0xbe, 0x0e, 0x10,       /* mov si, 0x100e */
    0xac,                   /* lodsb */
    0x84, 0xc0,             /* test al, al */
    0x74, 0x04,             /* jz done */
    0xe6, DEBUG_PORT,       /* out 0xe9, al */
    0xeb, 0xf7,             /* jmp loop */
    0xf4,                   /* done: hlt */
    0x00,                   /* padding */
    'h', 'e', 'l', 'l', 'o', ' ',
    'f', 'r', 'o', 'm', ' ',
    'a', ' ', 't', 'i', 'n', 'y', ' ',
    'm', 'i', 'c', 'r', 'o', 'V', 'M', '\n',
    0x00,
};

static void setup_real_mode(int vcpu_fd) {
    struct kvm_sregs sregs;
    memset(&sregs, 0, sizeof(sregs));
    kvm_ioctl(vcpu_fd, KVM_GET_SREGS, &sregs);

    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    sregs.ds.base = 0;
    sregs.ds.selector = 0;
    sregs.es.base = 0;
    sregs.es.selector = 0;
    sregs.ss.base = 0;
    sregs.ss.selector = 0;
    kvm_ioctl(vcpu_fd, KVM_SET_SREGS, &sregs);

    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rflags = 0x2;
    regs.rip = GUEST_ENTRY;
    regs.rsp = 0x200000;
    kvm_ioctl(vcpu_fd, KVM_SET_REGS, &regs);
}

static void handle_io_exit(struct kvm_run *run) {
    if (run->io.direction != KVM_EXIT_IO_OUT ||
        run->io.size != 1 ||
        run->io.port != DEBUG_PORT) {
        fprintf(stderr,
                "\nunhandled I/O exit: direction=%u size=%u port=0x%x\n",
                run->io.direction,
                run->io.size,
                run->io.port);
        exit(EXIT_FAILURE);
    }

    uint8_t *data = (uint8_t *)run + run->io.data_offset;
    for (uint32_t i = 0; i < run->io.count; i++) {
        putchar(data[i]);
    }
    fflush(stdout);
}

int main(void) {
    int kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        die("open /dev/kvm");
    }

    int api_version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
    if (api_version != KVM_API_VERSION) {
        fprintf(stderr, "unsupported KVM API version: got %d expected %d\n",
                api_version,
                KVM_API_VERSION);
        return EXIT_FAILURE;
    }

    int vm_fd = kvm_ioctl(kvm_fd, KVM_CREATE_VM, NULL);

    uint8_t *guest_mem = mmap(NULL,
                              GUEST_MEM_SIZE,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                              -1,
                              0);
    if (guest_mem == MAP_FAILED) {
        die("mmap guest memory");
    }
    memcpy(guest_mem + GUEST_ENTRY, guest_code, sizeof(guest_code));

    struct kvm_userspace_memory_region region;
    memset(&region, 0, sizeof(region));
    region.slot = 0;
    region.guest_phys_addr = 0;
    region.memory_size = GUEST_MEM_SIZE;
    region.userspace_addr = (uint64_t)guest_mem;
    kvm_ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);

    int vcpu_fd = kvm_ioctl(vm_fd, KVM_CREATE_VCPU, (void *)0);

    int run_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (run_mmap_size < (int)sizeof(struct kvm_run)) {
        fprintf(stderr, "KVM_GET_VCPU_MMAP_SIZE returned too small a size\n");
        return EXIT_FAILURE;
    }

    struct kvm_run *run = mmap(NULL,
                               (size_t)run_mmap_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               vcpu_fd,
                               0);
    if (run == MAP_FAILED) {
        die("mmap kvm_run");
    }

    setup_real_mode(vcpu_fd);

    printf("host VMM: starting vCPU\n");
    for (;;) {
        if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
            if (errno == EINTR) {
                continue;
            }
            die("KVM_RUN");
        }

        switch (run->exit_reason) {
        case KVM_EXIT_IO:
            handle_io_exit(run);
            break;
        case KVM_EXIT_HLT:
            printf("host VMM: guest halted\n");
            goto done;
        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr,
                    "KVM_EXIT_FAIL_ENTRY hardware_entry_failure_reason=0x%llx\n",
                    (unsigned long long)run->fail_entry.hardware_entry_failure_reason);
            goto fail;
        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "KVM_EXIT_INTERNAL_ERROR suberror=%u\n",
                    run->internal.suberror);
            goto fail;
        default:
            fprintf(stderr, "unhandled KVM exit reason: %u\n", run->exit_reason);
            goto fail;
        }
    }

done:
    munmap(run, (size_t)run_mmap_size);
    munmap(guest_mem, GUEST_MEM_SIZE);
    close(vcpu_fd);
    close(vm_fd);
    close(kvm_fd);
    return EXIT_SUCCESS;

fail:
    munmap(run, (size_t)run_mmap_size);
    munmap(guest_mem, GUEST_MEM_SIZE);
    close(vcpu_fd);
    close(vm_fd);
    close(kvm_fd);
    return EXIT_FAILURE;
}
