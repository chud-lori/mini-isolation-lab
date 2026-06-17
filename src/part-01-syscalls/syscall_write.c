#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
    const char *message = "part 01: hello through the write syscall\n";

    /*
     * printf eventually calls write(2). Here we skip printf and ask the kernel
     * directly: write these bytes to file descriptor 1, which is stdout.
     */
    long ret = syscall(SYS_write, STDOUT_FILENO, message, strlen(message));
    if (ret < 0) {
        perror("syscall write");
        return 1;
    }

    return 0;
}
