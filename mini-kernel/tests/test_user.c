#include <kernel/string.h>
#include <kernel/user.h>

#include <assert.h>

struct user_capture {
    char bytes[128];
    size_t len;
};

static void capture_byte(void *ctx, char ch)
{
    struct user_capture *capture = ctx;
    assert(capture->len < sizeof(capture->bytes));
    capture->bytes[capture->len++] = ch;
}

void test_user_runtime(void)
{
    struct user_capture capture = {{0}, 0};
    struct kuser_process process;
    kuser_init_process(&process, 100, capture_byte, &capture);

    assert(kuser_run(&process, kuser_init_program()) == KUSER_OK);
    assert(process.exited);
    assert(process.exit_status == 0);
    assert(capture.len > 0);
    assert(kmem_equal(capture.bytes, "init: userspace online\n", 23));
}
