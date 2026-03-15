`v26` は Phase 2（ベアメタルカーネル編）の最終段階です。v18-v25 で構築した全コンポーネントを統合し、協調的マルチタスクカーネルを実装します。

## 概要

v26 では複数のタスクを登録し、ラウンドロビン方式で順番に実行するミニ OS を完成させます。これまでに構築した以下のコンポーネントがすべて連携して動作します。

| 段階 | コンポーネント | 役割 |
|------|---------------|------|
| v18 | UART | シリアル出力 |
| v19 | GDT | セグメントと特権レベル |
| v20 | IDT + PIC | 割り込みハンドラ |
| v21 | PIT タイマー | 定期的な割り込み |
| v22 | PMM | 物理メモリ管理 |
| v23 | ページング | 仮想メモリ |
| v24 | syscall/sysret | システムコール機構 |
| v25 | DMA | メモリ間データ転送 |
| v26 | カーネル + タスク | 協調的マルチタスク |

![mini OS アーキテクチャ](/images/v26/minios-architecture.drawio.svg)

## 協調的マルチタスク vs プリエンプティブマルチタスク

### 協調的マルチタスク（v26 で実装）

タスクが自発的に制御を返すスケジューリング方式です。

```
タスク A が実行
  → タスク A が return（自発的に制御を返す）
  → スケジューラがタスク B を選択
  → タスク B が実行
  → タスク B が return
  → スケジューラがタスク A を選択
  → ...
```

利点:
- 実装が単純（スタック切り替え不要）
- コンテキストスイッチのオーバーヘッドが小さい

欠点:
- タスクが無限ループすると他のタスクが実行されない
- タスクの実行時間を制限できない

### プリエンプティブマルチタスク

タイマー割り込みで強制的にタスクを切り替える方式です。

```
タスク A が実行中...
  → タイマー割り込み発生（強制）
  → CPU がタスク A の状態を保存
  → スケジューラがタスク B を選択
  → CPU がタスク B の状態を復元
  → タスク B が実行中...
  → タイマー割り込み発生（強制）
  → ...
```

プリエンプティブ方式ではコンテキストスイッチ時にすべてのレジスタを保存・復元する必要があり、TSS やスタック切り替えが必須になります。v26 では概念を説明しますが、実装は協調的方式にとどめます。

## タスク制御ブロック (TCB)

各タスクの情報を保持する構造体です。

```
struct task {
    enum task_state state;    現在の状態 (READY/RUNNING/DONE)
    uint32_t id;              タスク ID
    const char *name;         タスク名
    void (*entry)(void);      エントリポイント関数
    int iterations;           残り実行回数
};
```

### タスク状態遷移

```
           kernel_add_task()
READY <──────────────────────── (登録)
  │
  │ schedule() がタスクを選択
  ▼
RUNNING ────────────────────── タスク関数を実行
  │
  │ タスク関数が return
  ▼
iterations > 0? ──yes──→ READY（次の実行まで待機）
  │
  no
  ▼
DONE ────────────────────── タスク完了
```

## ラウンドロビンスケジューリング

```
タスクテーブル: [Alpha, Beta]
current_task = -1 (初期状態)

ラウンド 1:
  schedule() → current_task = 0 (Alpha)
  Alpha を実行 → iterations: 3 → 2
  schedule() → current_task = 1 (Beta)
  Beta を実行 → iterations: 3 → 2

ラウンド 2:
  schedule() → current_task = 0 (Alpha)
  Alpha を実行 → iterations: 2 → 1
  schedule() → current_task = 1 (Beta)
  Beta を実行 → iterations: 2 → 1

ラウンド 3:
  schedule() → current_task = 0 (Alpha)
  Alpha を実行 → iterations: 1 → 0 → DONE
  schedule() → current_task = 1 (Beta)
  Beta を実行 → iterations: 1 → 0 → DONE

schedule() → -1 (実行可能タスクなし) → ループ終了
```

