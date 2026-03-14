#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/execve_echo"
output=$("$PROG") && rc=$? || rc=$?
if [[ $rc -eq 0 ]] && echo "$output" | grep -q "hello from execve"; then
  echo "PASS: execve_echo printed 'hello from execve' and exited 0"
else
  echo "FAIL: execve_echo rc=$rc, output='$output'" >&2
  exit 1
fi
