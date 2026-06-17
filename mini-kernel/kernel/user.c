#include <kernel/user.h>

#include <kernel/mem.h>

static bool user_range_ok(uint32_t offset, uint32_t len)
{
    return offset <= KUSER_MEMORY_SIZE && len <= KUSER_MEMORY_SIZE - offset;
}

static uintptr_t user_ptr(struct kuser_process *process, uint32_t offset)
{
    return (uintptr_t)&process->memory[offset];
}

void kuser_init_process(struct kuser_process *process, uint32_t pid,
                        ksyscall_write_byte_fn write_byte, void *write_ctx)
{
    if (process == NULL) {
        return;
    }

    kmemset(process, 0, sizeof(*process));
    process->task.pid = pid;
    process->task.user_base = (uintptr_t)process->memory;
    process->task.user_len = sizeof(process->memory);
    process->task.write_byte = write_byte;
    process->task.write_ctx = write_ctx;

    for (size_t i = 0; i < KUSER_MAX_FD_SLOTS; i++) {
        process->fd_slots[i] = -1;
    }
}

static enum kuser_error run_frame(struct kuser_process *process, struct ksyscall_trap_frame *frame)
{
    ksyscall_entry(&process->task, frame);
    process->last_syscall_error = frame->error;

    if (frame->error != KSYSCALL_OK) {
        return KUSER_ESYSCALL;
    }

    if (frame->should_exit) {
        process->exited = true;
        process->exit_status = frame->exit_status;
    }

    return KUSER_OK;
}

enum kuser_error kuser_run(struct kuser_process *process, const struct kuser_program *program)
{
    if (process == NULL || program == NULL || program->ops == NULL) {
        return KUSER_EINVAL;
    }

    if (program->initial_memory_len > sizeof(process->memory)) {
        return KUSER_EFAULT;
    }

    if (!kmemcpy_checked(process->memory, sizeof(process->memory),
                         program->initial_memory, program->initial_memory_len)) {
        return KUSER_EFAULT;
    }

    kfs_mount(program->fs_nodes, program->fs_node_count);
    process->last_error = KUSER_OK;
    process->last_syscall_error = KSYSCALL_OK;

    for (size_t ip = 0; ip < program->op_count && !process->exited; ip++) {
        const struct kuser_op *op = &program->ops[ip];
        struct ksyscall_trap_frame frame = {0};
        enum kuser_error err;

        switch (op->opcode) {
        case KUSER_OP_WRITE:
            if (!user_range_ok(op->offset, op->len)) {
                process->last_error = KUSER_EFAULT;
                return KUSER_EFAULT;
            }
            frame.rax = KSYSCALL_WRITE;
            frame.rdi = KSYSCALL_FD_STDOUT;
            frame.rsi = user_ptr(process, op->offset);
            frame.rdx = op->len;
            break;
        case KUSER_OP_OPEN:
            if (op->fd_slot >= KUSER_MAX_FD_SLOTS || !user_range_ok(op->offset, op->len)) {
                process->last_error = KUSER_EFAULT;
                return KUSER_EFAULT;
            }
            frame.rax = KSYSCALL_OPEN;
            frame.rdi = user_ptr(process, op->offset);
            frame.rsi = op->len;
            break;
        case KUSER_OP_READ:
            if (op->fd_slot >= KUSER_MAX_FD_SLOTS || process->fd_slots[op->fd_slot] < 0 ||
                !user_range_ok(op->offset, op->len)) {
                process->last_error = KUSER_EBADFD;
                return KUSER_EBADFD;
            }
            frame.rax = KSYSCALL_READ;
            frame.rdi = (uintptr_t)process->fd_slots[op->fd_slot];
            frame.rsi = user_ptr(process, op->offset);
            frame.rdx = op->len;
            break;
        case KUSER_OP_CLOSE:
            if (op->fd_slot >= KUSER_MAX_FD_SLOTS || process->fd_slots[op->fd_slot] < 0) {
                process->last_error = KUSER_EBADFD;
                return KUSER_EBADFD;
            }
            frame.rax = KSYSCALL_CLOSE;
            frame.rdi = (uintptr_t)process->fd_slots[op->fd_slot];
            break;
        case KUSER_OP_STAT:
            if (!user_range_ok(op->offset, op->len) || !user_range_ok(op->aux_offset, sizeof(struct kfs_stat))) {
                process->last_error = KUSER_EFAULT;
                return KUSER_EFAULT;
            }
            frame.rax = KSYSCALL_STAT;
            frame.rdi = user_ptr(process, op->offset);
            frame.rsi = op->len;
            frame.rdx = user_ptr(process, op->aux_offset);
            break;
        case KUSER_OP_EXIT:
            frame.rax = KSYSCALL_EXIT;
            frame.rdi = (uintptr_t)op->exit_status;
            break;
        default:
            process->last_error = KUSER_EINVAL;
            return KUSER_EINVAL;
        }

        err = run_frame(process, &frame);
        if (err != KUSER_OK) {
            process->last_error = err;
            return err;
        }

        if (op->opcode == KUSER_OP_OPEN) {
            process->fd_slots[op->fd_slot] = (int)frame.result;
        } else if (op->opcode == KUSER_OP_CLOSE) {
            process->fd_slots[op->fd_slot] = -1;
        }
    }

    return KUSER_OK;
}


static const uint8_t init_memory[] =
    "init: userspace online\n\0"
    "/etc/motd\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static const uint8_t init_motd[] = "motd: hello from the in-memory filesystem\n";

static const struct kfs_node init_fs[] = {
    {.path = "/etc/motd", .data = init_motd, .size = sizeof(init_motd) - 1, .mode = KFS_MODE_FILE},
};

enum {
    INIT_MSG_OFF = 0,
    INIT_MSG_LEN = 23,
    INIT_PATH_OFF = 24,
    INIT_PATH_LEN = 9,
    INIT_BUFFER_OFF = 34,
    INIT_BUFFER_LEN = 40,
};

static const struct kuser_op init_ops[] = {
    {.opcode = KUSER_OP_WRITE, .offset = INIT_MSG_OFF, .len = INIT_MSG_LEN},
    {.opcode = KUSER_OP_OPEN, .fd_slot = 0, .offset = INIT_PATH_OFF, .len = INIT_PATH_LEN},
    {.opcode = KUSER_OP_READ, .fd_slot = 0, .offset = INIT_BUFFER_OFF, .len = INIT_BUFFER_LEN},
    {.opcode = KUSER_OP_WRITE, .offset = INIT_BUFFER_OFF, .len = INIT_BUFFER_LEN},
    {.opcode = KUSER_OP_CLOSE, .fd_slot = 0},
    {.opcode = KUSER_OP_EXIT, .exit_status = 0},
};

static const struct kuser_program init_program = {
    .initial_memory = init_memory,
    .initial_memory_len = sizeof(init_memory),
    .ops = init_ops,
    .op_count = sizeof(init_ops) / sizeof(init_ops[0]),
    .fs_nodes = init_fs,
    .fs_node_count = sizeof(init_fs) / sizeof(init_fs[0]),
};

const struct kuser_program *kuser_init_program(void)
{
    return &init_program;
}
