`v17` は Phase 1 の総集編です。メモリ内 inode ファイルシステムとパイプ/リダイレクト付きミニシェルを実装し、UNIX の核心である「すべてはファイル」を体験します。

## 概要

UNIX では、ファイル、パイプ、デバイス、ソケットがすべて fd（ファイルディスクリプタ）テーブルのエントリとして統一的に扱われます。`ls | grep foo > out.txt` というコマンドは、3 つの fd 操作だけで実現できます。

v17 では 2 つのプログラムを作ります。

1. **minifs** --- メモリ内 inode ファイルシステム。ファイルの作成・読み書き・一覧を行う
2. **minishell** --- パイプとリダイレクト付きミニシェル。fd の付け替えでコマンドを接続する

## inode とは

inode はファイルのメタデータを保持する構造体です。UNIX ファイルシステムでは、ファイルの実体は inode 番号で識別されます。ファイル名は直接ファイルを指すのではなく、ディレクトリエントリを通じて inode 番号にマッピングされます。

```
ディレクトリエントリ              inode 配列
┌──────────┬────┐         ┌───────────────────────┐
│ "hello"  │  1 │────────→│ inode 1               │
├──────────┼────┤         │ type = FILE            │
│ "test"   │  2 │──┐     │ size = 13              │
└──────────┴────┘  │     │ data = "Hello, World!" │
                   │     ├───────────────────────┤
                   └────→│ inode 2               │
                         │ type = FILE            │
                         │ size = 0              │
                         │ data = ""             │
                         └───────────────────────┘
```

この構造により、1 つのファイルに複数の名前（ハードリンク）を付けることができます。ディレクトリエントリが異なっても、同じ inode 番号を指していれば同じファイルです。

### minifs の inode 構造

v17 の minifs では簡略化して、inode 内にデータを直接格納します。実際の FS ではデータはブロック単位で別の領域に置かれますが、概念は同じです。

```c
typedef struct {
    inode_type_t type;       /* FILE or DIR */
    size_t       size;       /* データサイズ（バイト） */
    char         data[256];  /* ファイル内容（簡略化） */
} inode_t;
```

最大 16 個の inode を配列で管理します。inode 0 はルートディレクトリに予約されています。

## fd テーブル

プロセスが open() や pipe() を呼ぶと、カーネルは fd テーブルにエントリを追加します。fd はただの整数（0, 1, 2, ...）で、プロセスからは「番号でアクセスするだけ」です。

```
プロセスの fd テーブル
┌─────┬──────────────────────┐
│ fd  │ 指す先               │
├─────┼──────────────────────┤
│  0  │ stdin（キーボード）   │
│  1  │ stdout（画面）        │
│  2  │ stderr（画面）        │
│  3  │ open("file.txt") の結果 │
└─────┴──────────────────────┘
```

重要なのは、fd 0, 1, 2 は特別な番号ではなく、単にプログラム起動時にシェルが設定するデフォルトの割り当てだということです。dup2() で自由に差し替えられます。

![fd テーブルから inode を経由してデータにアクセスする流れ](/images/v17/inode-fd.drawio.svg)

## minifs --- メモリ内ファイルシステム

### 操作と実行結果

| ステップ | 操作 | 出力 |
|----------|------|------|
| 1 | `fs_create("hello.txt")` | `created: hello.txt` |
| 2 | `fs_write("hello.txt", "Hello, World!")` | `wrote 13 bytes to hello.txt` |
| 3 | `fs_read("hello.txt")` | `read: Hello, World!` |
| 4 | `fs_create("test.txt")` | `created: test.txt` |
| 5 | `fs_list()` | `hello.txt  test.txt` |

### 内部の動き

`fs_create("hello.txt")` は次の 3 ステップで動きます。

1. 空き inode を探す（next_inode をインクリメント）
2. inode の type を FILE に設定する
3. ディレクトリエントリに「"hello.txt" → inode 1」を登録する

`fs_write("hello.txt", "Hello, World!")` は名前からディレクトリエントリを検索し、対応する inode のデータ領域にコピーします。

{{code:src/fs.h}}

{{code:src/minifs.c}}

## パイプ --- fd ペアによるプロセス間通信

pipe() システムコールは 2 つの fd を作ります。一方に書き込むと、もう一方から読み出せます。

```
pipe() が返す fd ペア:
  pipefd[0] = read 端（データを読む側）
  pipefd[1] = write 端（データを書く側）

  書き込み側              読み出し側
  ┌────────┐  カーネル内  ┌────────┐
  │ fd = 4 │──→ バッファ ──→│ fd = 3 │
  └────────┘   (4KB 等)   └────────┘
```

