#ifndef KERNEL_ARCH_H
#define KERNEL_ARCH_H

#include <kernel/compiler.h>
#include <kernel/types.h>

void arch_early_init(void);
KERNEL_NORETURN void arch_halt(void);
void arch_outb(uint16_t port, uint8_t value);
uint8_t arch_inb(uint16_t port);
void arch_serial_init(void);
void arch_serial_putc(char ch);

#endif

