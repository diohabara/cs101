# Content Model

## Chapter

各章は以下を持つ。

- `id`
- `title`
- `subtitle`
- `scenario`
- `starterCode`
- `observationChecklist`
- `sections`

## Section

各 section は教材と trace を結びつける最小単位。

- `id`
- `kind`: `read | write | watch | break`
- `title`
- `copy.beginner`
- `copy.practitioner`
- `copy.theory`
- `anchorEventId`

`copy.*` は UI 上では切り替えずに併記し、1 つの section の中に `初学者 / 実務 / 理論` の 3 視点を同居させる。

`anchorEventId` は timeline 側のイベントと紐付き、本文を読む位置と観察位置を同期する。

## TraceResult

Rust から返す観察データ。

- `summary`
- `diagnostics[]`
- `observations[]`
- `timeline[]`

各 timeline event は以下を持つ。

- `id`
- `label`
- `detail`
- `focus`
- `registers[]`
- `memory[]`
- `types[]`
- `heap[]`
