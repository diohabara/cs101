#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/mmap_write"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 42 ]]; then
  echo "PASS: mmap_write returned 42"
else
  echo "FAIL: mmap_write returned $rc, expected 42" >&2
  exit 1
fi
