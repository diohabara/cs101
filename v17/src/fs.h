/*
 * fs.h --- メモリ内 inode ファイルシステムの定義
 *
 * inode はファイルのメタデータ（種類、サイズ）と実データを保持する構造体。
 * UNIX ファイルシステムでは inode 番号でファイルを識別し、
 * ディレクトリエントリが「名前 → inode 番号」のマッピングを提供する。
 *
 *   ディレクトリエントリ          inode 配列
 *   ┌──────────┬────┐       ┌───────────────────┐
 *   │ "hello"  │  1 │──────→│ type=FILE, size=13│
 *   ├──────────┼────┤       │ data="Hello, ..." │
 *   │ "test"   │  2 │──┐   ├───────────────────┤
 *   └──────────┴────┘  └──→│ type=FILE, size=0 │
 *                           │ data=""           │
 *                           └───────────────────┘
 */

#ifndef FS_H
#define FS_H

#include <stddef.h>

/* 定数 */
#define MAX_INODES   16
#define MAX_DATA     256
#define MAX_NAME     32
#define MAX_DIR_ENTRIES 16

/* inode の種類 */
typedef enum {
    INODE_FREE = 0,  /* 未使用 */
    INODE_FILE,      /* 通常ファイル */
    INODE_DIR        /* ディレクトリ */
} inode_type_t;

/*
 * inode --- ファイル 1 つ分のメタデータ + データ
 *
 * 実際の UNIX inode はデータを別のブロックに置くが、
 * この教材では簡略化して inode 内に直接データを格納する。
 */
typedef struct {
    inode_type_t type;       /* ファイル種別 */
    size_t       size;       /* データサイズ（バイト） */
    char         data[MAX_DATA]; /* ファイル内容 */
} inode_t;

/*
 * ディレクトリエントリ --- 名前と inode 番号の対
 *
 * ディレクトリは「名前 → inode 番号」の表。
 * ls コマンドはこの表を読んでファイル名一覧を表示する。
 */
typedef struct {
    char name[MAX_NAME]; /* ファイル名 */
    int  inode_num;      /* 対応する inode 番号（-1 = 空き） */
} dir_entry_t;

/* ファイルシステム操作 */

/* ファイルを作成する。成功時は 0、失敗時は -1 を返す。 */
int fs_create(const char *name);

/* ファイルにデータを書き込む。書き込んだバイト数を返す。失敗時は -1。 */
int fs_write(const char *name, const char *data);

/* ファイルからデータを読み出す。データへのポインタを返す。失敗時は NULL。 */
const char *fs_read(const char *name);

/* ルートディレクトリのファイル一覧を表示する。 */
void fs_list(void);

#endif /* FS_H */
