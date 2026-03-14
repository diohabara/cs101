#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/hello"
output=$("$PROG")
if echo "$output" | grep -q "Hello, world!"; then
  echo "PASS: hello printed 'Hello, world!'"
else
  echo "FAIL: hello output was '$output', expected 'Hello, world!'" >&2
  exit 1
fi
