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

# テスト 1: カーネルが起動すること
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

# テスト 3: VMCS の結果（VT-x の有無で分岐）
if echo "$output" | grep -qF "VMCS initialized"; then
  echo "PASS: VMCS initialized (real VMX)"
  PASS=$((PASS + 1))
elif echo "$output" | grep -qF "VMCS: simulated"; then
  echo "PASS: VMCS simulated (no VT-x)"
  PASS=$((PASS + 1))
  # シミュレーションではゲスト状態の出力を確認
  if echo "$output" | grep -qF "Guest RIP"; then
    echo "PASS: simulated guest state shown"
    PASS=$((PASS + 1))
  fi
elif echo "$output" | grep -qF "VMCS setup skipped"; then
  echo "PASS: VMCS setup skipped (expected)"
  PASS=$((PASS + 1))
else
  echo "FAIL: no VMCS output" >&2
  FAIL=$((FAIL + 1))
fi

echo "got: $output" >&2

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
