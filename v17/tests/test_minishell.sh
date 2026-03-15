#!/usr/bin/env bash
set -euo pipefail
PROG="$(dirname "$0")/../build/minishell"
TMPFILE="/tmp/test_minishell_out_$$.txt"

# テスト後に一時ファイルを削除
cleanup() { rm -f "$TMPFILE"; }
trap cleanup EXIT

FAIL=0

# contains_line: 出力に指定行が含まれるか確認する
contains_line() {
  local output="$1"
  local line="$2"
  echo "$output" | grep -qF "$line"
}

# --- テスト 1: 単純実行 ---
output="$("$PROG" "echo hello")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: simple exec returned $rc, expected 0" >&2
  FAIL=1
elif contains_line "$output" "exec: echo hello" && contains_line "$output" "hello"; then
  echo "PASS: simple exec"
else
  echo "FAIL: simple exec output missing expected lines" >&2
  echo "got:"
  echo "$output"
  FAIL=1
fi

# --- テスト 2: パイプ ---
output="$("$PROG" "echo hello | cat")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: pipe returned $rc, expected 0" >&2
  FAIL=1
elif contains_line "$output" "pipe: echo hello | cat" && contains_line "$output" "hello"; then
  echo "PASS: pipe"
else
  echo "FAIL: pipe output missing expected lines" >&2
  echo "got:"
  echo "$output"
  FAIL=1
fi

# --- テスト 3: リダイレクト ---
output="$("$PROG" "echo redirected > $TMPFILE")" && rc=$? || rc=$?

if [[ $rc -ne 0 ]]; then
  echo "FAIL: redirect returned $rc, expected 0" >&2
  FAIL=1
elif contains_line "$output" "redirect: echo redirected > $TMPFILE"; then
  # ファイルの中身を検証
  file_content="$(cat "$TMPFILE")"
  if [[ "$file_content" == "redirected" ]]; then
    echo "PASS: redirect"
  else
    echo "FAIL: redirect file content mismatch" >&2
    echo "expected: redirected"
    echo "got: $file_content"
    FAIL=1
  fi
else
  echo "FAIL: redirect output mismatch" >&2
  echo "expected line: redirect: echo redirected > $TMPFILE"
  echo "got:"
  echo "$output"
  FAIL=1
fi

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
