#include <kernel/string.h>
#include <kernel/user.h>

#include <assert.h>

struct user_capture {
    char bytes[128];
    size_t len;
};

static const uint8_t user_file_data[] = "abc";
static const struct kfs_node user_nodes[] = {
    {.path = "/user.txt", .data = user_file_data, .size = sizeof(user_file_data) - 1, .mode = KFS_MODE_FILE},
};

static void capture_byte(void *ctx, char ch)
{
    struct user_capture *capture = ctx;
    assert(capture->len < sizeof(capture->bytes));
    capture->bytes[capture->len++] = ch;
}

static void test_init_program(void)
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

static void test_user_stat_writes_stat_struct(void)
{
    static const uint8_t memory[] = "/user.txt\0";
    static const struct kuser_op ops[] = {
        {.opcode = KUSER_OP_STAT, .offset = 0, .len = 9, .aux_offset = 32},
        {.opcode = KUSER_OP_EXIT, .exit_status = 0},
    };
    const struct kuser_program program = {
        .initial_memory = memory,
        .initial_memory_len = sizeof(memory),
        .ops = ops,
        .op_count = sizeof(ops) / sizeof(ops[0]),
        .fs_nodes = user_nodes,
        .fs_node_count = sizeof(user_nodes) / sizeof(user_nodes[0]),
    };
    struct kuser_process process;
    struct kfs_stat *stat;

    kuser_init_process(&process, 101, NULL, NULL);

    assert(kuser_run(&process, &program) == KUSER_OK);
    assert(process.exited);
    assert(process.exit_status == 0);

    stat = (struct kfs_stat *)&process.memory[32];
    assert(stat->size == sizeof(user_file_data) - 1);
    assert(stat->mode == KFS_MODE_FILE);
    assert(stat->is_file);
}

static void test_user_zero_length_write_at_memory_end(void)
{
    static const struct kuser_op ops[] = {
        {.opcode = KUSER_OP_WRITE, .offset = KUSER_MEMORY_SIZE, .len = 0},
        {.opcode = KUSER_OP_EXIT, .exit_status = 7},
    };
    const struct kuser_program program = {
        .initial_memory = NULL,
        .initial_memory_len = 0,
        .ops = ops,
        .op_count = sizeof(ops) / sizeof(ops[0]),
        .fs_nodes = NULL,
        .fs_node_count = 0,
    };
    struct user_capture capture = {{0}, 0};
    struct kuser_process process;

    kuser_init_process(&process, 102, capture_byte, &capture);

    assert(kuser_run(&process, &program) == KUSER_OK);
    assert(process.exited);
    assert(process.exit_status == 7);
    assert(capture.len == 0);
}

static void test_user_close_frees_fd_slot_for_reopen(void)
{
    static const uint8_t memory[] = "/user.txt\0";
    static const struct kuser_op ops[] = {
        {.opcode = KUSER_OP_OPEN, .fd_slot = 0, .offset = 0, .len = 9},
        {.opcode = KUSER_OP_CLOSE, .fd_slot = 0},
        {.opcode = KUSER_OP_OPEN, .fd_slot = 0, .offset = 0, .len = 9},
        {.opcode = KUSER_OP_READ, .fd_slot = 0, .offset = 32, .len = 3},
        {.opcode = KUSER_OP_CLOSE, .fd_slot = 0},
        {.opcode = KUSER_OP_EXIT, .exit_status = 0},
    };
    const struct kuser_program program = {
        .initial_memory = memory,
        .initial_memory_len = sizeof(memory),
        .ops = ops,
        .op_count = sizeof(ops) / sizeof(ops[0]),
        .fs_nodes = user_nodes,
        .fs_node_count = sizeof(user_nodes) / sizeof(user_nodes[0]),
    };
    struct kuser_process process;

    kuser_init_process(&process, 103, NULL, NULL);

    assert(kuser_run(&process, &program) == KUSER_OK);
    assert(process.exited);
    assert(process.fd_slots[0] == -1);
    assert(kmem_equal(&process.memory[32], "abc", 3));
}

void test_user_runtime(void)
{
    test_init_program();
    test_user_stat_writes_stat_struct();
    test_user_zero_length_write_at_memory_end();
    test_user_close_frees_fd_slot_for_reopen();
}
