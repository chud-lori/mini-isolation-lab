#include <kernel/arch.h>

enum {
    COM1 = 0x3f8,
    SERIAL_DATA = 0,
    SERIAL_INTERRUPT = 1,
    SERIAL_FIFO = 2,
    SERIAL_LINE_CONTROL = 3,
    SERIAL_MODEM_CONTROL = 4,
    SERIAL_LINE_STATUS = 5,
};

static bool serial_can_transmit(void)
{
    return (arch_inb(COM1 + SERIAL_LINE_STATUS) & 0x20u) != 0;
}

void arch_serial_init(void)
{
    arch_outb(COM1 + SERIAL_INTERRUPT, 0x00);
    arch_outb(COM1 + SERIAL_LINE_CONTROL, 0x80);
    arch_outb(COM1 + SERIAL_DATA, 0x03);
    arch_outb(COM1 + SERIAL_INTERRUPT, 0x00);
    arch_outb(COM1 + SERIAL_LINE_CONTROL, 0x03);
    arch_outb(COM1 + SERIAL_FIFO, 0xc7);
    arch_outb(COM1 + SERIAL_MODEM_CONTROL, 0x0b);
}

void arch_serial_putc(char ch)
{
    while (!serial_can_transmit()) {
        __asm__ volatile("pause" : : : "memory");
    }

    arch_outb(COM1, (uint8_t)ch);
}

