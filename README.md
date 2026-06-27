# Mini Isolation Lab

Beginner-friendly demos for learning how Linux containers and Firecracker-style
microVMs work under the hood.

This repository contains fourteen Linux demo lessons and one tiny OS/kernel learning project:

- Parts 01-06: focused C demos for syscalls, memory, processes, namespaces, root filesystems, and cgroups.
- Part 07, `mini-container`: combines Linux namespaces, `chroot`, `/proc`, and cgroups.
- Parts 08-10, `mini-firecracker`: use Linux KVM to inspect the API, map guest memory, and run a tiny guest program in a microVM-like VM.
- Part 11: demonstrates practical container hardening with `no_new_privs`, seccomp strict mode, and `/proc/self/status` inspection.
- Part 12: maps the current host uid/gid to root inside a new Linux user namespace.
- Part 13: checks KVM capabilities used by microVM and virtio-adjacent VMM paths.
- Part 14: drops Linux effective, permitted, and inheritable capabilities without libcap.
- `mini-kernel`: builds a working raw BIOS image with a 512-byte boot sector and stage2 monitor, plus a freestanding kernel scaffold used by the `/kernel` course page.

The tutorial page is `docs/index.html`, ready for GitHub Pages. The kernel workbench page is `docs/kernel/index.html`, published at `/kernel/`, and the kernel course index is `docs/kernel/course/index.html`, published at `/kernel/course/`.

## Quick Map

```text
mini-isolation-lab/
  docs/index.html                          GitHub Pages tutorial
  docs/kernel/index.html                   OS/kernel course page
  mini-kernel/                             tiny OS and freestanding kernel scaffold
  Makefile                                 builds the demos on Linux
  src/part-01-syscalls/syscall_write.c     first syscall example
  src/part-02-memory/mmap_memory.c         mmap and virtual memory example
  src/part-03-processes/clone_process.c    clone and wait example
  src/part-04-namespaces/uts_namespace.c   namespace example
  src/part-05-rootfs/chroot_exec.c         root filesystem example
  src/part-06-cgroups/cgroup_limit.c       cgroup v2 example
  src/part-07-container/mini_container.c   container demo
  src/part-08-kvm/kvm_api_version.c        KVM API handshake demo
  src/part-09-microvm/mini_firecracker.c   microVM/KVM demo
  src/part-10-microvm-memory/kvm_memory_slots.c  KVM memory slot demo
  src/part-11-hardening/container_hardening.c    container hardening demo
  src/part-12-userns/userns_map.c                user namespace uid/gid mapping demo
  src/part-13-kvm-capabilities/kvm_capabilities.c  KVM capability checklist demo
  src/part-14-capabilities/drop_capabilities.c      Linux capability dropping demo
```

## Requirements

These demos are Linux-only:

- The container demo needs Linux namespaces and cgroups.
- The microVM demo needs Linux KVM and `/dev/kvm`.
- The hardening demo needs Linux `prctl`, seccomp, and `/proc`.
- The user namespace demo needs Linux user namespaces and writable `/proc/<pid>/{uid,gid}_map`.
- The capability dropping demo needs Linux capability syscalls and `/proc`.

On Debian or Ubuntu:

```sh
sudo apt-get install build-essential busybox-static
make
```

On macOS, `make` intentionally prints a clear Linux-only message.

## Run The Container Demo

Create a tiny BusyBox root filesystem:

```sh
make rootfs
```

Run it:

```sh
sudo ./mini-container ./rootfs /bin/sh
```

Inside the container:

```sh
hostname
echo $$
ps
mount
```

Run the hardening follow-up:

```sh
make run-part-11-hardening
```

Run the user namespace mapping follow-up:

```sh
make run-part-12-userns
```

Run the capability dropping follow-up:

```sh
make run-part-14-capabilities
```

## Run The MicroVM Demo

Check KVM:

```sh
ls -l /dev/kvm
```

Run it:

```sh
./mini-firecracker
```

Run the KVM memory-slot follow-up:

```sh
make run-part-10-microvm-memory
```

Run the KVM capability checklist:

```sh
make run-part-13-kvm-capabilities
```

Expected output:

```text
host VMM: starting vCPU
hello from a tiny microVM
host VMM: guest halted
```

If `/dev/kvm` is not accessible:

```sh
sudo ./mini-firecracker
```

or add your user to the `kvm` group.


## Mini Kernel Course

The `/kernel/` page explains the `mini-kernel/` project as operating-system
learning material: what the boot sector binary does, why stage2 exists, how the
freestanding kernel is organized, how the tiny syscall-like dispatcher is
implemented and tested, and how the x86_64 GDT/IDT descriptor builders are
validated and installed by early x86_64 setup.

Status note: the working bootable path today is the raw BIOS image:
`boot/bootsector.S` at LBA 0 loads `boot/stage2.S` from LBA 1 and runs the
stage2 text monitor. The freestanding C kernel, syscall entry, filesystem,
userspace-shaped init path, and early GDT/IDT descriptor-table setup are
compiled and host-tested. `boot/multiboot2.S`,
`boot/linker.ld`, and `boot/limine.cfg` are scaffolding for a planned
Limine/Multiboot-style bootloader path, not the default working boot path yet.

Build the kernel lab locally:

```sh
cd mini-kernel
make all
```

When QEMU is installed:

```sh
make run-image
```

## What To Build Next

- Link the C kernel into the BIOS stage2 handoff, or keep BIOS as a monitor and add the Limine/Multiboot boot path explicitly.
- Add a real trap entry and hardware ring-3 transition for the syscall frame already tested on the host.
- Replace simulated init with a small user binary loader.
- Grow the Linux demos with veth networking, read-only mounts, and a minimal control API.
