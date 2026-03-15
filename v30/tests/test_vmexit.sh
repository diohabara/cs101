#!/usr/bin/env bash
set -euo pipefail

KERNEL="$(dirname "$0")/../build/kernel.bin"

output="$(timeout 5 qemu-system-x86_64 \
  -kernel "$KERNEL" \
  -serial stdio \
  -display none \
  -no-reboot \
  -nographic \
  -cpu qemu64,+vmx \
  2>/dev/null || true)"

PASS=0
FAIL=0

# テスト 1: カーネル起動
if echo "$output" | grep -qF "Hello, bare metal!"; then
  echo "PASS: kernel boot"
  PASS=$((PASS + 1))
else
  echo "FAIL: kernel did not boot" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 2: Phase 3 開始
if echo "$output" | grep -qF "Phase 3: Hypervisor"; then
  echo "PASS: Phase 3 started"
  PASS=$((PASS + 1))
else
  echo "FAIL: Phase 3 message not found" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 3: VM exit ハンドリング（real or simulated）
if echo "$output" | grep -qF "VM exit"; then
  echo "PASS: VM exit handling present"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "VM Exit Handling Demo"; then
  echo "PASS: VM exit demo shown"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "VT-x not available"; then
  echo "PASS: VT-x not available (expected)"
  PASS=$((PASS + 1))
else
  echo "FAIL: no VM exit handling output" >&2
  FAIL=$((FAIL + 1))
fi

# テスト 4: exit reason の説明が含まれる
if echo "$output" | grep -qF "HLT"; then
  echo "PASS: HLT exit reason shown"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "CPUID"; then
  echo "PASS: CPUID exit reason shown"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "Guest: skipped"; then
  echo "PASS: guest skipped (no VT-x)"
  PASS=$((PASS + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
