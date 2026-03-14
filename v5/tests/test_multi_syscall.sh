#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/multi_syscall"
output=$("$PROG") && rc=$? || rc=$?
if [[ $rc -eq 7 ]] && [[ "$output" == "7" ]]; then
  echo "PASS: multi_syscall printed '7' and exited with 7"
else
  echo "FAIL: multi_syscall rc=$rc, output='$output'" >&2
  exit 1
fi