## 起動シーケンス（完全版）

v26 の `_start` から `kernel_main` 完了までの全体フローです。

```
_start:
  1. uart_init()        シリアルポート初期化
  2. gdt_init()         GDT ロード + セグメントレジスタ設定
  3. idt_init()         IDT ロード + PIC 再マッピング
  4. timer_init(100)    PIT: 100Hz 割り込み設定
  5. pmm_init()         物理メモリマネージャ初期化
  6. paging_init()      4レベルページテーブル構築 + CR3 設定
  7. usermode_init()    EFER.SCE + STAR/LSTAR/SFMASK 設定
  8. kernel_add_task()  タスク Alpha, Beta を登録
  9. kernel_main()      スケジューラ起動 → タスク交互実行
  10. hlt ループ        全タスク完了後にシステム停止
```

## 実装

### kernel.h --- カーネル定義

{{code:src/kernel.h}}

### kernel.c --- スケジューラ実装

{{code:src/kernel.c}}

### task.c --- サンプルタスク

{{code:src/task.c}}

### boot.c --- カーネルエントリポイント（v26）

{{code:src/boot.c}}

### uart.h / uart.c --- UART ドライバ（v18 から継続）

{{code:src/uart.h}}

{{code:src/uart.c}}

### gdt.h / gdt.c --- GDT（v19 から継続）

{{code:src/gdt.h}}

{{code:src/gdt.c}}

### idt.h / idt.c --- IDT（v20 から継続）

{{code:src/idt.h}}

{{code:src/idt.c}}

### timer.h / timer.c --- PIT タイマー（v21 から継続）

{{code:src/timer.h}}

{{code:src/timer.c}}

### pmm.h / pmm.c --- 物理メモリマネージャ（v22 から継続）

{{code:src/pmm.h}}

{{code:src/pmm.c}}

### paging.h / paging.c --- ページング（v23 から継続）

{{code:src/paging.h}}

{{code:src/paging.c}}

### usermode.h / usermode.c / syscall_handler.c --- syscall 機構（v24 から継続）

{{code:src/usermode.h}}

{{code:src/usermode.c}}

{{code:src/syscall_handler.c}}

### dma_hw.h / dma_hw.c --- DMA コントローラ（v25 から継続）

{{code:src/dma_hw.h}}

{{code:src/dma_hw.c}}

### linker.ld --- リンカスクリプト

{{code:src/linker.ld}}

## Phase 2 まとめ

v18 から v26 まで、9 段階にわたってベアメタルカーネルを段階的に構築しました。

```
Phase 2 の到達点:

  ハードウェア制御:
    - UART (シリアル通信)
    - PIT (タイマー割り込み)
    - PIC (割り込みコントローラ)

  CPU 機構:
    - GDT (セグメンテーション + 特権レベル)
    - IDT (割り込みハンドラ)
    - ページング (4レベルページテーブル)
    - syscall/sysret (高速システムコール)

  OS 基盤:
    - 物理メモリマネージャ (ビットマップアロケータ)
    - DMA デスクリプタリング
    - 協調的マルチタスク (ラウンドロビンスケジューラ)
```

これらはすべて、Linux や Windows など実際の OS カーネルが持つ基本機能と同じものです。

## 参考文献

- [Intel SDM Vol.3 Ch.8 "Task Management"](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) --- x86 のタスク管理機構
- [OSDev Wiki: Multitasking](https://wiki.osdev.org/Multitasking) --- 協調的/プリエンプティブマルチタスクの概要
- [OSDev Wiki: Context Switching](https://wiki.osdev.org/Context_Switching) --- コンテキストスイッチの実装方法
- [Tanenbaum, "Modern Operating Systems" Ch.2 "Processes and Threads"](https://www.pearson.com/en-us/subject-catalog/p/modern-operating-systems/P200000003460) --- プロセスとスケジューリングの教科書
