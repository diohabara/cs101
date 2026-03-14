#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/countdown"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 0 ]]; then
  echo "PASS: countdown returned 0"
else
  echo "FAIL: countdown returned $rc, expected 0" >&2
  exit 1
fi
