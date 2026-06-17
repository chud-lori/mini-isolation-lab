.section .text
.code16
.global boot_sector_start

#ifndef STAGE2_SECTORS
#define STAGE2_SECTORS 1
#endif

#if STAGE2_SECTORS > 17
#error "stage2 must fit sectors 2..18 for the simple CHS loader"
#endif

.set STAGE2_SEGMENT, 0x0800
.set STAGE2_OFFSET, 0x0000

boot_sector_start:
    ljmp $0x0000, $boot_entry

boot_entry:
    cli
    movw $0x07c0, %ax
    movw %ax, %ds
    xorw %ax, %ax
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    sti
    movb %dl, boot_drive

    movw $boot_message, %si
    call print_string

    movw $STAGE2_SEGMENT, %ax
    movw %ax, %es
    xorw %bx, %bx
    movb $0x02, %ah
    movb $STAGE2_SECTORS, %al
    movb $0x00, %ch
    movb $0x00, %dh
    movb $0x02, %cl
    movb boot_drive, %dl
    int $0x13
    jc disk_error

    movw $ok_message, %si
    call print_string
    ljmp $STAGE2_SEGMENT, $STAGE2_OFFSET

disk_error:
    movw $disk_error_message, %si
    call print_string
    jmp halt_loop

print_string:
    lodsb
    testb %al, %al
    jz print_done
    movb $0x0e, %ah
    movb $0x00, %bh
    int $0x10
    jmp print_string

print_done:
    ret

halt_loop:
    cli
    hlt
    jmp halt_loop

boot_drive:
    .byte 0

boot_message:
    .asciz "mini-kernel bootloader\r\n"

ok_message:
    .asciz "stage2 loaded\r\n"

disk_error_message:
    .asciz "disk read failed\r\n"

    .org 510
    .word 0xaa55
