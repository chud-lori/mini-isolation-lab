# Mini Isolation Lab

Beginner-friendly demos for learning how Linux containers and Firecracker-style
microVMs work under the hood.

This repository contains two small C programs:

- `mini-container`: uses Linux namespaces, `chroot`, `/proc`, and cgroups.
- `mini-firecracker`: uses Linux KVM to run a tiny guest program in a microVM-like VM.

The tutorial page is `docs/index.html`, ready for GitHub Pages.

## Quick Map

```text
mini-isolation-lab/
  docs/index.html                          GitHub Pages tutorial
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

## Publish As GitHub Pages

Push this directory as a repository, then in GitHub:

1. Open repository settings.
2. Go to Pages.
3. Choose Deploy from a branch.
4. Select your branch and `/docs`.
5. Save.

GitHub will serve `docs/index.html`.
