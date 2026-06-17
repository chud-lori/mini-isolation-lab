#ifndef KERNEL_USER_H
#define KERNEL_USER_H

#include <kernel/fs.h>
#include <kernel/syscall_entry.h>
#include <kernel/types.h>

#define KUSER_MEMORY_SIZE 512u
#define KUSER_MAX_FD_SLOTS 4u

enum kuser_error {
    KUSER_OK = 0,
    KUSER_EFAULT = 1,
    KUSER_ESYSCALL = 2,
    KUSER_EBADFD = 3,
    KUSER_EINVAL = 4,
};

enum kuser_opcode {
    KUSER_OP_WRITE = 1,
    KUSER_OP_OPEN = 2,
    KUSER_OP_READ = 3,
    KUSER_OP_CLOSE = 4,
    KUSER_OP_STAT = 5,
    KUSER_OP_EXIT = 6,
};

struct kuser_op {
    enum kuser_opcode opcode;
    uint32_t fd_slot;
    uint32_t offset;
    uint32_t len;
    uint32_t aux_offset;
    uint32_t aux_len;
    int exit_status;
};

struct kuser_program {
    const uint8_t *initial_memory;
    size_t initial_memory_len;
    const struct kuser_op *ops;
    size_t op_count;
    const struct kfs_node *fs_nodes;
    size_t fs_node_count;
};

struct kuser_process {
    struct ktask task;
    uint8_t memory[KUSER_MEMORY_SIZE];
    int fd_slots[KUSER_MAX_FD_SLOTS];
    bool exited;
    int exit_status;
    enum kuser_error last_error;
    enum ksyscall_error last_syscall_error;
};

void kuser_init_process(struct kuser_process *process, uint32_t pid,
                        ksyscall_write_byte_fn write_byte, void *write_ctx);
enum kuser_error kuser_run(struct kuser_process *process, const struct kuser_program *program);
const struct kuser_program *kuser_init_program(void);

#endif
