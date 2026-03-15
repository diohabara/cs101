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

if echo "$output" | grep -qF "GDT loaded: selectors 0x08(code) 0x10(data)"; then
  echo "PASS: GDT loaded successfully"
else
  echo "FAIL: expected GDT loaded message" >&2
  echo "got: $output" >&2
  exit 1
fi
