#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/sched_rr"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: sched_rr returned $rc, expected 0" >&2
  exit 1
fi

expected="A:1
B:1
C:1
A:2
B:2
C:2
A:3
B:3
C:3"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: sched_rr output and exit code correct"
else
  echo "FAIL: sched_rr output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
