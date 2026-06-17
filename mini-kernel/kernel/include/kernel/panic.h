#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <kernel/compiler.h>

KERNEL_NORETURN void kernel_panic(const char *message);

#endif

