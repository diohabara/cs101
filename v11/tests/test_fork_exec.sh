#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/fork_exec"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: fork_exec returned $rc, expected 0" >&2
  exit 1
fi

# 子の exec 出力と親の出力の順序は不定なので、各行を独立に検証
fail=0

if ! echo "$output" | grep -qF "hello from child"; then
  echo "FAIL: missing 'hello from child' in output" >&2
  fail=1
fi

if ! echo "$output" | grep -qF "parent: child exited with 0"; then
  echo "FAIL: missing 'parent: child exited with 0' in output" >&2
  fail=1
fi

if [[ $fail -ne 0 ]]; then
  echo "got:"
  echo "$output"
  exit 1
fi

echo "PASS: fork_exec output correct"
