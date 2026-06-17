#ifndef KERNEL_MEM_H
#define KERNEL_MEM_H

#include <kernel/types.h>

void *kmemset(void *dst, int value, size_t len);
void *kmemcpy(void *dst, const void *src, size_t len);
bool kmemcpy_checked(void *dst, size_t dst_len, const void *src, size_t src_len);

#endif

