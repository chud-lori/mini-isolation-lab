# Architecture

This scaffold targets a minimal x86_64 kernel that can be built from C, Rust, or a
small mix of both. Limine is the expected boot protocol because it provides a
clean 64-bit entry environment and avoids writing early boot assembly before the
kernel has something useful to run.

## Shape

- `boot/`: bootloader configuration, ISO staging files, and Limine assets.
- `kernel/`: freestanding kernel code and linker script.
- `scripts/`: helper commands for building images and running emulators.
- `tests/`: host-side checks or emulator smoke tests.
- `docs/`: design, security, and boot notes.

The kernel should keep the early path intentionally small:

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

## Near-Term Milestones

- Boot to a text or framebuffer message in QEMU.
- Add IDT handlers for faults with register dumps.
- Add physical and virtual memory managers.
- Add a timer interrupt and a cooperative task stub.
- Add a tiny syscall ABI only after user-mode paging exists.
