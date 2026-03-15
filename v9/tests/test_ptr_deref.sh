#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/ptr_deref"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 42 ]]; then
  echo "PASS: ptr_deref returned 42"
else
  echo "FAIL: ptr_deref returned $rc, expected 42" >&2
  exit 1
fi
