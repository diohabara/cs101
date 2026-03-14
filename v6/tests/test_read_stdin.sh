#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/read_stdin"
output=$(echo "hello" | "$PROG")
if echo "$output" | grep -q "hello"; then
  echo "PASS: read_stdin echoed back input"
else
  echo "FAIL: read_stdin output was '$output', expected 'hello'" >&2
  exit 1
fi
