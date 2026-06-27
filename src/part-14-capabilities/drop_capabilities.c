#define _GNU_SOURCE

#ifndef __linux__
#error "drop_capabilities.c requires Linux capability syscalls."
#endif

#include <errno.h>
#include <linux/capability.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void print_status_field(const char *label, const char *field) {
    FILE *status = fopen("/proc/self/status", "r");
    if (!status) {
        fprintf(stderr, "%s: could not open /proc/self/status: %s\n",
                label,
                strerror(errno));
        return;
    }

    char *line = NULL;
    size_t cap = 0;
    size_t field_len = strlen(field);
    while (getline(&line, &cap, status) > 0) {
        if (strncmp(line, field, field_len) == 0) {
            printf("%s: %s", label, line);
            break;
        }
    }

    free(line);
    fclose(status);
}

static void print_cap_snapshot(const char *label) {
    print_status_field(label, "Uid:");
    print_status_field(label, "CapInh:");
    print_status_field(label, "CapPrm:");
    print_status_field(label, "CapEff:");
    print_status_field(label, "CapBnd:");
}

static void get_caps(struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3]) {
    struct __user_cap_header_struct header;
    memset(&header, 0, sizeof(header));
    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = 0;

    memset(data, 0, sizeof(struct __user_cap_data_struct) * _LINUX_CAPABILITY_U32S_3);
    if (syscall(SYS_capget, &header, data) < 0) {
        die("capget");
    }
}

static void drop_process_caps(void) {
    struct __user_cap_header_struct header;
    struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];

    memset(&header, 0, sizeof(header));
    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = 0;

    memset(data, 0, sizeof(data));
    if (syscall(SYS_capset, &header, data) < 0) {
        die("capset");
    }
}

static unsigned long long effective_caps_to_u64(
    const struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3]) {
    return ((unsigned long long)data[1].effective << 32) | data[0].effective;
}

static void try_raw_socket(const char *label) {
    int fd = socket(AF_INET, SOCK_RAW | SOCK_CLOEXEC, IPPROTO_ICMP);
    if (fd >= 0) {
        printf("%s: raw ICMP socket opened (CAP_NET_RAW was effective)\n", label);
        close(fd);
        return;
    }

    if (errno == EPERM || errno == EACCES) {
        printf("%s: raw ICMP socket denied: %s\n", label, strerror(errno));
        return;
    }

    printf("%s: raw ICMP socket failed for another reason: %s\n",
           label,
           strerror(errno));
}

int main(void) {
    struct __user_cap_data_struct before[_LINUX_CAPABILITY_U32S_3];
    struct __user_cap_data_struct after[_LINUX_CAPABILITY_U32S_3];

    printf("capability demo: drop effective/permitted/inheritable sets\n");
    print_cap_snapshot("before");
    get_caps(before);
    try_raw_socket("before drop");

    drop_process_caps();

    print_cap_snapshot("after");
    get_caps(after);
    try_raw_socket("after drop");

    unsigned long long effective_after = effective_caps_to_u64(after);
    if (effective_after != 0) {
        fprintf(stderr,
                "expected zero effective capabilities after capset, got 0x%llx\n",
                effective_after);
        return EXIT_FAILURE;
    }

    if (before[0].effective == 0 && before[1].effective == 0) {
        printf("note: this process already started without effective capabilities; "
               "the capset call still proves the drop path is available.\n");
    } else {
        printf("capability drop succeeded: effective capability set is now zero\n");
    }

    return EXIT_SUCCESS;
}
