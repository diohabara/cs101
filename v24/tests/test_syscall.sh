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

if echo "$output" | grep -qF "Syscall handler ready"; then
  echo "PASS: syscall handler initialized"
else
  echo "FAIL: expected 'Syscall handler ready' in serial output" >&2
  fail=1
fi

if echo "$output" | grep -qF "Hello from syscall!"; then
  echo "PASS: syscall write test passed"
else
  echo "FAIL: expected 'Hello from syscall!' in serial output" >&2
  fail=1
fi

if echo "$output" | grep -qF "Syscall test complete"; then
  echo "PASS: syscall returned successfully"
else
  echo "FAIL: expected 'Syscall test complete' in serial output" >&2
  fail=1
fi

if [[ $fail -gt 0 ]]; then
  echo "got: $output" >&2
  exit 1
fi
