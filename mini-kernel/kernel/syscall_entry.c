#include <kernel/syscall_entry.h>

void ksyscall_entry(struct ktask *task, struct ksyscall_trap_frame *frame)
{
    struct ksyscall_result result;
    struct ksyscall_regs regs;

    if (frame == NULL) {
        return;
    }

    regs = (struct ksyscall_regs){
        .nr = frame->rax,
        .arg0 = frame->rdi,
        .arg1 = frame->rsi,
        .arg2 = frame->rdx,
        .arg3 = frame->r10,
        .arg4 = frame->r8,
        .arg5 = frame->r9,
    };

    result = ksyscall_dispatch(task, &regs);
    frame->rax = (uintptr_t)result.value;
    frame->result = result.value;
    frame->error = result.error;
    frame->should_exit = result.should_exit;
    frame->exit_status = result.exit_status;
}
