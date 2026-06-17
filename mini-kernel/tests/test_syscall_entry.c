#include <kernel/mem.h>
#include <kernel/string.h>
#include <kernel/syscall_entry.h>

#include <assert.h>
#include <stdint.h>

struct entry_capture {
    char bytes[32];
    size_t len;
};

struct entry_user_space {
    char buffer[32];
};

static void entry_capture_byte(void *ctx, char ch)
{
    struct entry_capture *capture = ctx;
    assert(capture->len < sizeof(capture->bytes));
    capture->bytes[capture->len++] = ch;
}

static struct ktask entry_test_task(
    struct entry_user_space *user,
    struct entry_capture *capture)
{
    return (struct ktask){
        .pid = 42,
        .user_base = (uintptr_t)user,
        .user_len = sizeof(*user),
        .write_byte = entry_capture_byte,
        .write_ctx = capture,
    };
}

static void test_entry_dispatches_register_frame(void)
{
    struct entry_user_space user = {.buffer = "entry"};
    struct entry_capture capture = {{0}, 0};
    struct ktask task = entry_test_task(&user, &capture);
    struct ksyscall_trap_frame frame = {
        .rax = KSYSCALL_WRITE,
        .rdi = KSYSCALL_FD_STDOUT,
        .rsi = (uintptr_t)user.buffer,
        .rdx = 5,
        .rcx = 0x1111,
        .r11 = 0x202,
        .user_rsp = 0x7000,
    };

    ksyscall_entry(&task, &frame);

    assert(frame.rax == 5);
    assert(frame.result == 5);
    assert(frame.error == KSYSCALL_OK);
    assert(!frame.should_exit);
    assert(frame.exit_status == 0);
    assert(frame.rcx == 0x1111);
    assert(frame.r11 == 0x202);
    assert(frame.user_rsp == 0x7000);
    assert(capture.len == 5);
    assert(kmem_equal(capture.bytes, "entry", 5));
}

static void test_entry_reports_dispatch_error(void)
{
    struct ksyscall_trap_frame frame = {.rax = 999};

    ksyscall_entry(NULL, &frame);

    assert(frame.rax == (uintptr_t)-1);
    assert(frame.result == -1);
    assert(frame.error == KSYSCALL_ENOSYS);
    assert(!frame.should_exit);
}

static void test_entry_preserves_exit_metadata(void)
{
    struct entry_user_space user = {0};
    struct entry_capture capture = {{0}, 0};
    struct ktask task = entry_test_task(&user, &capture);
    struct ksyscall_trap_frame frame = {
        .rax = KSYSCALL_EXIT,
        .rdi = 77,
    };

    ksyscall_entry(&task, &frame);

    assert(frame.rax == 0);
    assert(frame.result == 0);
    assert(frame.error == KSYSCALL_OK);
    assert(frame.should_exit);
    assert(frame.exit_status == 77);
}

static void test_entry_allows_null_frame(void)
{
    ksyscall_entry(NULL, NULL);
}

void test_syscall_entry(void)
{
    test_entry_dispatches_register_frame();
    test_entry_reports_dispatch_error();
    test_entry_preserves_exit_metadata();
    test_entry_allows_null_frame();
}
