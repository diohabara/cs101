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

fail=0

if echo "$output" | grep -qF "IDT loaded: 256 entries"; then
  echo "PASS: IDT loaded"
else
  echo "FAIL: expected 'IDT loaded'" >&2
  fail=1
fi

if echo "$output" | grep -qF "Exception: #BP"; then
  echo "PASS: breakpoint exception handled"
else
  echo "FAIL: expected 'Exception: #BP'" >&2
  fail=1
fi

if [[ $fail -gt 0 ]]; then
  echo "got: $output" >&2
fi

exit $fail
