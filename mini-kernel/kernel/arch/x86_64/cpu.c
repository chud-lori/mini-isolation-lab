#include <kernel/arch.h>

void arch_early_init(void)
{
    arch_serial_init();
}

void arch_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

uint8_t arch_inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

void arch_halt(void)
{
    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}

