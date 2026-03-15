#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/dma_sim"

# stdout + exit code を同時にキャプチャ
output="$("$PROG")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: dma_sim returned $rc, expected 0" >&2
  exit 1
fi

expected="DMA descriptor submitted
DMA transfer complete
data verified: Hello, DMA!"

if [[ "$output" == "$expected" ]]; then
  echo "PASS: dma_sim DMA transfer output correct"
else
  echo "FAIL: dma_sim output mismatch" >&2
  echo "expected:"
  echo "$expected"
  echo "got:"
  echo "$output"
  exit 1
fi
