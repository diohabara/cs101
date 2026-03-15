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

if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: boot message"
else
  echo "FAIL: expected 'Hello, bare metal!'" >&2
  fail=1
fi

if echo "$output" | grep -qF "Phase 2 init complete"; then
  echo "PASS: Phase 2 initialization complete"
else
  echo "FAIL: expected 'Phase 2 init complete'" >&2
  fail=1
fi

if [[ $fail -gt 0 ]]; then
  echo "got: $output" >&2
  exit 1
fi
