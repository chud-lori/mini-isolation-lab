#include <kernel/fs.h>
#include <kernel/mem.h>
#include <kernel/syscall.h>
#include <kernel/string.h>

#include <assert.h>
#include <stdint.h>

struct capture {
    char bytes[64];
    size_t len;
};

struct user_space {
    char path[32];
    char buffer[64];
    struct kfs_stat stat;
};

static const uint8_t readme_data[] = "hello kernel file";
static const struct kfs_node syscall_nodes[] = {
    {.path = "/readme.txt", .data = readme_data, .size = sizeof(readme_data) - 1, .mode = KFS_MODE_FILE},
};

static void capture_byte(void *ctx, char ch)
{
    struct capture *capture = ctx;
    assert(capture->len < sizeof(capture->bytes));
    capture->bytes[capture->len++] = ch;
}

static struct ktask test_task(struct user_space *user, struct capture *capture)
{
    return (struct ktask){
        .pid = 7,
        .user_base = (uintptr_t)user,
        .user_len = sizeof(*user),
        .write_byte = capture_byte,
        .write_ctx = capture,
    };
}

static void reset_fs(void)
{
    kfs_mount(syscall_nodes, sizeof(syscall_nodes) / sizeof(syscall_nodes[0]));
}

static void test_write_success(void)
{
    struct user_space user = {.buffer = "hello syscall"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);
    struct ksyscall_regs regs = {
        .nr = KSYSCALL_WRITE,
        .arg0 = KSYSCALL_FD_STDOUT,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = 5,
    };

    struct ksyscall_result result = ksyscall_dispatch(&task, &regs);
    assert(result.error == KSYSCALL_OK);
    assert(result.value == 5);
    assert(capture.len == 5);
    assert(kmem_equal(capture.bytes, "hello", 5));
}

static void test_open_read_close_stat(void)
{
    reset_fs();
    struct user_space user = {.path = "/readme.txt"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    struct ksyscall_result open_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
    });
    assert(open_result.error == KSYSCALL_OK);
    assert(open_result.value >= KSYSCALL_FD_FIRST_FILE);

    struct ksyscall_result read_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_READ,
        .arg0 = (uintptr_t)open_result.value,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = 5,
    });
    assert(read_result.error == KSYSCALL_OK);
    assert(read_result.value == 5);
    assert(kmem_equal(user.buffer, "hello", 5));

    struct ksyscall_result stat_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_STAT,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
        .arg2 = (uintptr_t)&user.stat,
    });
    assert(stat_result.error == KSYSCALL_OK);
    assert(user.stat.size == sizeof(readme_data) - 1);

    struct ksyscall_result close_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_CLOSE,
        .arg0 = (uintptr_t)open_result.value,
    });
    assert(close_result.error == KSYSCALL_OK);
}

static void test_open_missing_file(void)
{
    reset_fs();
    struct user_space user = {.path = "/missing"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);
    struct ksyscall_result result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 8,
    });
    assert(result.error == KSYSCALL_ENOENT);
}

static void test_read_rejects_bad_fd(void)
{
    struct user_space user = {0};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);
    struct ksyscall_result result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_READ,
        .arg0 = 99,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = 1,
    });
    assert(result.error == KSYSCALL_EBADF);
}

static void test_write_rejects_bad_pointer(void)
{
    struct user_space user = {.buffer = "abc"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);
    struct ksyscall_result result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_WRITE,
        .arg0 = KSYSCALL_FD_STDOUT,
        .arg1 = UINTPTR_MAX,
        .arg2 = 8,
    });
    assert(result.error == KSYSCALL_EFAULT);
    assert(capture.len == 0);
}


