#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/sched_prio"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: sched_prio returned $rc, expected 0" >&2
  exit 1
fi

expected="HIGH:1
HIGH:2
MED:1
MED:2
LOW:1
LOW:2"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: sched_prio output and exit code correct"
else
  echo "FAIL: sched_prio output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
