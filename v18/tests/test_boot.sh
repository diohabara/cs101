#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

# QEMU でカーネルを起動し、シリアル出力をキャプチャ
output="$(timeout 5 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial stdio \
  -display none \
  -no-reboot \
  -nographic \
  2>/dev/null || true)"

if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: boot output 'Hello, bare metal!'"
else
  echo "FAIL: expected 'Hello, bare metal!' in serial output" >&2
  echo "got: $output" >&2
  exit 1
fi
