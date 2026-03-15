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

if echo "$output" | grep -qF "2 tasks registered"; then
  echo "PASS: tasks registered"
else
  echo "FAIL: expected '2 tasks registered'" >&2
  fail=1
fi

if echo "$output" | grep -qF "Alpha working"; then
  echo "PASS: Alpha task executed"
else
  echo "FAIL: expected 'Alpha working'" >&2
  fail=1
fi

if echo "$output" | grep -qF "Beta working"; then
  echo "PASS: Beta task executed"
else
  echo "FAIL: expected 'Beta working'" >&2
  fail=1
fi

if echo "$output" | grep -qF "[Alpha] completed"; then
  echo "PASS: Alpha task completed"
else
  echo "FAIL: expected '[Alpha] completed'" >&2
  fail=1
fi

if echo "$output" | grep -qF "[Beta] completed"; then
  echo "PASS: Beta task completed"
else
  echo "FAIL: expected '[Beta] completed'" >&2
  fail=1
fi

if echo "$output" | grep -qF "all tasks done"; then
  echo "PASS: all tasks done"
else
  echo "FAIL: expected 'all tasks done'" >&2
  fail=1
fi

if echo "$output" | grep -qF "System halted"; then
  echo "PASS: system halted"
else
  echo "FAIL: expected 'System halted'" >&2
  fail=1
fi

if [[ $fail -gt 0 ]]; then
  echo "got: $output" >&2
  exit 1
fi
