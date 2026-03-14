# cs101

CPU の仕組みを、段階的な実装・テスト・構造化ログ・Web ビューアで体系的に学ぶリポジトリ。

## Quick Start

前提: [just](https://github.com/casey/just), [Docker](https://www.docker.com/), [Node.js](https://nodejs.org/) (v18+)

```bash
git clone https://github.com/diohabara/cs101
cd cs101
npm install

# 解説書ビューアを開く
just docs

# CPU 全段階のテスト
just test-cpu
```

## 方針

- 実装は `v1`, `v2`, ... の feature 単位で刻む
- 各 `vN` は前段に 1 つだけ重要な概念を足す
- テストは仕様書として書く
- trace / log を多めに出し、壊れた場所をログから追えるようにする

## Structure

```
├── v1/ ~ v8/          # CPU 基礎編（実装済み）
│   ├── src/lib.rs     #   Rust 実装
│   ├── tests/         #   振る舞いテスト
│   ├── book.md        #   解説書コンテンツ
│   └── docs/          #   delta / logging などの補助メタデータ
├── v9/ ~ v55/         # 今後の段階（計画中）
├── crates/
│   ├── emu-core/      # 共通データ構造（TraceEvent 等）
│   ├── os-lab/        # OS カリキュラム
│   ├── hypervisor-lab/# 仮想化カリキュラム
│   ├── compiler-lab/  # コンパイラカリキュラム
│   └── wasm-bindings/ # WebAssembly FFI レイヤー
├── apps/web/          # React + TypeScript 解説書ビューア
├── docs/              # プロジェクトドキュメント
├── scripts/           # コンテナ用ヘルパースクリプト
├── Justfile           # タスクランナー
├── Dockerfile         # linux/amd64 ビルド環境
└── compose.yaml       # Docker Compose 設定
```

## CPU 基礎編 (v1-v8)

| 段階 | テーマ |
|------|--------|
| v1 | CPU 状態モデルとトレース基盤 (`hlt`) |
| v2 | データ移動 (`mov`) |
| v3 | 算術演算 (`add`) |
| v4 | 減算 (`sub`) |
| v5 | 比較と条件分岐 (`cmp`, branch) |
| v6 | メモリと load/store |
| v7 | スタックと call/ret |
| v8 | ページングと仮想メモリ |

## コマンド

```bash
just docs           # 解説書ビューアを起動
just docs-build     # ビューアのビルド確認
just shell          # linux/amd64 コンテナに入る
just test v1        # 特定段階のテスト（コンテナ内）
just test-local v1  # ホスト上でのテスト（高速）
just test-cpu       # CPU 全段階テスト
just test-all       # 全テスト実行
```

## ドキュメント

- [Version Map](docs/version-map.md) — 全段階の地図
- [Roadmap](docs/roadmap.md) — 開発ロードマップ
- [Testing Strategy](docs/testing-strategy.md) — テスト戦略
- [Logging Strategy](docs/logging-strategy.md) — ログ戦略
- [Container Guide](docs/container.md) — コンテナ経由のテスト方法
- [Mastery Goals](docs/mastery-goals.md) — 習得目標

## License

MIT
