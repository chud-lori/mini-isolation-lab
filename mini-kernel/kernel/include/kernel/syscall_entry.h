#ifndef KERNEL_SYSCALL_ENTRY_H
#define KERNEL_SYSCALL_ENTRY_H

#include <kernel/syscall.h>
#include <kernel/types.h>

/*
 * Host-testable syscall trap boundary.
 *
 * The x86_64 syscall instruction enters the kernel with the syscall number in
 * rax and arguments in rdi, rsi, rdx, r10, r8, and r9. Hardware also saves the
 * user return rip in rcx and rflags in r11. A future assembly stub can save
 * those registers into this frame, call ksyscall_entry(), then restore the
 * user-visible result fields before sysret/iret.
 */
struct ksyscall_trap_frame {
    uintptr_t rax;
    uintptr_t rdi;
    uintptr_t rsi;
    uintptr_t rdx;
    uintptr_t r10;
    uintptr_t r8;
    uintptr_t r9;
    uintptr_t rcx;
    uintptr_t r11;
    uintptr_t user_rsp;

    intptr_t result;
    enum ksyscall_error error;
    bool should_exit;
    int exit_status;
};

void ksyscall_entry(struct ktask *task, struct ksyscall_trap_frame *frame);

#endif
