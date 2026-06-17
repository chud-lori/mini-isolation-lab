.section .multiboot2, "a"
.align 8

/* Multiboot2 header: enough for a compliant bootloader to identify the image. */
mb2_header_start:
    .long 0xe85250d6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xe85250d6 + 0 + (mb2_header_end - mb2_header_start))

    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

