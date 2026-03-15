#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/minifs"

# stdout + exit code をキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: minifs returned $rc, expected 0" >&2
  exit 1
fi

expected="created: hello.txt
wrote 13 bytes to hello.txt
read: Hello, World!
created: test.txt
hello.txt  test.txt"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: minifs output correct"
else
  echo "FAIL: minifs output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
