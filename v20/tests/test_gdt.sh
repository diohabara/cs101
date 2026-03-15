#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をファイルにキャプチャ
tmpfile="$(mktemp)"
trap 'rm -f "$tmpfile"' EXIT
timeout 5 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial "file:$tmpfile" \
  -display none \
  -no-reboot \
  2>/dev/null || true

output="$(cat "$tmpfile")"

if echo "$output" | grep -qF "GDT loaded: selectors 0x08(code) 0x10(data)"; then
  echo "PASS: GDT loaded"
else
  echo "FAIL: expected 'GDT loaded'" >&2
  echo "got: $output" >&2
  exit 1
fi
