#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/bitops"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 4 ]]; then
  echo "FAIL: bitops returned $rc, expected 4" >&2
  exit 1
fi
expected="set CF,ZF:  flags = 0x03
CF is set
SF is clear
clear ZF:   flags = 0x01
toggle CF,SF: flags = 0x04"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: bitops output and exit code correct"
else
  echo "FAIL: bitops output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
