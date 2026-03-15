#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をファイルにキャプチャ
# -serial file: でシリアル出力をファイルに書き出す（stdio との競合を回避）
tmpfile="$(mktemp)"
trap 'rm -f "$tmpfile"' EXIT
timeout 5 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial "file:$tmpfile" \
  -display none \
  -no-reboot \
  2>/dev/null || true

output="$(cat "$tmpfile")"

if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: boot output 'Hello, bare metal!'"
else
  echo "FAIL: expected 'Hello, bare metal!' in serial output" >&2
  echo "got: $output" >&2
  exit 1
fi
