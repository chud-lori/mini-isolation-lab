#include <kernel/arch.h>
#include <kernel/log.h>
#include <kernel/panic.h>

void kernel_panic(const char *message)
{
    klog_write("kernel panic: ");
    klog_writeln(message != NULL ? message : "(null)");
    arch_halt();
}

