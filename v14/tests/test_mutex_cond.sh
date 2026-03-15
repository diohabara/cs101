#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/mutex_cond"

output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: mutex_cond returned $rc, expected 0" >&2
  exit 1
fi

expected_consumed="consumed: 0 1 2 3 4 5 6 7 8 9"
expected_result="all values received"

if echo "$output" | grep -q "$expected_result"; then
  echo "PASS: mutex_cond output correct"
  echo "$output"
else
  echo "FAIL: mutex_cond output mismatch" >&2
  echo "expected '$expected_result' in output"
  echo "got:"
  echo "$output"
  exit 1
fi
