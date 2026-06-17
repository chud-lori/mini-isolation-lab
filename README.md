# Mini Isolation Lab

Beginner-friendly demos for learning how Linux containers and Firecracker-style
microVMs work under the hood.

This repository contains two small C programs and one tiny OS/kernel learning project:

- `mini-container`: uses Linux namespaces, `chroot`, `/proc`, and cgroups.
- `mini-firecracker`: uses Linux KVM to run a tiny guest program in a microVM-like VM.
- `mini-kernel`: builds a two-stage BIOS image and a freestanding kernel scaffold used by the `/kernel` course page.

The tutorial page is `docs/index.html`, ready for GitHub Pages. The kernel course page is `docs/kernel/index.html`, published at `/kernel/`.

## Quick Map

```text
mini-isolation-lab/
  docs/index.html                          GitHub Pages tutorial
  docs/kernel/index.html                   OS/kernel course page
  mini-kernel/                             tiny OS and freestanding kernel scaffold
  Makefile                                 builds the demos on Linux
  src/part-01-syscalls/syscall_write.c     first syscall example
  src/part-07-container/mini_container.c   container demo
  src/part-09-microvm/mini_firecracker.c   microVM/KVM demo
```

## Requirements

These demos are Linux-only:

- The container demo needs Linux namespaces and cgroups.
- The microVM demo needs Linux KVM and `/dev/kvm`.

On Debian or Ubuntu:

```sh
sudo apt-get install build-essential busybox-static
make
```

On macOS, `make` intentionally prints a clear Linux-only message.

## Run The Container Demo

Create a tiny BusyBox root filesystem:

```sh
mkdir -p rootfs/bin
cp /bin/busybox rootfs/bin/
ln -s busybox rootfs/bin/sh
ln -s busybox rootfs/bin/ps
ln -s busybox rootfs/bin/hostname
ln -s busybox rootfs/bin/mount
ln -s busybox rootfs/bin/ip
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

## Run The MicroVM Demo

Check KVM:

```sh
ls -l /dev/kvm
```

Run it:

```sh
./mini-firecracker
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
freestanding kernel is organized, why the first implementation uses C and
assembly instead of Rust or Zig, and how the project can grow toward a small
POSIX-compatible syscall and libc surface.

Build the kernel lab locally:

```sh
cd mini-kernel
make all
```

When QEMU is installed:

```sh
make run-image
```
