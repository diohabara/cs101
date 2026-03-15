#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/spinlock"

output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: spinlock returned $rc, expected 0" >&2
  exit 1
fi

# "with lock: 200000" が含まれることを検証
# "without lock" の結果は非決定的なので検証しない
if echo "$output" | grep -q "with lock: 200000"; then
  echo "PASS: spinlock output correct"
  echo "$output"
else
  echo "FAIL: spinlock output mismatch" >&2
  echo "expected 'with lock: 200000' in output"
  echo "got:"
  echo "$output"
  exit 1
fi
