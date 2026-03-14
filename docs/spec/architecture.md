# Architecture

## 目的

`cs101` は教材、編集、実行、観察を分断しない。本文中の段落、コード片、実行 trace を同じデータモデルに乗せ、読者が「読んだ箇所をそのまま書き換え、実行結果を観察する」ことを前提に設計する。

## 構成

- `apps/web`
  - React ベースの統合ワークベンチ
  - 章ナビゲーション、開閉できる読み物タブ、実験エディタ、観察ビュー
- `crates/*`
  - Rust 側の学習エンジン
  - 各領域のシナリオから `TraceResult` を返す
- `crates/wasm-bindings`
  - Web から呼ぶ薄い API レイヤ
  - `run_scenario(scenario, source)` を公開

## データフロー

1. 章を選ぶ
2. TS 側が章本文、初期コード、観察ポイントを表示する
3. ユーザーがコードを編集して `Run` を押す
4. Web は WASM の `run_scenario` を呼ぶ
5. Rust が `TraceResult` を返す
6. UI が timeline、register、memory、type、heap を描画する

## 初期方針

- 実機互換より教育上の観察可能性を優先する
- trace schema を共通化し、CPU / OS / 仮想化 / コンパイラで同じ UI を再利用する
- 同一の章を `初学者 / 実務 / 理論` の 3 視点を併記する形で説明する
