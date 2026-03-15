/*
 * minifs.c --- メモリ内 inode ファイルシステム
 *
 * UNIX ファイルシステムの核心を最小限に再現する。
 *
 * 構造:
 *   - inode 配列: ファイルのメタデータとデータを保持（最大 16 個）
 *   - ディレクトリエントリ: 「名前 → inode 番号」のマッピング
 *   - ルートディレクトリ: inode 0 に固定
 *
 * 操作:
 *   fs_create(name)       --- 新しいファイルを作成
 *   fs_write(name, data)  --- ファイルにデータを書き込み
 *   fs_read(name)         --- ファイルからデータを読み出し
 *   fs_list()             --- ディレクトリ内のファイル一覧を表示
 *
 * 実際の FS との対応:
 *   - inode 配列 → ディスク上の inode テーブル
 *   - dir_entry  → ディレクトリブロック内のエントリ
 *   - data[]     → データブロック（ここでは inode 内に直接格納）
 */

#include <stdio.h>
#include <string.h>
#include "fs.h"

/* グローバルなファイルシステム状態 */
static inode_t     inodes[MAX_INODES];
static dir_entry_t root_dir[MAX_DIR_ENTRIES];
static int         next_inode = 1; /* inode 0 はルートディレクトリ用 */

/*
 * fs_init --- ファイルシステムを初期化する
 *
 * inode 0 をルートディレクトリとして確保し、
 * ディレクトリエントリを全て空きにする。
 */
static void fs_init(void) {
    /* 全 inode を未使用に */
    for (int i = 0; i < MAX_INODES; i++) {
        inodes[i].type = INODE_FREE;
        inodes[i].size = 0;
        memset(inodes[i].data, 0, MAX_DATA);
    }

    /* inode 0 = ルートディレクトリ */
    inodes[0].type = INODE_DIR;

    /* ディレクトリエントリを空きに */
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        root_dir[i].name[0] = '\0';
        root_dir[i].inode_num = -1;
    }
}

/*
 * find_entry --- 名前でディレクトリエントリを検索する
 *
 * 見つかったらインデックスを返す。見つからなければ -1。
 */
static int find_entry(const char *name) {
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (root_dir[i].inode_num >= 0 &&
            strcmp(root_dir[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * fs_create --- ファイルを作成する
 *
 * 空き inode と空きディレクトリエントリを確保し、
 * 名前と inode 番号を紐付ける。
 */
int fs_create(const char *name) {
    /* 既存チェック */
    if (find_entry(name) >= 0) {
        fprintf(stderr, "error: %s already exists\n", name);
        return -1;
    }

    /* inode 枯渇チェック */
    if (next_inode >= MAX_INODES) {
        fprintf(stderr, "error: no free inodes\n");
        return -1;
    }

    /* 空きディレクトリエントリを探す */
    int slot = -1;
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (root_dir[i].inode_num < 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        fprintf(stderr, "error: directory full\n");
        return -1;
    }

    /* inode を確保 */
    int ino = next_inode++;
    inodes[ino].type = INODE_FILE;
    inodes[ino].size = 0;

    /* ディレクトリエントリに登録 */
    strncpy(root_dir[slot].name, name, MAX_NAME - 1);
    root_dir[slot].name[MAX_NAME - 1] = '\0';
    root_dir[slot].inode_num = ino;

    printf("created: %s\n", name);
    return 0;
}

/*
 * fs_write --- ファイルにデータを書き込む
 *
 * ファイルの data[] にコピーし、サイズを更新する。
 * 既存データは上書きされる（追記ではない）。
 */
int fs_write(const char *name, const char *data) {
    int idx = find_entry(name);
    if (idx < 0) {
        fprintf(stderr, "error: %s not found\n", name);
        return -1;
    }

    int ino = root_dir[idx].inode_num;
    size_t len = strlen(data);
    if (len >= MAX_DATA) {
        len = MAX_DATA - 1;
    }

    memcpy(inodes[ino].data, data, len);
    inodes[ino].data[len] = '\0';
    inodes[ino].size = len;

    printf("wrote %zu bytes to %s\n", len, name);
    return (int)len;
}

/*
 * fs_read --- ファイルからデータを読み出す
 *
 * ファイルの data[] へのポインタを返す。
 */
const char *fs_read(const char *name) {
    int idx = find_entry(name);
    if (idx < 0) {
        fprintf(stderr, "error: %s not found\n", name);
        return NULL;
    }

    int ino = root_dir[idx].inode_num;
    return inodes[ino].data;
}

/*
 * fs_list --- ルートディレクトリのファイル一覧を表示する
 *
 * 有効なディレクトリエントリの名前をスペース区切りで出力する。
 * ls コマンドの最小版。
 */
void fs_list(void) {
    int first = 1;
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (root_dir[i].inode_num >= 0) {
            if (!first) {
                printf("  ");
            }
            printf("%s", root_dir[i].name);
            first = 0;
        }
    }
    printf("\n");
}

int main(void) {
    fs_init();

    /* ファイルを作成 */
    fs_create("hello.txt");

    /* データを書き込み */
    fs_write("hello.txt", "Hello, World!");

    /* データを読み出し */
    const char *data = fs_read("hello.txt");
    if (data) {
        printf("read: %s\n", data);
    }

    /* 2 つ目のファイルを作成 */
    fs_create("test.txt");

    /* ファイル一覧 */
    fs_list();

    return 0;
}