`echo hello | cat` を実行するとき、シェルは次のように fd を付け替えます。

```
echo プロセス:              cat プロセス:
  fd 0 → stdin              fd 0 → pipe read端
  fd 1 → pipe write端       fd 1 → stdout
  fd 2 → stderr             fd 2 → stderr
```

echo は stdout（fd 1）に書き込むつもりですが、実際にはパイプの write 端に繋がっています。cat は stdin（fd 0）から読むつもりですが、実際にはパイプの read 端に繋がっています。プログラム自体は fd の付け替えを知りません。

## リダイレクト --- dup2 による stdout の差し替え

`echo hello > out.txt` は次のように動きます。

```
1. open("out.txt", O_WRONLY | O_CREAT | O_TRUNC) → fd 3
2. dup2(3, STDOUT_FILENO)  → fd 1 がファイルを指すようになる
3. close(3)                → 元の fd 3 は不要
4. execvp("echo", ...)     → echo は fd 1 に書く → ファイルに出力される

  fd テーブルの変化:
  dup2 前:                  dup2 後:
  fd 0 → stdin             fd 0 → stdin
  fd 1 → stdout（画面）     fd 1 → out.txt（ファイル）
  fd 2 → stderr            fd 2 → stderr
  fd 3 → out.txt           （fd 3 は close 済み）
```

dup2(old_fd, new_fd) は「new_fd が指す先を old_fd と同じにする」操作です。echo プログラムは自分の出力がファイルに向いていることを知りません。

## minishell --- コマンド実行の流れ

minishell は argv でコマンド文字列を受け取り、パイプ (`|`) とリダイレクト (`>`) を検出して適切に実行します。

### 単純実行

```
./minishell "echo hello"
→ exec: echo hello
→ hello
```

fork() で子プロセスを作り、execvp() でコマンドを実行します。

### パイプ実行

```
./minishell "echo hello | cat"
→ pipe: echo hello | cat
→ hello
```

1. pipe() で fd ペアを作る
2. fork() で左の子を作り、stdout をパイプの write 端に差し替え
3. fork() で右の子を作り、stdin をパイプの read 端に差し替え
4. 両方の子を waitpid() で待つ

### リダイレクト実行

```
./minishell "echo hello > /tmp/out.txt"
→ redirect: echo hello > /tmp/out.txt
（/tmp/out.txt に "hello" が書き込まれる）
```

1. fork() で子を作る
2. 子で open() してファイルの fd を得る
3. dup2() で stdout をファイルに差し替え
4. execvp() でコマンドを実行

{{code:src/minishell.c}}

## Phase 1 のつながり

v17 は Phase 1（v9-v17）の総集編として、これまでの概念を統合しています。

| 段階 | 概念 | v17 での活用 |
|------|------|-------------|
| v9 | ポインタ、C 言語基礎 | inode 配列のポインタ操作、文字列処理 |
| v10-v12 | メモリ管理 | ファイルデータのメモリ上管理 |
| v13-v14 | プロセス | fork + exec によるコマンド実行 |
| v15-v16 | I/O | fd テーブル、pipe、dup2 |
| **v17** | **統合** | **FS + シェル = UNIX の核心** |

「すべてはファイル」という UNIX の設計思想は、fd テーブルという単一の抽象化で実現されています。通常ファイル、パイプ、デバイスが同じ read/write インターフェースで扱えるのは、fd テーブルのエントリが指す先だけが異なるからです。

## PlayStation との関連

- **PFS（PlayStation File System）**: PS3/PS4 は独自のファイルシステム（PFS）を使用します。PFS も inode ベースの設計で、ファイルメタデータとデータブロックを分離して管理します。暗号化レイヤーが追加されている点が特徴です
- **fd 管理**: ゲームエンジンはファイル I/O を非同期で行います。内部的には fd テーブルと同等の管理構造を持ち、アセットのストリーミング読み込みを制御します

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [The UNIX Time-Sharing System (Ritchie & Thompson, 1974)](https://dsf.berkeley.edu/cs262/unix.pdf) --- UNIX のファイルシステムと「すべてはファイル」の設計思想の原典
- [pipe(2) - Linux man page](https://man7.org/linux/man-pages/man2/pipe.2.html) --- pipe() システムコールの仕様
- [dup2(2) - Linux man page](https://man7.org/linux/man-pages/man2/dup2.2.html) --- dup2() システムコールの仕様
- [The Design and Implementation of the 4.4BSD Operating System (McKusick et al.)](https://www.freebsd.org/doc/en/books/design-44bsd/) --- BSD inode ファイルシステムの設計
