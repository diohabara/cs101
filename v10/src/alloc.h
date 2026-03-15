/*
 * alloc.h --- メモリアロケータ共通定義
 *
 * bump allocator と free-list allocator で共有するヘッダ。
 * 各 .c ファイルは独立した実行可能ファイルにコンパイルされるため、
 * 関数の実体は含まず、定数とインライン関数のみ定義する。
 */

#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>   /* size_t */
#include <stdint.h>   /* uintptr_t */
#include <sys/mman.h> /* mmap, munmap */
#include <stdio.h>
#include <stdlib.h>

/* mmap で取得するプールサイズ（1 ページ = 4096 バイト） */
#define POOL_SIZE 4096

/*
 * mmap で匿名メモリプールを取得する。
 *
 *   カーネルへの syscall:
 *     mov rax, 9       ; SYS_mmap
 *     mov rdi, 0       ; addr = NULL（カーネル任せ）
 *     mov rsi, 4096    ; length
 *     mov rdx, 3       ; PROT_READ | PROT_WRITE
 *     mov r10, 0x22    ; MAP_PRIVATE | MAP_ANONYMOUS
 *     mov r8, -1       ; fd = -1
 *     mov r9, 0        ; offset = 0
 *     syscall
 */
static inline void *pool_create(size_t size) {
    void *p = mmap(NULL, size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return p;
}

/* mmap で取得したプールを解放する */
static inline void pool_destroy(void *pool, size_t size) {
    munmap(pool, size);
}

/*
 * アライメントのためにサイズを 8 バイト境界に切り上げる。
 *
 * 例: align8(5) = 8, align8(16) = 16, align8(17) = 24
 *
 * x86-64 では多くのデータ型が 8 バイトアラインを要求する。
 * 非アラインアクセスはパフォーマンスペナルティを招く。
 */
static inline size_t align8(size_t n) {
    return (n + 7) & ~(size_t)7;
}

#endif /* ALLOC_H */
