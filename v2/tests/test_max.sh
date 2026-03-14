#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/max"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 27 ]]; then
  echo "PASS: max returned 27"
else
  echo "FAIL: max returned $rc, expected 27" >&2
  exit 1
fi
