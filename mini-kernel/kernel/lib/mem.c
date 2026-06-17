#include <kernel/mem.h>

void *kmemset(void *dst, int value, size_t len)
{
    uint8_t *out = dst;

    if (out == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        out[i] = (uint8_t)value;
    }

    return dst;
}

void *kmemcpy(void *dst, const void *src, size_t len)
{
    uint8_t *out = dst;
    const uint8_t *in = src;

    if (len == 0) {
        return dst;
    }

    if (out == NULL || in == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        out[i] = in[i];
    }

    return dst;
}

bool kmemcpy_checked(void *dst, size_t dst_len, const void *src, size_t src_len)
{
    if (src_len > dst_len) {
        return false;
    }

    return kmemcpy(dst, src, src_len) == dst;
}

