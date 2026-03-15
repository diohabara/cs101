#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/pcb"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: pcb returned $rc, expected 0" >&2
  exit 1
fi

expected="PID 1: READY -> RUNNING
PID 1: RUNNING -> BLOCKED
PID 1: BLOCKED -> RUNNING
PID 1: RUNNING -> TERMINATED"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: pcb state transitions correct"
else
  echo "FAIL: pcb output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
