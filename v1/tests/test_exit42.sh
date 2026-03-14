#!/usr/bin/env bash
set -euo pipefail

PROG="$(dirname "$0")/../build/exit42"

"$PROG" && rc=$? || rc=$?

if [[ $rc -eq 42 ]]; then
  echo "PASS: exit42 returned 42"
else
  echo "FAIL: exit42 returned $rc, expected 42" >&2
  exit 1
fi
