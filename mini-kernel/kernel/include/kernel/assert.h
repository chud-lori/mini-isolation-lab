#ifndef KERNEL_ASSERT_H
#define KERNEL_ASSERT_H

#include <kernel/panic.h>

#define KASSERT(expr) \
    do { \
        if (!(expr)) { \
            kernel_panic("assertion failed: " #expr); \
        } \
    } while (0)

#endif

