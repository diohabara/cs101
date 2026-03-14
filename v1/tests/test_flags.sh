#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/flags"
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 0 ]]; then
  echo "PASS: flags returned 0 (ZF set)"
else
  echo "FAIL: flags returned $rc, expected 0" >&2
  exit 1
fi
