#include <kernel/panic.h>

#ifdef KERNEL_HOST_TEST
#include <stdio.h>
#include <stdlib.h>

void kernel_panic(const char *message)
{
    fprintf(stderr, "kernel panic: %s\n", message != NULL ? message : "(null)");
    abort();
}
#endif

