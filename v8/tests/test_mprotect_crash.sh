#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/mprotect_crash"
# mprotect_crash should die with SIGSEGV → exit code 139 (128 + 11)
"$PROG" && rc=$? || rc=$?
if [[ $rc -eq 139 ]]; then
  echo "PASS: mprotect_crash died with SIGSEGV (139)"
else
  echo "FAIL: mprotect_crash returned $rc, expected 139" >&2
  exit 1
fi
