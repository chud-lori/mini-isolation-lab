#include <kernel/arch.h>
#include <kernel/log.h>
#include <kernel/mem.h>
#include <kernel/string.h>
#include <kernel/syscall.h>

static char banner_buffer[64];

void klog_init(void)
{
    arch_early_init();
}

void klog_putc(char ch)
{
    if (ch == '\n') {
        arch_serial_putc('\r');
    }
    arch_serial_putc(ch);
}

void klog_write(const char *text)
{
    if (text == NULL) {
        return;
    }

    for (size_t i = 0; text[i] != '\0'; i++) {
        klog_putc(text[i]);
    }
}

void klog_writeln(const char *text)
{
    klog_write(text);
    klog_putc('\n');
}

void kernel_start(void)
{
    klog_init();

    const char *banner = "mini-kernel alive";
    if (!kmemcpy_checked(banner_buffer, sizeof(banner_buffer), banner, kstrlen(banner) + 1)) {
        klog_writeln("boot banner copy failed");
        arch_halt();
    }

    struct ktask init_task = {
        .pid = 1,
        .user_base = (uintptr_t)banner_buffer,
        .user_len = kstrlen(banner_buffer),
    };
    struct ksyscall_regs write_banner = {
        .nr = KSYSCALL_WRITE,
        .arg0 = KSYSCALL_FD_STDOUT,
        .arg1 = (uintptr_t)banner_buffer,
        .arg2 = kstrlen(banner_buffer),
    };
    struct ksyscall_result write_result = ksyscall_dispatch(&init_task, &write_banner);
    if (write_result.error != KSYSCALL_OK) {
        klog_writeln("sys_write failed");
        arch_halt();
    }

    klog_putc('\n');
    klog_writeln("syscall dispatcher online; entering secure halt loop");

    arch_halt();
}
