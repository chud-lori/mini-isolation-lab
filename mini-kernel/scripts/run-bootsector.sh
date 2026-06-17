#!/bin/sh
set -eu

bootsector="${1:-build/bootsector.bin}"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "qemu-system-x86_64 is required to run the boot sector" >&2
    exit 1
fi

exec qemu-system-x86_64 \
    -drive "format=raw,file=$bootsector" \
    -serial stdio \
    -display curses \
    -no-reboot

