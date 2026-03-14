#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/getpid"
"$PROG" && rc=$? || rc=$?
if [[ $rc -gt 0 ]]; then
  echo "PASS: getpid returned $rc (PID > 0)"
else
  echo "FAIL: getpid returned $rc, expected > 0" >&2
  exit 1
fi
