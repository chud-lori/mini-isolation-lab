#include <kernel/arch.h>
#include <kernel/log.h>
#include <kernel/mem.h>
#include <kernel/string.h>

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

    klog_writeln(banner_buffer);
    klog_writeln("serial online; entering secure halt loop");

    arch_halt();
}

