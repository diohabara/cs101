#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/arithmetic"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 13 ]]; then
  echo "PASS: arithmetic returned 13"
else
  echo "FAIL: arithmetic returned $rc, expected 13" >&2
  exit 1
fi
