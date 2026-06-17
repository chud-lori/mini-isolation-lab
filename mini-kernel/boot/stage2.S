.section .text
.code16
.global stage2_start

stage2_start:
    cli
    movw %cs, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0xfffe, %sp
    sti

    call clear_screen
    movw $banner, %si
    call print_string

command_loop:
    movw $prompt, %si
    call print_string
    movw $command_buffer, %di
    movw $0, %cx

read_key:
    xorw %ax, %ax
    int $0x16

    cmpb $0x0d, %al
    je finish_command

    cmpb $0x08, %al
    je backspace

    cmpw $31, %cx
    jae read_key

    stosb
    incw %cx
    call print_char
    jmp read_key

backspace:
    cmpw $0, %cx
    je read_key
    decw %di
    decw %cx
    movw $erase_char, %si
    call print_string
    jmp read_key

finish_command:
    movb $0, (%di)
    movw $newline, %si
    call print_string

    movw $command_buffer, %si
    movw $help_command, %di
    call streq
    cmpb $1, %al
    je show_help

    movw $command_buffer, %si
    movw $about_command, %di
    call streq
    cmpb $1, %al
    je show_about

    movw $command_buffer, %si
    movw $clear_command, %di
    call streq
    cmpb $1, %al
    je show_clear

    movw $command_buffer, %si
    movw $halt_command, %di
    call streq
    cmpb $1, %al
    je halt_system

    movw $command_buffer, %si
    movw $reboot_command, %di
    call streq
    cmpb $1, %al
    je reboot_system

    cmpb $0, command_buffer
    je command_loop

    movw $unknown, %si
    call print_string
    jmp command_loop

show_help:
    movw $help_text, %si
    call print_string
    jmp command_loop

show_about:
    movw $about_text, %si
    call print_string
    jmp command_loop

show_clear:
    call clear_screen
    jmp command_loop

halt_system:
    movw $halt_text, %si
    call print_string
halt_loop:
    cli
    hlt
    jmp halt_loop

reboot_system:
    int $0x19
    jmp halt_loop

streq:
    pushw %si
    pushw %di
streq_loop:
    movb (%si), %al
    movb (%di), %ah
    cmpb %ah, %al
    jne streq_false
    cmpb $0, %al
    je streq_true
    incw %si
    incw %di
    jmp streq_loop

streq_true:
    movb $1, %al
    jmp streq_done

streq_false:
    movb $0, %al

streq_done:
    popw %di
    popw %si
    ret

print_string:
    lodsb
    testb %al, %al
    jz print_done
    call print_char
    jmp print_string

print_done:
    ret

print_char:
    movb $0x0e, %ah
    movb $0x00, %bh
    int $0x10
    ret

clear_screen:
    movw $0x0600, %ax
    movb $0x07, %bh
    xorw %cx, %cx
    movw $0x184f, %dx
    int $0x10
    movw $0x0200, %ax
    xorw %bx, %bx
    xorw %dx, %dx
    int $0x10
    ret

banner:
    .asciz "Mini Kernel stage2 monitor\r\nType 'help' for commands.\r\n"

prompt:
    .asciz "mk> "

newline:
    .asciz "\r\n"

erase_char:
    .byte 0x08, ' ', 0x08, 0

unknown:
    .asciz "unknown command\r\n"

help_text:
    .asciz "commands: help, about, clear, halt, reboot\r\n"

about_text:
    .asciz "Mini Kernel: tiny BIOS-loaded OS monitor, built from scratch.\r\n"

halt_text:
    .asciz "halted\r\n"

help_command:
    .asciz "help"

about_command:
    .asciz "about"

clear_command:
    .asciz "clear"

halt_command:
    .asciz "halt"

reboot_command:
    .asciz "reboot"

command_buffer:
    .space 32