static void test_repeated_read_reaches_eof(void)
{
    reset_fs();
    struct user_space user = {.path = "/readme.txt"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    struct ksyscall_result open_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
    });
    assert(open_result.error == KSYSCALL_OK);

    struct ksyscall_result first = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_READ,
        .arg0 = (uintptr_t)open_result.value,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = sizeof(user.buffer),
    });
    assert(first.error == KSYSCALL_OK);
    assert(first.value == (intptr_t)(sizeof(readme_data) - 1));

    struct ksyscall_result eof = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_READ,
        .arg0 = (uintptr_t)open_result.value,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = sizeof(user.buffer),
    });
    assert(eof.error == KSYSCALL_OK);
    assert(eof.value == 0);
}

static void test_close_errors_and_read_after_close(void)
{
    reset_fs();
    struct user_space user = {.path = "/readme.txt"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    struct ksyscall_result open_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
    });
    assert(open_result.error == KSYSCALL_OK);

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_CLOSE,
        .arg0 = (uintptr_t)open_result.value,
    }).error == KSYSCALL_OK);

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_CLOSE,
        .arg0 = (uintptr_t)open_result.value,
    }).error == KSYSCALL_EBADF);

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_READ,
        .arg0 = (uintptr_t)open_result.value,
        .arg1 = (uintptr_t)user.buffer,
        .arg2 = 1,
    }).error == KSYSCALL_EBADF);
}

static void test_open_emfile(void)
{
    reset_fs();
    struct user_space user = {.path = "/readme.txt"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    for (size_t fd = KSYSCALL_FD_FIRST_FILE; fd < KTASK_MAX_FILES; fd++) {
        struct ksyscall_result result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
            .nr = KSYSCALL_OPEN,
            .arg0 = (uintptr_t)user.path,
            .arg1 = 11,
        });
        assert(result.error == KSYSCALL_OK);
        assert(result.value == (intptr_t)fd);
    }

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
    }).error == KSYSCALL_EMFILE);
}

static void test_stat_errors(void)
{
    reset_fs();
    struct user_space user = {.path = "/missing"};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_STAT,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 8,
        .arg2 = (uintptr_t)&user.stat,
    }).error == KSYSCALL_ENOENT);

    kmemcpy_checked(user.path, sizeof(user.path), "/readme.txt", 11);
    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_STAT,
        .arg0 = (uintptr_t)user.path,
        .arg1 = 11,
        .arg2 = UINTPTR_MAX,
    }).error == KSYSCALL_EFAULT);
}

static void test_open_rejects_bad_path_pointer(void)
{
    reset_fs();
    struct user_space user = {0};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    assert(ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_OPEN,
        .arg0 = UINTPTR_MAX,
        .arg1 = 11,
    }).error == KSYSCALL_EFAULT);
}

static void test_getpid_and_exit(void)
{
    struct user_space user = {0};
    struct capture capture = {{0}, 0};
    struct ktask task = test_task(&user, &capture);

    struct ksyscall_result pid = ksyscall_dispatch(&task, &(struct ksyscall_regs){.nr = KSYSCALL_GETPID});
    assert(pid.error == KSYSCALL_OK);
    assert(pid.value == 7);

    struct ksyscall_result exit_result = ksyscall_dispatch(&task, &(struct ksyscall_regs){
        .nr = KSYSCALL_EXIT,
        .arg0 = 42,
    });
    assert(exit_result.error == KSYSCALL_OK);
    assert(exit_result.should_exit);
    assert(exit_result.exit_status == 42);
}

static void test_unknown_syscall(void)
{
    struct ksyscall_result result = ksyscall_dispatch(NULL, &(struct ksyscall_regs){.nr = 999});
    assert(result.error == KSYSCALL_ENOSYS);
}

void test_syscalls(void)
{
    test_write_success();
    test_open_read_close_stat();
    test_open_missing_file();
    test_repeated_read_reaches_eof();
    test_close_errors_and_read_after_close();
    test_open_emfile();
    test_stat_errors();
    test_open_rejects_bad_path_pointer();
    test_read_rejects_bad_fd();
    test_write_rejects_bad_pointer();
    test_getpid_and_exit();
    test_unknown_syscall();
}
