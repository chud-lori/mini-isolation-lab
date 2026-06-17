# Mini Kernel

Mini Kernel is a from-scratch starter OS kernel scaffold. It is intentionally
small, freestanding, and conservative: the first milestone is a serial-logging
kernel core with explicit unsafe hardware boundaries and host-side tests for
portable kernel logic.

This repository does not vendor a bootloader. The source is prepared for an
x86_64 boot path, while local verification and a tiny BIOS boot sector work
without network downloads. The raw BIOS path is bootsector at LBA 0 plus a
contiguous stage2 binary at LBA 1 and above.

## What exists now

- Freestanding C kernel core under `kernel/`
- x86_64 port I/O and CPU halt helpers
- Serial logger for early diagnostics
- Panic path that prints and halts
- Checked memory and string primitives for kernel use
- POSIX-like syscall layer: read, write, open, close, stat, getpid, exit
- Tiny read-only in-memory filesystem for kernel tests and early init data
- Multiboot2 header and linker script for bootloader integration
- 512-byte BIOS boot sector that loads a contiguous second-stage monitor
- Raw BIOS disk image target: bootsector at LBA 0, stage2 at LBA 1+
- Script support for assembling a raw BIOS image from bootsector + stage2
- Host tests for portable kernel modules
- Build scripts that detect missing optional tools

## Quick Start

```sh
make check
```

`make check` compiles the host-testable kernel modules and runs unit tests.
`make all` also builds the raw BIOS disk image, validates the boot sector and
stage2 layout, and compiles freestanding kernel objects.

The no-network artifact is:

```text
build/mini-kernel.img
```

The image layout is LBA 0 boot sector, then stage2 starting at LBA 1 and padded
to a 512-byte boundary. Stage2 currently provides a tiny BIOS text monitor with:

```text
help, about, clear, halt, reboot
```

When QEMU is installed, run it with:

```sh
make run-image
```

To build a bootable kernel image later, install optional local tools:

- `ld.lld`
- `qemu-system-x86_64`
- a Multiboot2-compatible bootloader such as GRUB, or adapt `boot/limine.cfg`
  for Limine

Then run:

```sh
make kernel-objects
make build/mini-kernel.elf
make run
```

## Design Goals

- Fast boot path with serial-first debugging
- Small trusted code base
- No writable-executable mappings once paging is implemented
- Unsafe CPU and device access isolated under `kernel/arch/`
- Panic and assertion failures halt safely
- Portable logic covered by host tests

## Support Matrix

- Partial POSIX-like API: `read`, `write`, `open`, `close`, `stat`, `getpid`, `exit`.
- Partial filesystem: read-only in-memory files, no disk driver yet.
- Userspace: not yet; syscalls are host-tested and callable from kernel code.
- Networking: not yet.
- SMP: not yet.
- USB: not yet.
- UEFI bootloader from scratch: not yet; BIOS raw-image path exists today.
