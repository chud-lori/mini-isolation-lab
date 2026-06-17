#include <kernel/mem.h>
#include <kernel/string.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_strlen(void)
{
    assert(kstrlen(NULL) == 0);
    assert(kstrlen("") == 0);
    assert(kstrlen("mini") == 4);
}

static void test_mem_equal(void)
{
    const uint8_t a[] = {1, 2, 3};
    const uint8_t b[] = {1, 2, 3};
    const uint8_t c[] = {1, 2, 4};

    assert(kmem_equal(a, b, sizeof(a)));
    assert(!kmem_equal(a, c, sizeof(a)));
    assert(!kmem_equal(NULL, b, sizeof(b)));
    assert(kmem_equal(NULL, NULL, 0));
}

static void test_memset_and_copy(void)
{
    uint8_t buffer[8];
    uint8_t expected[8] = {0xaa, 0xaa, 0xaa, 0xaa, 0, 0, 0, 0};
    const uint8_t src[] = {9, 8, 7, 6};

    assert(kmemset(buffer, 0, sizeof(buffer)) == buffer);
    assert(kmemset(buffer, 0xaa, 4) == buffer);
    assert(kmem_equal(buffer, expected, sizeof(buffer)));

    assert(kmemcpy(buffer, src, sizeof(src)) == buffer);
    assert(kmem_equal(buffer, src, sizeof(src)));
    assert(kmemcpy(NULL, src, sizeof(src)) == NULL);
}

static void test_checked_copy(void)
{
    uint8_t dst[4] = {0};
    const uint8_t src[] = {1, 2, 3, 4, 5};

    assert(!kmemcpy_checked(dst, sizeof(dst), src, sizeof(src)));
    assert(kmem_equal(dst, "\0\0\0\0", sizeof(dst)));
    assert(kmemcpy_checked(dst, sizeof(dst), src, 4));
}

int main(void)
{
    test_strlen();
    test_mem_equal();
    test_memset_and_copy();
    test_checked_copy();
    puts("kernel lib tests passed");
    return 0;
}

