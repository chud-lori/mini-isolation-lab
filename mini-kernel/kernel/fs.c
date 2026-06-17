#include <kernel/fs.h>

#include <kernel/string.h>

static const struct kfs_node *mounted_nodes;
static size_t mounted_count;

static bool node_is_valid(const struct kfs_node *node)
{
    if (node == NULL || node->path == NULL || node->mode != KFS_MODE_FILE) {
        return false;
    }

    if (node->size > 0 && node->data == NULL) {
        return false;
    }

    return true;
}

void kfs_mount(const struct kfs_node *nodes, size_t count)
{
    mounted_nodes = nodes;
    mounted_count = count;
}

const struct kfs_node *kfs_lookup(const char *path, size_t path_len)
{
    if (path == NULL || path_len == 0 || mounted_nodes == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < mounted_count; i++) {
        const struct kfs_node *node = &mounted_nodes[i];
        if (node_is_valid(node) && kstrlen(node->path) == path_len && kmem_equal(node->path, path, path_len)) {
            return node;
        }
    }

    return NULL;
}

enum kfs_error kfs_open(const char *path, struct kfs_handle *out_handle)
{
    const struct kfs_node *node;

    if (path == NULL || out_handle == NULL) {
        return KFS_EINVAL;
    }

    node = kfs_lookup(path, kstrlen(path));
    if (node == NULL) {
        out_handle->node = NULL;
        out_handle->offset = 0;
        return KFS_ENOENT;
    }

    out_handle->node = node;
    out_handle->offset = 0;
    return KFS_OK;
}

enum kfs_error kfs_stat_node(const struct kfs_node *node, struct kfs_stat *out)
{
    if (!node_is_valid(node) || out == NULL) {
        return KFS_EINVAL;
    }

    out->size = node->size;
    out->mode = node->mode;
    out->is_file = true;
    return KFS_OK;
}

enum kfs_error kfs_stat(const char *path, struct kfs_stat *out)
{
    const struct kfs_node *node;

    if (path == NULL || out == NULL) {
        return KFS_EINVAL;
    }

    node = kfs_lookup(path, kstrlen(path));
    if (node == NULL) {
        out->size = 0;
        out->mode = 0;
        out->is_file = false;
        return KFS_ENOENT;
    }

    return kfs_stat_node(node, out);
}

size_t kfs_read_node(const struct kfs_node *node, size_t offset, void *dst, size_t len)
{
    uint8_t *out = dst;

    if (!node_is_valid(node) || (len > 0 && out == NULL) || offset >= node->size) {
        return 0;
    }

    size_t available = node->size - offset;
    size_t to_copy = len < available ? len : available;
    for (size_t i = 0; i < to_copy; i++) {
        out[i] = node->data[offset + i];
    }

    return to_copy;
}

enum kfs_error kfs_read(struct kfs_handle *handle, void *dst, size_t len, size_t *out_read)
{
    size_t bytes_read;

    if (out_read != NULL) {
        *out_read = 0;
    }

    if (handle == NULL || !node_is_valid(handle->node)) {
        return KFS_EINVAL;
    }

    if (len > 0 && dst == NULL) {
        return KFS_EINVAL;
    }

    bytes_read = kfs_read_node(handle->node, handle->offset, dst, len);
    handle->offset += bytes_read;

    if (out_read != NULL) {
        *out_read = bytes_read;
    }

    return KFS_OK;
}
