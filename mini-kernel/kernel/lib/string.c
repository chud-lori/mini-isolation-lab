#include <kernel/string.h>

size_t kstrlen(const char *text)
{
    size_t len = 0;

    if (text == NULL) {
        return 0;
    }

    while (text[len] != '\0') {
        len++;
    }

    return len;
}

bool kmem_equal(const void *lhs, const void *rhs, size_t len)
{
    const uint8_t *left = lhs;
    const uint8_t *right = rhs;

    if (len == 0) {
        return true;
    }

    if (left == NULL || right == NULL) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (left[i] != right[i]) {
            return false;
        }
    }

    return true;
}

