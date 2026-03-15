`v14` では、pthreads を使ってマルチスレッドプログラミングの基礎を学びます。
データ競合を実際に体験し、spinlock の自作と mutex による生産者-消費者パターンを実装します。

## 概要

現代の CPU は複数のコアを持ち、複数のスレッドが同時に実行されます。
しかし、複数のスレッドが同じ変数に同時にアクセスすると、予期しない結果が生じます。
これが「データ競合（data race）」です。

v14 では以下の 3 つを体験します:

1. **データ競合**: ロックなしで 2 スレッドが同じ変数をインクリメント → 結果が狂う
2. **spinlock**: `__atomic` ビルトインで自作したロックでデータ競合を防ぐ
3. **mutex + condition variable**: OS の支援を受けた効率的な同期で生産者-消費者パターンを実装

## データ競合とは

2 つのスレッドが同じ変数 `counter` を同時にインクリメントする場合を考えます。
`counter++` は C では 1 行ですが、CPU レベルでは 3 つの操作に分解されます:

```
スレッド A              スレッド B
─────────────          ─────────────
load counter → 0
                       load counter → 0
add 1 → 1
                       add 1 → 1
store 1 → counter
                       store 1 → counter

結果: counter = 1（期待値は 2）
```

スレッド A の store がスレッド B の load より先に完了すれば問題ありませんが、
タイミングによっては上のように「更新が失われる（lost update）」が発生します。

![データ競合: 2 スレッドが同時に読み書きして更新が失われる](/images/v14/data-race.drawio.svg)

## spinlock — __atomic ビルトインによる自作ロック

spinlock は最もシンプルなロック機構です。
ロックが取得できるまで CPU がビジーウェイト（空回り）し続けます。

### __atomic_exchange_n の仕組み

`__atomic_exchange_n(&locked, 1, __ATOMIC_ACQUIRE)` は以下を不可分（atomic）に行います:

1. `locked` の現在の値を読む
2. `locked` に `1` を書き込む
3. 読んだ値を返す

| locked の状態 | 返り値 | 意味 |
|--------------|--------|------|
| `0`（空き） | `0` | ロック取得成功、ループ終了 |
| `1`（使用中）| `1` | 他スレッドが保持中、ビジーウェイト継続 |

この操作が「不可分」であることが重要です。
通常の load + store では、2 スレッドが同時に「空き」を読んでしまう可能性がありますが、
atomic exchange ではハードウェアが排他性を保証します。

### メモリオーダリング

`__ATOMIC_ACQUIRE` と `__ATOMIC_RELEASE` は、コンパイラと CPU に対する命令の並び替え制約です:

- **ACQUIRE（ロック取得時）**: 「このロード以降のメモリアクセスを、ロック取得より前に動かすな」
- **RELEASE（ロック解放時）**: 「このストア以前のメモリアクセスを、ロック解放より後に動かすな」

```
spin_lock():
  [ACQUIRE barrier]
  ── ここから先のアクセスはロック内に留まる ──
  counter++     ← クリティカルセクション
  ── ここまでのアクセスはロック内に留まる ──
  [RELEASE barrier]
spin_unlock()
```

### sync.h — spinlock の実装

{{code:src/sync.h}}

### spinlock.c — データ競合の体験と排他制御

| フェーズ | 処理 | 期待される結果 |
|----------|------|---------------|
| 1. ロックなし | 2 スレッドが各 100000 回インクリメント | 200000 未満（データ競合） |
| 2. spinlock あり | 同じ処理を spin_lock/spin_unlock で保護 | 200000（正確） |

{{code:src/spinlock.c}}

## CAS（Compare-And-Swap）

spinlock で使った `__atomic_exchange_n` は CAS（Compare-And-Swap）の一種です。
より汎用的な CAS は `__atomic_compare_exchange_n` で、以下のように動作します:

```c
// 擬似コード: CAS の意味
bool cas(int *ptr, int *expected, int desired) {
    if (*ptr == *expected) {
        *ptr = desired;   // 期待通りなら更新
        return true;
    } else {
        *expected = *ptr;  // 現在値を返す
        return false;
    }
}
```

CAS は lock-free データ構造の基本操作です。
ロックを使わずに、「読んだ値が変わっていなければ更新する」というパターンで安全な並行処理を実現します。

## mutex + condition variable — 生産者-消費者パターン

spinlock はシンプルですが、ロック待ちの間 CPU を消費し続けます。
`pthread_mutex` は OS カーネルの支援を受け、ロック待ちのスレッドをスリープさせます。

### spinlock vs mutex

| 特性 | spinlock | pthread_mutex |
|------|----------|---------------|
| 待機方法 | ビジーウェイト（CPU 空回り） | スリープ（OS がスケジュール） |
| レイテンシ | 低い（コンテキストスイッチなし） | やや高い（カーネル介入） |
| CPU 使用量 | 高い（待機中も 100%） | 低い（待機中は 0%） |
| 適切な場面 | クリティカルセクションが短い場合 | 長時間の待機がある場合 |

### condition variable

条件変数（condition variable）は「ある条件が満たされるまで待つ」ための仕組みです:

- `pthread_cond_wait(&cond, &mtx)` — 条件が通知されるまでスリープ（mutex を自動的に解放）
- `pthread_cond_signal(&cond)` — 待機中のスレッド 1 つを起床

### 生産者-消費者パターン

有界バッファ（サイズ 4）を介して、生産者と消費者が協調動作します:

```
生産者                    有界バッファ (サイズ 4)         消費者
─────────                ┌───┬───┬───┬───┐          ─────────
値 0 を投入 ───────────→  │ 0 │   │   │   │
値 1 を投入 ───────────→  │ 0 │ 1 │   │   │
値 2 を投入 ───────────→  │ 0 │ 1 │ 2 │   │
値 3 を投入 ───────────→  │ 0 │ 1 │ 2 │ 3 │  ← 満杯！生産者は待機
                         │   │ 1 │ 2 │ 3 │  ←─────── 値 0 を取得
値 4 を投入 ───────────→  │ 4 │   │ 2 │ 3 │  ← 空きができた
                         ...
```

- バッファが満杯 → 生産者が `not_full` を待つ
- バッファが空 → 消費者が `not_empty` を待つ
- 投入/取得後に相手側の条件変数を signal

### mutex_cond.c — 生産者-消費者の実装

{{code:src/mutex_cond.c}}

## PlayStation との関連

- **spinlock**: PS4/PS5 のカーネル内部では、割り込みハンドラなどコンテキストスイッチが許されない場面で spinlock が使われます。割り込み禁止 + spinlock の組み合わせは OS カーネルの定番パターンです
- **lock-free**: ゲームエンジンのジョブシステムは lock-free キューでタスクを分配します。ロックの競合によるスレッドのストールを避け、全コアを効率的に使い切ることが 60fps 維持の鍵です
- **生産者-消費者**: GPU コマンドバッファの submit は典型的な生産者-消費者パターンです。CPU（生産者）がコマンドリストを構築し、GPU（消費者）が非同期に実行します

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [GCC __atomic Builtins](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html) — `__atomic_exchange_n`, `__atomic_store_n`, `__atomic_compare_exchange_n` の仕様
- [POSIX Threads (IEEE Std 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html) — pthread_create, pthread_mutex, pthread_cond の仕様
- [C11 Standard §7.17 (Atomics)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) — C11 のアトミック操作の標準定義
- [Herb Sutter, "atomic<> Weapons" (C++ and Beyond 2012)](https://herbsutter.com/2013/02/11/atomic-weapons-the-c-memory-model-and-modern-hardware/) — メモリオーダリングの実践的解説
