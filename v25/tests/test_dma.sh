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

fail=0

if echo "$output" | grep -qF "DMA initialized"; then
  echo "PASS: DMA initialized"
else
  echo "FAIL: expected 'DMA initialized' in serial output" >&2
  fail=1
fi

if echo "$output" | grep -qF "DMA transfer complete"; then
  echo "PASS: DMA transfer completed"
else
  echo "FAIL: expected 'DMA transfer complete' in serial output" >&2
  fail=1
fi

if echo "$output" | grep -qF "DMA data verified"; then
  echo "PASS: DMA data verified"
else
  echo "FAIL: expected 'DMA data verified' in serial output" >&2
  fail=1
fi

if [[ $fail -gt 0 ]]; then
  echo "got: $output" >&2
  exit 1
fi
