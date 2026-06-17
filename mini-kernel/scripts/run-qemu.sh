#!/bin/sh
set -eu

kernel="${1:-build/mini-kernel.elf}"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 is required to run the kernel" >&2
    exit 1
fi

exec qemu-system-x86_64 \
    -kernel "$kernel" \
    -serial stdio \
    -display none \
    -no-reboot \
    -no-shutdown

