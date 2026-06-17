#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <kernel/types.h>

size_t kstrlen(const char *text);
bool kmem_equal(const void *lhs, const void *rhs, size_t len);

#endif

