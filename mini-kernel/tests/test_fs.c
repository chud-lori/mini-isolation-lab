#include <kernel/fs.h>
#include <kernel/string.h>

#include <assert.h>

static const uint8_t readme_data[] = "hello from initramfs";
static const uint8_t config_data[] = {0x7f, 0x45, 0x4c, 0x46};
static const struct kfs_node test_nodes[] = {
    {.path = "/readme.txt", .data = readme_data, .size = sizeof(readme_data) - 1, .mode = KFS_MODE_FILE},
    {.path = "/empty", .data = (const uint8_t *)"", .size = 0, .mode = KFS_MODE_FILE},
    {.path = "/config.bin", .data = config_data, .size = sizeof(config_data), .mode = KFS_MODE_FILE},
};

static void reset_fs(void)
{
    kfs_mount(test_nodes, sizeof(test_nodes) / sizeof(test_nodes[0]));
}

static void test_lookup(void)
{
    const struct kfs_node *node = kfs_lookup("/readme.txt", 11);
    assert(node != NULL);
    assert(node->size == sizeof(readme_data) - 1);
    assert(kfs_lookup("/readme.txt/suffix", 11) != NULL);
    assert(kfs_lookup("/missing", 8) == NULL);
    assert(kfs_lookup(NULL, 0) == NULL);
}

static void test_node_helpers(void)
{
    const struct kfs_node *node = kfs_lookup("/readme.txt", 11);
    struct kfs_stat stat;

    assert(kfs_stat_node(node, &stat) == KFS_OK);
    assert(stat.size == sizeof(readme_data) - 1);
    assert(stat.mode == KFS_MODE_FILE);
    assert(stat.is_file);

    char out[6] = {0};
    assert(kfs_read_node(node, 0, out, 5) == 5);
    assert(kmem_equal(out, "hello", 5));
    assert(kfs_read_node(node, sizeof(readme_data), out, 5) == 0);
    assert(kfs_read_node(node, 0, NULL, 0) == 0);
}

static void test_open_and_read(void)
{
    struct kfs_handle handle;
    char out[32] = {0};
    size_t bytes_read = 99;

    assert(kfs_open("/readme.txt", &handle) == KFS_OK);
    assert(handle.offset == 0);

    assert(kfs_read(&handle, out, 5, &bytes_read) == KFS_OK);
    assert(bytes_read == 5);
    assert(handle.offset == 5);
    assert(kmem_equal(out, "hello", 5));

    assert(kfs_read(&handle, out + 5, sizeof(out) - 5, &bytes_read) == KFS_OK);
    assert(bytes_read == sizeof(readme_data) - 1 - 5);
    assert(kmem_equal(out, readme_data, sizeof(readme_data) - 1));

    bytes_read = 99;
    assert(kfs_read(&handle, out, sizeof(out), &bytes_read) == KFS_OK);
    assert(bytes_read == 0);
}

static void test_zero_length_read_does_not_touch_state(void)
{
    struct kfs_handle handle;
    size_t bytes_read = 99;

    assert(kfs_open("/readme.txt", &handle) == KFS_OK);
    assert(handle.offset == 0);

    assert(kfs_read(&handle, NULL, 0, &bytes_read) == KFS_OK);
    assert(bytes_read == 0);
    assert(handle.offset == 0);
}

static void test_empty_file(void)
{
    struct kfs_handle handle;
    char byte = 'x';
    size_t bytes_read = 99;

    assert(kfs_open("/empty", &handle) == KFS_OK);
    assert(kfs_read(&handle, &byte, 1, &bytes_read) == KFS_OK);
    assert(bytes_read == 0);
    assert(byte == 'x');
}

static void test_path_stat(void)
{
    struct kfs_stat stat;

    assert(kfs_stat("/config.bin", &stat) == KFS_OK);
    assert(stat.size == sizeof(config_data));
    assert(stat.mode == KFS_MODE_FILE);
    assert(stat.is_file);

    assert(kfs_lookup("/missing", 8) == NULL);
    assert(kfs_stat("/missing", &stat) == KFS_ENOENT);
    assert(stat.size == 0);
    assert(stat.mode == 0);
    assert(!stat.is_file);
    assert(kfs_stat("/config.bin", NULL) == KFS_EINVAL);
    assert(kfs_stat(NULL, &stat) == KFS_EINVAL);
}

static void test_remount_replaces_visible_nodes(void)
{
    static const uint8_t alt_data[] = "replacement";
    static const struct kfs_node alt_nodes[] = {
        {.path = "/alt", .data = alt_data, .size = sizeof(alt_data) - 1, .mode = KFS_MODE_FILE},
    };
    struct kfs_stat stat;

    assert(kfs_lookup("/readme.txt", 11) != NULL);

    kfs_mount(NULL, 0);
    assert(kfs_lookup("/readme.txt", 11) == NULL);
    assert(kfs_stat("/readme.txt", &stat) == KFS_ENOENT);

    kfs_mount(alt_nodes, sizeof(alt_nodes) / sizeof(alt_nodes[0]));
    assert(kfs_lookup("/readme.txt", 11) == NULL);
    assert(kfs_stat("/alt", &stat) == KFS_OK);
    assert(stat.size == sizeof(alt_data) - 1);

    reset_fs();
}

static void test_invalid_inputs(void)
{
    struct kfs_handle handle;
    size_t bytes_read = 99;

    assert(kfs_open("/missing", &handle) == KFS_ENOENT);
    assert(handle.node == NULL);
    assert(kfs_open(NULL, &handle) == KFS_EINVAL);
    assert(kfs_open("/readme.txt", NULL) == KFS_EINVAL);

    assert(kfs_open("/readme.txt", &handle) == KFS_OK);
    assert(kfs_read(&handle, NULL, 1, &bytes_read) == KFS_EINVAL);
    assert(bytes_read == 0);

    bytes_read = 99;
    assert(kfs_read(NULL, NULL, 0, &bytes_read) == KFS_EINVAL);
    assert(bytes_read == 0);
}

void test_fs(void)
{
    reset_fs();
    test_lookup();
    test_node_helpers();
    test_open_and_read();
    test_zero_length_read_does_not_touch_state();
    test_empty_file();
    test_path_stat();
    test_remount_replaces_visible_nodes();
    test_invalid_inputs();
}
