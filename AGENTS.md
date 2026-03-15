# AGENTS.md

このリポジトリで作業する AI エージェント向けの操作マニュアル。

## Commands

ビルド・テスト:

```bash
just test v1            # 特定段階のテスト（Docker コンテナ内で実行）
just test-local v1      # ホスト上でのテスト（cargo test を直接実行、高速）
just test-cpu           # v1-v8 を順番にテスト
just test-all           # 全テスト実行
cargo test --manifest-path v1/Cargo.toml  # Cargo 直接実行
```

Web ビューア:

```bash
just docs               # Vite dev server を起動
just docs-build         # ビルド確認
npm run build:wasm      # WASM バインディングの再生成
```

コンテナ:

```bash
just shell              # linux/amd64 コンテナに入る
just docker-build       # コンテナイメージの明示ビルド
```

## Structure

- `v1/` ~ `v8/` — CPU 基礎編（実装済み）。各ディレクトリは x86-64 アセンブリプロジェクト
  - `asm/*.asm` — NASM アセンブリソース
  - `tests/test_*.sh` — シェルベースのテスト（exit code / stdout / stderr 検証）
  - `tests/run_all.sh` — テスト集計スクリプト
  - `gdb/*.gdb` — GDB トレーススクリプト（学習用）
  - `book.md` — Web ビューア用の解説書コンテンツ
  - `docs/` — delta などの補助メタデータ
  - `Makefile` — NASM + ld によるビルド
- `v9/` ~ `v55/` — 計画中の段階（README.md のみ）
- `crates/emu-core/` — 共通データ構造（`TraceEvent`, `TraceResult`, `KeyValue`）
- `crates/os-lab/` — OS カリキュラム実装
- `crates/hypervisor-lab/` — 仮想化カリキュラム実装
- `crates/compiler-lab/` — コンパイラカリキュラム実装
- `crates/wasm-bindings/` — WebAssembly FFI（Rust → ブラウザ橋渡し）
- `apps/web/` — React + TypeScript + Vite の解説書ビューア
- `docs/` — プロジェクトレベルのドキュメント
- `scripts/container/` — Docker コンテナ操作スクリプト

## Workflow

1. 新しい段階 `vN` を実装する場合、前段 `v(N-1)` をコピーしてから差分を加える
2. `asm/*.asm` に NASM アセンブリを書き、`tests/test_*.sh` にテストを追加する
3. `book.md` と `docs/` 配下のメタデータを更新する
4. `just test vN` でコンテナ内テストが通ることを確認する

## Rules

### コード規約

- v1-v8 は x86-64 アセンブリ（NASM Intel 構文）で実装
- テストは `tests/test_*.sh` にシェルスクリプトで書く。exit code、stdout/stderr の出力を検証する
- v5 以降は stdout/stderr キャプチャテストも使用する
- 初学者向けの教材では、`section .text`、`global _start`、ラベル、syscall ABI などの非自明な構文や慣習を省略しない。やや説明過多でも、本文またはコードコメントで意味を明示する

### ファイル規約

- 各 `vN/` は独立したアセンブリプロジェクト。Makefile で asm/*.asm を自動検出する
- `book.md` は Markdown で書く。Web ビューアがレンダリングする
- Markdown 内の図は `.drawio.svg` 形式で作成し、`apps/web/public/images/vN/` に配置して `![alt](/images/vN/name.drawio.svg)` で参照する。ソースは `vN/docs/` にも残す。Mermaid は使用しない
- パフォーマンス計測結果などのグラフ・チャートは Python + plotly で可視化し、PNG 形式で出力して画像リンクとして埋め込む
- `book.md` を学習者向け本文の正本とし、`docs/` 配下は `delta.md` などの補助メタデータを置く

### 禁止事項

- `v9/` ~ `v55/` の既存 README.md を削除しない（計画の記録として残す）
- `apps/web/src/generated/` 配下を手動編集しない（`npm run build:wasm` で自動生成される）
- テストなしのコード変更をコミットしない

### コンテナ

- ホストが macOS arm64 でも、低レイヤ実験は `linux/amd64` コンテナ内で行う
- コンテナイメージは `rust:1.93-bookworm` ベース。QEMU, NASM, GDB 等がインストール済み

## Commit and PR

- コミットメッセージは日本語または英語。変更内容を簡潔に書く
- 1 コミットで 1 つの論理的変更にまとめる
- PR を作る場合、影響する `vN` の範囲とテスト結果を明記する
