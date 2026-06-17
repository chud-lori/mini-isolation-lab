#include <kernel/syscall.h>

#ifndef KERNEL_HOST_TEST
#include <kernel/log.h>
#endif

static void default_write_byte(void *ctx, char ch)
{
    (void)ctx;
#ifdef KERNEL_HOST_TEST
    (void)ch;
#else
    klog_putc(ch);
#endif
}

static struct ksyscall_result ksyscall_result(
    intptr_t value,
    enum ksyscall_error error)
{
    return (struct ksyscall_result){
        .value = value,
        .error = error,
        .should_exit = false,
        .exit_status = 0,
    };
}

static bool task_contains_user_range(const struct ktask *task, uintptr_t ptr, size_t len)
{
    uintptr_t ptr_end;
    uintptr_t user_end;

    if (task == NULL) {
        return false;
    }

    if (len == 0) {
        return true;
    }

    if (__builtin_add_overflow(ptr, (uintptr_t)len, &ptr_end)) {
        return false;
    }

    if (__builtin_add_overflow(task->user_base, (uintptr_t)task->user_len, &user_end)) {
        return false;
    }

    return ptr >= task->user_base && ptr_end <= user_end;
}

static struct ksyscall_result sys_write(
    struct ktask *task,
    uintptr_t fd,
    uintptr_t ptr,
    size_t len)
{
    const char *bytes;

    if (fd != KSYSCALL_FD_STDOUT && fd != KSYSCALL_FD_STDERR) {
        return ksyscall_result(-1, KSYSCALL_EBADF);
    }

    if (!task_contains_user_range(task, ptr, len)) {
        return ksyscall_result(-1, KSYSCALL_EFAULT);
    }

    ksyscall_write_byte_fn write_byte = task->write_byte != NULL ? task->write_byte : default_write_byte;

    bytes = (const char *)ptr;
    for (size_t i = 0; i < len; i++) {
        write_byte(task->write_ctx, bytes[i]);
    }

    return ksyscall_result((intptr_t)len, KSYSCALL_OK);
}

static struct ksyscall_result sys_open(struct ktask *task, uintptr_t path_ptr, size_t path_len)
{
    const struct kfs_node *node;

    if (!task_contains_user_range(task, path_ptr, path_len)) {
        return ksyscall_result(-1, KSYSCALL_EFAULT);
    }

    node = kfs_lookup((const char *)path_ptr, path_len);
    if (node == NULL) {
        return ksyscall_result(-1, KSYSCALL_ENOENT);
    }

    for (size_t fd = KSYSCALL_FD_FIRST_FILE; fd < KTASK_MAX_FILES; fd++) {
        if (!task->files[fd].used) {
            task->files[fd].node = node;
            task->files[fd].offset = 0;
            task->files[fd].used = true;
            return ksyscall_result((intptr_t)fd, KSYSCALL_OK);
        }
    }

    return ksyscall_result(-1, KSYSCALL_EMFILE);
}

static struct ksyscall_result sys_close(struct ktask *task, uintptr_t fd)
{
    if (task == NULL || fd < KSYSCALL_FD_FIRST_FILE || fd >= KTASK_MAX_FILES || !task->files[fd].used) {
        return ksyscall_result(-1, KSYSCALL_EBADF);
    }

    task->files[fd] = (struct ktask_file){0};
    return ksyscall_result(0, KSYSCALL_OK);
}

static struct ksyscall_result sys_read(
    struct ktask *task,
    uintptr_t fd,
    uintptr_t dst,
    size_t len)
{
    struct ktask_file *file;
    size_t bytes_read;

    if (task == NULL || fd < KSYSCALL_FD_FIRST_FILE || fd >= KTASK_MAX_FILES || !task->files[fd].used) {
        return ksyscall_result(-1, KSYSCALL_EBADF);
    }

    if (!task_contains_user_range(task, dst, len)) {
        return ksyscall_result(-1, KSYSCALL_EFAULT);
    }

    file = &task->files[fd];
    bytes_read = kfs_read_node(file->node, file->offset, (void *)dst, len);
    file->offset += bytes_read;

    return ksyscall_result((intptr_t)bytes_read, KSYSCALL_OK);
}

static struct ksyscall_result sys_stat(
    struct ktask *task,
    uintptr_t path_ptr,
    size_t path_len,
    uintptr_t stat_ptr)
{
    const struct kfs_node *node;
    struct kfs_stat *out;

    if (!task_contains_user_range(task, path_ptr, path_len) ||
        !task_contains_user_range(task, stat_ptr, sizeof(struct kfs_stat))) {
        return ksyscall_result(-1, KSYSCALL_EFAULT);
    }

    node = kfs_lookup((const char *)path_ptr, path_len);
    if (node == NULL) {
        return ksyscall_result(-1, KSYSCALL_ENOENT);
    }

    out = (struct kfs_stat *)stat_ptr;
    if (kfs_stat_node(node, out) != KFS_OK) {
        return ksyscall_result(-1, KSYSCALL_EINVAL);
    }

    return ksyscall_result(0, KSYSCALL_OK);
}

static struct ksyscall_result sys_getpid(const struct ktask *task)
{
    if (task == NULL || task->pid == 0) {
        return ksyscall_result(-1, KSYSCALL_EINVAL);
    }

    return ksyscall_result((intptr_t)task->pid, KSYSCALL_OK);
}

static struct ksyscall_result sys_exit(int status)
{
    return (struct ksyscall_result){
        .value = 0,
        .error = KSYSCALL_OK,
        .should_exit = true,
        .exit_status = status,
    };
}

struct ksyscall_result ksyscall_dispatch(
    struct ktask *task,
    const struct ksyscall_regs *regs)
{
    if (regs == NULL) {
        return ksyscall_result(-1, KSYSCALL_EINVAL);
    }

    switch ((enum ksyscall_number)regs->nr) {
    case KSYSCALL_READ:
        return sys_read(task, regs->arg0, regs->arg1, (size_t)regs->arg2);
    case KSYSCALL_WRITE:
        return sys_write(task, regs->arg0, regs->arg1, (size_t)regs->arg2);
    case KSYSCALL_OPEN:
        return sys_open(task, regs->arg0, (size_t)regs->arg1);
    case KSYSCALL_CLOSE:
        return sys_close(task, regs->arg0);
    case KSYSCALL_STAT:
        return sys_stat(task, regs->arg0, (size_t)regs->arg1, regs->arg2);
    case KSYSCALL_GETPID:
        return sys_getpid(task);
    case KSYSCALL_EXIT:
        return sys_exit((int)regs->arg0);
    default:
        return ksyscall_result(-1, KSYSCALL_ENOSYS);
    }
}
