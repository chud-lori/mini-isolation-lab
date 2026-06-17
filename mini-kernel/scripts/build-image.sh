#!/bin/sh
set -eu

bootsector="${BOOTSECTOR_BIN:-build/bootsector.bin}"
stage2="${1:-build/stage2.bin}"
image="${2:-build/mini-kernel.img}"

echo "assembling raw BIOS image: LBA0 boot sector, LBA1+ stage2"

if [ ! -f "$bootsector" ]; then
    echo "missing boot sector: $bootsector" >&2
    exit 1
fi

if [ ! -f "$stage2" ]; then
    echo "missing stage2 image: $stage2" >&2
    exit 1
fi

bootsector_size="$(wc -c < "$bootsector" | tr -d ' ')"
if [ "$bootsector_size" != "512" ]; then
    echo "boot sector must be exactly 512 bytes: $bootsector" >&2
    exit 1
fi

if ! tail -c 2 "$bootsector" | od -An -tx1 | tr -s ' ' | grep -q '55 aa'; then
    echo "boot sector signature missing: $bootsector" >&2
    exit 1
fi

stage2_size="$(wc -c < "$stage2" | tr -d ' ')"
if [ "$stage2_size" = "0" ]; then
    echo "stage2 image is empty: $stage2" >&2
    exit 1
fi

mkdir -p "$(dirname "$image")"
tmp="${image}.tmp.$$"
trap 'rm -f "$tmp"' EXIT HUP INT TERM

cp "$bootsector" "$tmp"
cat "$stage2" >> "$tmp"

image_size="$(wc -c < "$tmp" | tr -d ' ')"
remainder=$((image_size % 512))
if [ "$remainder" -ne 0 ]; then
    pad=$((512 - remainder))
    dd if=/dev/zero bs=1 count="$pad" >> "$tmp" 2>/dev/null
fi

mv "$tmp" "$image"
trap - EXIT HUP INT TERM

echo "wrote $image"
