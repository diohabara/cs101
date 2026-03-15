#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/ringbuf"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: ringbuf returned $rc, expected 0" >&2
  exit 1
fi

expected="enqueue: 1
enqueue: 2
enqueue: 3
enqueue: 4
ring full
dequeue: 1
dequeue: 2
enqueue: 5
enqueue: 6
dequeue: 3
dequeue: 4
dequeue: 5
dequeue: 6"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: ringbuf wrap-around output correct"
else
  echo "FAIL: ringbuf output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
