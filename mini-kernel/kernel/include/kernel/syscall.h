#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <kernel/fs.h>
#include <kernel/types.h>

#define KTASK_MAX_FILES 8u

enum ksyscall_number {
    KSYSCALL_READ = 0,
    KSYSCALL_WRITE = 1,
    KSYSCALL_OPEN = 2,
    KSYSCALL_CLOSE = 3,
    KSYSCALL_STAT = 4,
    KSYSCALL_GETPID = 5,
    KSYSCALL_EXIT = 6,
};

enum ksyscall_error {
    KSYSCALL_OK = 0,
    KSYSCALL_ENOENT = 2,
    KSYSCALL_EBADF = 9,
    KSYSCALL_EFAULT = 14,
    KSYSCALL_EINVAL = 22,
    KSYSCALL_EMFILE = 24,
    KSYSCALL_ENOSYS = 38,
};

enum ksyscall_fd {
    KSYSCALL_FD_STDIN = 0,
    KSYSCALL_FD_STDOUT = 1,
    KSYSCALL_FD_STDERR = 2,
    KSYSCALL_FD_FIRST_FILE = 3,
};

struct ksyscall_regs {
    uintptr_t nr;
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    uintptr_t arg3;
    uintptr_t arg4;
    uintptr_t arg5;
};

struct ksyscall_result {
    intptr_t value;
    enum ksyscall_error error;
    bool should_exit;
    int exit_status;
};

typedef void (*ksyscall_write_byte_fn)(void *ctx, char ch);

struct ktask_file {
    const struct kfs_node *node;
    size_t offset;
    bool used;
};

struct ktask {
    uint32_t pid;
    uintptr_t user_base;
    size_t user_len;
    ksyscall_write_byte_fn write_byte;
    void *write_ctx;
    struct ktask_file files[KTASK_MAX_FILES];
};

struct ksyscall_result ksyscall_dispatch(
    struct ktask *task,
    const struct ksyscall_regs *regs);

#endif
