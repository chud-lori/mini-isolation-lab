# Security Posture

This is a toy kernel scaffold, not a hardened operating system. The goal is to
make insecure states obvious, keep early assumptions narrow, and leave room for
real isolation features as the kernel grows.

## Baseline Assumptions

- The bootloader and firmware are trusted for early development.
- Limine structures are treated as external input and checked before use.
- There is no confidentiality boundary until user mode, process address spaces,
  and syscall validation exist.
- QEMU is the primary test environment; hardware behavior may differ.

## Early Protections

- Compile freestanding code with warnings enabled and fail builds on serious
  diagnostics when practical.
- Disable the x86_64 red zone for kernel code.
- Keep stack setup explicit and avoid large stack allocations.
- Install an IDT early so faults are diagnosed instead of triple-faulting.
- Validate memory map ranges for overflow, alignment, and overlap with reserved
  kernel regions.
- Keep writable and executable mappings separate where paging setup allows it.
- Halt cleanly on invariant violations instead of continuing after corruption.

## Rust Notes

Rust is useful for reducing memory-safety bugs, but early kernel code still needs
`unsafe` for boot data, CPU registers, port I/O, and page tables. Keep unsafe
blocks small, documented, and wrapped by narrow safe interfaces once behavior is
understood.

Recommended defaults:

- `#![no_std]` and `#![no_main]`.
- `panic = "abort"`.
- No allocator until physical and virtual memory initialization is complete.
- Avoid global mutable state unless it has a clear initialization order.

## C Notes

C code should avoid assuming a hosted environment. Do not call libc functions
unless the kernel provides them. Prefer explicit-width integer types, checked
pointer arithmetic, and small local helpers for alignment and range validation.

Recommended defaults:

- `-ffreestanding`
- `-fno-stack-protector` only until a kernel stack protector runtime exists
- `-mno-red-zone`
- `-Wall -Wextra` with project-specific exceptions kept small

## Not Yet Covered

- Secure boot or measured boot.
- Kernel address space layout randomization.
- SMP safety.
- User-mode sandboxing.
- Filesystem, driver, or network attack surfaces.
- Side-channel mitigations.

Document each new trust boundary as it appears; the security model should grow
with the kernel instead of being reconstructed after the fact.
