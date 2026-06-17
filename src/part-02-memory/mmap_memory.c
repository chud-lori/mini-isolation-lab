#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
    char *heap = malloc(64);
    if (!heap) {
        perror("malloc");
        return 1;
    }
    strcpy(heap, "malloc: memory managed by the C allocator");

    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    char *mapped = mmap(NULL,
                        page_size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        free(heap);
        return 1;
    }
    strcpy(mapped, "mmap: memory requested directly from the kernel");

    printf("%s\n", heap);
    printf("%s\n", mapped);
    printf("page size: %zu bytes\n", page_size);

    munmap(mapped, page_size);
    free(heap);
    return 0;
}
