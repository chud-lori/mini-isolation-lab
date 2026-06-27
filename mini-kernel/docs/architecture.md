# Architecture

This scaffold targets a minimal x86_64 kernel that can be built from C, Rust, or a
small mix of both. There are two boot tracks:

- Working today: a raw BIOS disk image whose boot sector loads a contiguous
  stage2 text monitor.
- Future kernel path: a bootloader-backed ELF kernel using the existing
  Multiboot2 header, linker script, and Limine configuration as starting points.

Limine or another Multiboot2-compatible bootloader is still the preferred route
for the full kernel because it can provide a clean 64-bit entry environment.
The raw BIOS path is intentionally small and exists so local boot-image work can
continue without vendoring or downloading a bootloader.

## Shape

- `boot/`: BIOS boot sector and stage2 monitor, plus bootloader configuration
  and linker files for the future ELF kernel path.
- `kernel/`: freestanding kernel code and linker script.
- `scripts/`: helper commands for building images and running emulators.
- `tests/`: host-side checks or emulator smoke tests.
- `docs/`: design, security, and boot notes.

The future bootloader-backed kernel should keep the early path intentionally small:

1. Receive control from Limine at a single architecture entry point.
2. Validate required Limine responses before touching memory or devices.
3. Initialize a console path for diagnostics.
4. Install a GDT and IDT, then enable basic exception handling.
5. Read Limine's memory map and build a simple physical page allocator.
6. Create kernel page tables with a higher-half direct map if requested.
7. Hand off to architecture-neutral kernel initialization.

## C And Rust Boundary

Use a C ABI for cross-language entry points. A Rust kernel can expose
`extern "C"` functions and use `#[repr(C)]` structs for shared boot data. A C
kernel can keep the same ABI so architecture assembly, tests, or future Rust
modules do not need to know implementation details.

Recommended conventions:

- Freestanding build flags: no host libc, no red zone, explicit target triple.
- Panic/assert paths print a short message and halt.
- Shared structs mirror Limine data only where needed; avoid copying the entire
  protocol surface into the kernel core.
- Architecture-specific code stays under an `x86_64` module or directory.

## Memory Model

Start with one allocator and one mapping policy:

- Treat all Limine memory map entries as untrusted until checked.
- Reserve the kernel image, bootloader reclaimable memory until after init, and
  framebuffer or module ranges.
- Use 4 KiB pages first; add huge pages later only after the page table code is
  tested.
- Keep user/kernel isolation as a design requirement, even if the first build is
  kernel-only.

## Current Host-Tested Kernel Surface

The C kernel code currently exposes portable pieces that run under host tests:

- checked memory/string helpers;
- a tiny read-only in-memory filesystem;
- a POSIX-like syscall dispatcher for read, write, open, close, stat, getpid,
  and exit;
- x86_64 GDT/GDTR and IDT/IDTR descriptor builders and default table setup with
  exact-byte host tests, plus freestanding `lgdt`/`lidt` loading helpers that
  compile as kernel objects;
- a simulated userspace runtime that exercises syscall-entry state without a
  hardware ring-3 transition.

The BIOS stage2 monitor and the C kernel are not wired together yet.

## Near-Term Milestones

- Keep the raw BIOS image booting to the stage2 monitor while kernel work grows.
- Boot the ELF kernel through a bootloader to a text or framebuffer message in QEMU.
- Add fault handlers with register dumps for the loaded IDT.
- Add physical and virtual memory managers.
- Add a timer interrupt and a cooperative task stub.
- Replace the simulated userspace runtime with hardware-enforced user/kernel
  isolation once user-mode paging exists.
