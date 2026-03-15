`v9` では、v1-v8 で学んだ x86-64 アセンブリの概念を C 言語で再表現します。
ポインタの間接参照、volatile キーワード、ビット演算を通じて、「アセンブリで直接やっていたこと」が C ではどう書かれるかを確認します。

## 概要

`v9` は Phase 1（OS 基礎編）の入口です。ここから実装言語がアセンブリから C に切り替わります。
C はアセンブリの「薄いラッパー」であり、v1-v8 で学んだ概念がそのまま使えることを体験します。

## なぜ C か

v1-v8 ではレジスタ操作、メモリアクセス、syscall をアセンブリで直接書きました。
C はこれらの操作に対して最小限の抽象化を加えた言語です。

| アセンブリ (v1-v8) | C (v9〜) | 対応 |
|---------------------|----------|------|
| `mov rax, 42` | `int x = 42;` | レジスタ → 変数 |
| `mov [rax], 42` | `*ptr = 42;` | メモリ書き込み → ポインタ間接参照 |
| `mov rbx, [rax]` | `int y = *ptr;` | メモリ読み出し → ポインタ間接参照 |
| RFLAGS ビット操作 | `flags \|= FLAG` | フラグレジスタ → ビット演算 |
| `syscall` | `write()`, `mmap()` | 直接 syscall → libc ラッパー |

## ポインタの間接参照

C のポインタは、アセンブリの「レジスタにアドレスを入れて間接アクセス」と同じです。

```
アセンブリ:                    C:
  lea rax, [x]                  int *ptr = &x;
  mov qword [rax], 42           *ptr = 42;
  mov rbx, [rax]                int y = *ptr;
```

`&x` はアドレス取得（lea に相当）、`*ptr` は間接参照（`[rax]` に相当）です。

### ptr_deref — ポインタ経由の値の読み書き

| ステップ | C コード | アセンブリ相当 | 状態 |
|----------|----------|----------------|------|
| 1 | `int x = 0;` | `mov qword [x], 0` | 変数 x をゼロ初期化 |
| 2 | `int *ptr = &x;` | `lea rax, [x]` | アドレスを取得 |
| 3 | `*ptr = 42;` | `mov qword [rax], 42` | ポインタ経由で書き込み |
| 4 | `return *ptr;` | `mov rdi, [rax]` / `exit` | 値を読み出して exit code に |

{{code:src/ptr_deref.c}}

v1 の `exit42.asm` と同じ結果（exit code 42）を、C のポインタで実現しています。

## volatile キーワード

`volatile` は「この変数へのアクセスをコンパイラが最適化で省略してはならない」という指示です。

### なぜ必要か

通常、コンパイラは最適化で不要な読み書きを省略します。例えば以下のコードで `x = 10; x = 20;` とあれば、最初の `x = 10` は最終的に上書きされるため、コンパイラは省略しても結果は同じです。

しかし MMIO（Memory-Mapped I/O）では事情が異なります。ハードウェアレジスタに対する書き込みはそれぞれが独立した「コマンド送信」です。最初の書き込みを省略されると、デバイスが正しく動作しません。

```
volatile なし（コンパイラが最適化可能）:
  device_reg = 10;   ← 省略される可能性
  device_reg = 20;   ← これだけ残る

volatile あり（全アクセスが保証される）:
  device_reg = 10;   ← 必ず実行（コマンド A 送信）
  device_reg = 20;   ← 必ず実行（コマンド B 送信）
```

### volatile_io — volatile の効果確認

| ステップ | C コード | 意味 |
|----------|----------|------|
| 1 | `volatile int device_reg = 0;` | 仮想デバイスレジスタ |
| 2 | `device_reg = 10;` | コマンド A 送信（省略不可） |
| 3 | `device_reg = 20;` | コマンド B 送信（省略不可） |
| 4 | `int status = device_reg;` | ステータス読み出し → 20 |
| 5 | `return status;` | exit code 20 |

{{code:src/volatile_io.c}}

`-O2` で最適化しても、volatile があるため全ての読み書きが実行されます。
v15 の MMIO シミュレーションでこの概念が実践的に活きてきます。

## ビット演算

v2 で RFLAGS レジスタのビットを `cmp` や `test` で操作しました。
C では `|`, `&`, `~`, `^` 演算子でビット単位の操作を明示的に行います。

| 操作 | アセンブリ | C | 意味 |
|------|-----------|---|------|
| セット | `or flags, FLAG` | `flags \|= FLAG;` | ビットを 1 にする |
| テスト | `test flags, FLAG` | `flags & FLAG` | ビットが 1 か確認 |
| クリア | `and flags, ~FLAG` | `flags &= ~FLAG;` | ビットを 0 にする |
| トグル | `xor flags, FLAG` | `flags ^= FLAG;` | ビットを反転 |

### ビットフラグの仕組み

1 つの整数の各ビットを独立したフラグとして使います。

```
ビット位置:   3    2    1    0
フラグ名:   OVF  SIGN ZERO CARRY
            ───  ───  ───  ───
例: 0x03 =   0    0    1    1   ← ZERO と CARRY がセット
例: 0x04 =   0    1    0    0   ← SIGN のみセット
```

### bitops — フラグの設定・テスト・クリア・トグル

| ステップ | C コード | flags の値 | 説明 |
|----------|----------|-----------|------|
| 1 | `flags \|= FLAG_CARRY \| FLAG_ZERO` | `0x03` | CF, ZF をセット |
| 2 | `flags & FLAG_CARRY` → true | `0x03` | CF をテスト |
| 3 | `flags &= ~FLAG_ZERO` | `0x01` | ZF をクリア |
| 4 | `flags ^= FLAG_CARRY \| FLAG_SIGN` | `0x04` | CF, SF をトグル |

{{code:src/bitops.c}}

最終値 `0x04`（= FLAG_SIGN のみ）が exit code として返されます。

## PlayStation との関連

- **volatile**: PS4/PS5 の GPU コマンドプロセッサはメモリマップドレジスタで制御されます。ドライバはこれらのレジスタに volatile ポインタ経由でアクセスし、コマンドバッファの submit や割り込みの制御を行います
- **ビット演算**: GPU コマンドバッファのフラグフィールドはビット演算で構築されます。描画状態（ブレンドモード、カリング方向、デプステスト有無）を各ビットに詰めてレジスタに書き込みます

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [ISO/IEC 9899:2011 (C11) §6.7.3](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) — volatile 修飾子の定義。「volatile 修飾された型のオブジェクトへのアクセスは、実装が定義する方法で副作用を持つ可能性がある」
- [GCC Optimization Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html) — `-O2` 最適化レベルの仕様
- [ISO/IEC 9899:2011 §6.5.10-12](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) — ビット演算子 `&`, `^`, `|` のセマンティクス
