#!/bin/sh
set -eu

rootfs=${1:-rootfs}
busybox=${BUSYBOX:-/bin/busybox}

if [ ! -x "$busybox" ]; then
    echo "missing BusyBox binary: $busybox" >&2
    echo "install busybox-static or set BUSYBOX=/path/to/busybox" >&2
    exit 1
fi

mkdir -p "$rootfs/bin" "$rootfs/proc" "$rootfs/tmp"
cp "$busybox" "$rootfs/bin/busybox"

for applet in sh ps hostname mount ip ls cat echo pwd; do
    ln -sf busybox "$rootfs/bin/$applet"
done

echo "created BusyBox rootfs at $rootfs"
echo "try: sudo ./mini-container ./$rootfs /bin/sh"
