#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernel/types.h>

enum kfs_error {
    KFS_OK = 0,
    KFS_EINVAL = 22,
    KFS_ENOENT = 2,
};

enum kfs_mode {
    KFS_MODE_FILE = 0100000,
};

struct kfs_node {
    const char *path;
    const uint8_t *data;
    size_t size;
    uint32_t mode;
};

struct kfs_stat {
    size_t size;
    uint32_t mode;
    bool is_file;
};

struct kfs_handle {
    const struct kfs_node *node;
    size_t offset;
};

void kfs_mount(const struct kfs_node *nodes, size_t count);
const struct kfs_node *kfs_lookup(const char *path, size_t path_len);
enum kfs_error kfs_open(const char *path, struct kfs_handle *out_handle);
enum kfs_error kfs_read(struct kfs_handle *handle, void *dst, size_t len, size_t *out_read);
enum kfs_error kfs_stat(const char *path, struct kfs_stat *out);
enum kfs_error kfs_stat_node(const struct kfs_node *node, struct kfs_stat *out);
size_t kfs_read_node(const struct kfs_node *node, size_t offset, void *dst, size_t len);

#endif
