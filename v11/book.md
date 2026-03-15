`v11` では、OS がプロセスをどのように管理するかを C で表現します。
プロセス制御ブロック（PCB）、状態遷移モデル、fork/exec/wait の仕組みを実装を通じて理解します。

## 概要

OS はプロセスという抽象を通じてプログラムの実行を管理します。
各プロセスは PCB（Process Control Block）というデータ構造で表現され、OS カーネルは PCB を操作することでプロセスのライフサイクルを制御します。

v9 で C 言語の基礎を学んだので、ここからは C の構造体と関数を使って OS の内部構造を表現していきます。

## プロセス制御ブロック（PCB）

PCB は OS がプロセスを管理するための中心的なデータ構造です。
1 つのプロセスにつき 1 つの PCB が存在し、以下の情報を保持します。

```
PCB の構成要素（実際の OS）:
┌─────────────────────────────┐
│ PID: 1234                   │ ← プロセス識別子
│ State: RUNNING              │ ← 現在の状態
│ Program Counter: 0x401000   │ ← 次に実行する命令のアドレス
│ Registers: rax=0, rbx=42...│ ← コンテキスト（レジスタ保存領域）
│ Memory Map: text=0x400000   │ ← メモリレイアウト情報
│ Open Files: [fd0, fd1, fd2] │ ← ファイルディスクリプタテーブル
│ Priority: 20                │ ← スケジューリング優先度
│ Parent PID: 1000            │ ← 親プロセスの PID
└─────────────────────────────┘
```

v11 では最小限のフィールド（PID、状態、名前）で概念を示します。

## 5 状態モデル

OS の教科書で広く使われるプロセスの状態遷移モデルです。

![5 状態プロセスモデル](/images/v11/process-states.drawio.svg)

```
                 ディスパッチ          exit()
  ┌───────┐    ┌──────────┐    ┌────────────┐
  │ READY │───→│ RUNNING  │───→│ TERMINATED │
  └───────┘    └──────────┘    └────────────┘
      ↑            │
      │            │ I/O 要求
      │            ↓
      │       ┌──────────┐
      └───────│ BLOCKED  │
   I/O 完了   └──────────┘
```

| 状態 | 説明 | 例 |
|------|------|----|
| READY | 実行可能だが CPU 待ち | fork 直後の子プロセス |
| RUNNING | CPU 上で命令を実行中 | 現在実行中のプロセス |
| BLOCKED | 外部イベント待ち | read() で I/O 完了を待機中 |
| TERMINATED | 実行完了、資源回収待ち | exit() 呼び出し後 |

### 状態遷移の契機

| 遷移 | 契機 | カーネルの動作 |
|------|------|----------------|
| READY → RUNNING | スケジューラがディスパッチ | PCB からレジスタを復元、CPU に実行権を渡す |
| RUNNING → READY | タイムスライス消費 | レジスタを PCB に保存、Ready キューに戻す |
| RUNNING → BLOCKED | I/O 要求 / wait() | PCB を Wait キューに移動 |
| BLOCKED → READY | I/O 完了 / 子終了 | PCB を Ready キューに移動 |
| RUNNING → TERMINATED | exit() / return | PCB にリターンコードを記録、資源を解放 |

## pcb.c — PCB と状態遷移のシミュレーション

PCB を構造体で定義し、典型的なライフサイクルをシミュレートします。

| ステップ | 遷移 | 契機（シナリオ） | PCB の state |
|----------|-------|-----------------|--------------|
| 1 | READY → RUNNING | スケジューラが選択 | RUNNING |
| 2 | RUNNING → BLOCKED | I/O を要求 | BLOCKED |
| 3 | BLOCKED → RUNNING | I/O 完了、再ディスパッチ | RUNNING |
| 4 | RUNNING → TERMINATED | プロセスが exit() | TERMINATED |

### ヘッダ: process.h

{{code:src/process.h}}

### 本体: pcb.c

{{code:src/pcb.c}}

## fork/exec/wait — UNIX のプロセス生成モデル

UNIX 系 OS では、プロセス生成を 3 つの独立した操作に分離しています。

```
親プロセス (PID 1000)
    │
    │ fork()
    ├──────────────────────┐
    │                      │
    │ 親 (PID 1000)        │ 子 (PID 1001)
    │ fork() → 1001        │ fork() → 0
    │                      │
    │ waitpid(1001)        │ execve("/bin/echo", ...)
    │ 状態: BLOCKED        │ メモリイメージを置換
    │                      │
    │                      │ "hello from child" 出力
    │                      │ exit(0)
    │                      │
    │ 状態: RUNNING         ↓（子プロセス終了）
    │ "parent: child
    │  exited with 0"
    ↓
```

### fork() — プロセスの複製

fork() はプロセス全体を複製します。PCB も含めてコピーされます。

```
fork() 前:                 fork() 後:
┌────────────────┐        ┌────────────────┐  ┌────────────────┐
│ PID: 1000      │        │ PID: 1000      │  │ PID: 1001      │
│ State: RUNNING │   →    │ State: RUNNING │  │ State: READY   │
│ PC: fork()の次 │        │ 戻り値: 1001   │  │ 戻り値: 0      │
│ メモリ: ...    │        │ メモリ: (共有) │  │ メモリ: (CoW)  │
└────────────────┘        └────────────────┘  └────────────────┘
```

fork() の戻り値が親子で異なるのは、カーネルが複製後の各 PCB に異なる値を設定するためです。
同じコードが 2 回実行されるのではなく、2 つの独立したプロセスがそれぞれ再開します。

### execve() — プログラムの置換

execve() は PCB はそのまま（PID やファイルディスクリプタは維持）に、メモリイメージ（テキスト、データ、スタック）を新しいプログラムで置換します。成功すると返りません。

### waitpid() — 子プロセスの終了待ち

親プロセスが子の終了を待ちます。親の状態は RUNNING → BLOCKED に遷移し、子が終了すると BLOCKED → READY → RUNNING と復帰します。

| ステップ | 親の状態 | 子の状態 | 動作 |
|----------|---------|---------|------|
| 1 | RUNNING | (存在しない) | fork() 呼び出し |
| 2 | RUNNING | READY | 子の PCB 作成完了 |
| 3 | RUNNING | RUNNING | 子がディスパッチされる |
| 4 | BLOCKED | RUNNING | 親が waitpid() でブロック |
| 5 | BLOCKED | RUNNING | 子が execve() でイメージ置換 |
| 6 | BLOCKED | TERMINATED | 子が exit(0) |
| 7 | RUNNING | (回収済み) | 親に子の終了が通知される |

{{code:src/fork_exec.c}}

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [POSIX.1-2017 fork()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html) — fork() の仕様。「新しいプロセスは呼び出しプロセスのコピーとして作成される」
- [POSIX.1-2017 exec](https://pubs.opengroup.org/onlinepubs/9699919799/functions/exec.html) — exec ファミリの仕様。「プロセスイメージを新しいプロセスイメージで置換する」
- [POSIX.1-2017 waitpid()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/waitpid.html) — waitpid() の仕様。子プロセスの状態変化を待つ
- [FreeBSD sys/proc.h](https://cgit.freebsd.org/src/tree/sys/sys/proc.h) — FreeBSD の `struct proc` 定義
