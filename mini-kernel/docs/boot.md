# Boot And Run

The working boot path today is a raw BIOS disk image tested in QEMU. It uses a
512-byte boot sector at LBA 0 and a second-stage monitor at LBA 1 and above.

The intended full kernel path is separate: a freestanding x86_64 ELF kernel
loaded by a bootloader such as Limine or another Multiboot2-compatible loader.
The repo has the early pieces for that path, but the runnable no-network image
currently boots to the stage2 monitor, not into the C kernel.

## Optional Tools

- `ld.lld`: linker for freestanding ELF64 kernels.
- `qemu-system-x86_64`: emulator for local boot tests.
- `limine`: bootloader files and ISO image helpers.
- `xorriso`: commonly used when producing BIOS/UEFI bootable ISO images.

`llvm-objcopy` is enough for the raw BIOS image. A full bootloader-backed kernel
run also needs an ELF linker and emulator.

## Current No-Network Flow

```sh
make all
```

This runs host tests, validates `build/mini-kernel.img`, and compiles kernel
objects. The image can be run later with:

```sh
make run-image
```

That target requires `qemu-system-x86_64` and starts the stage2 monitor.

## Raw BIOS Image Path

For a boot path without an ISO bootloader, keep the disk layout simple:

```text
LBA 0      512-byte BIOS boot sector with 55 aa signature
LBA 1..n   stage2 binary, padded to 512-byte sectors
```

The boot sector preserves the BIOS drive number, reads the padded stage2 from
sector 2 using BIOS `int 13h`, and jumps to `0800:0000`. Stage2 sets its own
segment state and provides a tiny monitor with `help`, `about`, `clear`, `halt`,
and `reboot`.

Build the image with:

```sh
make image
```

Run that image in QEMU with:

```sh
make run-image
```

Keep the boot sector small. Put protected-mode or long-mode setup in stage2
unless there is a strong reason not to. Do not treat this raw image as the final
kernel boot ABI; it is a practical monitor path while the bootloader-backed
kernel path matures.

## Future Bootloader-Backed Kernel Flow

1. Compile C or Rust kernel objects for a freestanding x86_64 target.
2. Link the kernel with a linker script into an ELF64 image.
3. Stage the kernel, `limine.conf`, and Limine boot files into an ISO directory.
4. Create a bootable ISO or disk image.
5. Run the image with QEMU.

For C builds, typical flags include:

```sh
-ffreestanding -mno-red-zone -fno-pic -fno-pie
```

For Rust builds, use a custom target or a known bare-metal x86_64 target with:

```toml
panic = "abort"
```

The linker should set the boot protocol entry point and place kernel sections at
the virtual and physical addresses expected by that protocol.

## Limine Configuration

A minimal `limine.conf` normally names the kernel path and protocol. This repo
currently keeps a Multiboot2-compatible example while leaving room to adapt to a
Limine-native protocol later:

```text
TIMEOUT=0

:mini-kernel
    PROTOCOL=multiboot2
    KERNEL_PATH=boot:///mini-kernel.elf
```

Keep boot configuration small until multiple kernels, modules, or command-line
options are needed.

## QEMU Smoke Test

For the raw BIOS image, a basic run command is:

```sh
qemu-system-x86_64 -drive format=raw,file=build/mini-kernel.img -serial stdio
```

Useful additions while debugging:

```sh
-no-reboot -no-shutdown -d int
```

For a future bootloader-backed kernel, `make run` uses `qemu-system-x86_64
-kernel build/mini-kernel.elf` after the ELF is linked. If the kernel writes to a
framebuffer instead of serial, keep serial logging available for faults and CI
smoke tests.

## Troubleshooting

- Immediate reset usually means a triple fault; check the IDT, stack, and entry
  address.
- No Limine response usually means the request structure was not retained,
  aligned, or placed in the expected section.
- Linker errors usually mean the linker script and object format disagree.
- QEMU boot failure usually means the ISO was not made bootable or Limine files
  were not installed into the image.
