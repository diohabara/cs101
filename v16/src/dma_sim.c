/*
 * dma_sim.c --- DMA 転送シミュレーション
 *
 * DMA (Direct Memory Access) の核心:
 *   CPU がバイトを 1 つずつコピーする代わりに、デバイス（DMA エンジン）が
 *   自分でメモリにデータを書き込む。CPU は転送の「開始」と「完了確認」だけ。
 *
 * 実行フロー:
 *   1. CPU: 転送元バッファにデータを用意する
 *   2. CPU: DMA ディスクリプタに転送情報を書き込み、status = PENDING にする
 *   3. DMA エンジン: ディスクリプタを読み取り、データをコピーする
 *   4. DMA エンジン: status = COMPLETE にする
 *   5. CPU: status をポーリングして完了を検知する
 *   6. CPU: 転送先バッファのデータが正しいか検証する
 *
 * このシミュレーションは同一プロセス内で行うが、実際のハードウェアでは
 * ステップ 3-4 は PCIe バス上のデバイスが独立して実行する。
 * CPU はその間、別の仕事ができる。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dma.h"

/*
 * DMA エンジンのシミュレーション。
 *
 * 実際のハードウェアでは、DMA コントローラが PCIe バス経由で
 * メモリを直接読み書きする。ここでは memcpy で代替する。
 *
 * ポイント: CPU はこの関数を呼ぶ（= 転送開始を指示する）だけで、
 * データのコピー自体は「デバイス側」が行う。
 */
static void dma_engine_process(DmaDescriptor *desc) {
    if (desc->status != DMA_PENDING) {
        return;  /* 転送要求がなければ何もしない */
    }

    /* デバイスがメモリを直接コピーする（CPU は関与しない） */
    memcpy(desc->dst, desc->src, desc->length);

    /* 転送完了を通知する（実際はステータスレジスタへの書き込み） */
    desc->status = DMA_COMPLETE;
}

int main(void) {
    /* 転送元: CPU が用意するデータ */
    const char src[] = "Hello, DMA!";

    /* 転送先: ゼロクリアされたバッファ（DMA 転送後にデータが入る） */
    char dst[32];
    memset(dst, 0, sizeof(dst));

    /*
     * ステップ 1: CPU がディスクリプタを作成する
     *
     * NVMe では、これが Submission Queue Entry (SQE) に相当する。
     * CPU が SQE を書き込んで Doorbell レジスタを叩くと、
     * デバイスが転送を開始する。
     */
    DmaDescriptor desc;
    desc.src    = src;
    desc.dst    = dst;
    desc.length = strlen(src) + 1;  /* null 終端を含む */
    desc.status = DMA_PENDING;
    printf("DMA descriptor submitted\n");

    /*
     * ステップ 2: DMA エンジンが転送を実行する
     *
     * 実際のハードウェアではこの処理は非同期で行われる。
     * CPU は転送中に別の処理を実行できる。
     */
    dma_engine_process(&desc);

    /*
     * ステップ 3: CPU がポーリングで完了を確認する
     *
     * NVMe では Completion Queue Entry (CQE) を確認する。
     * 割り込みベースの通知もあるが、高性能 NVMe ドライバは
     * ポーリングモードを使う（レイテンシが低い）。
     */
    if (desc.status == DMA_COMPLETE) {
        printf("DMA transfer complete\n");
    }

    /*
     * ステップ 4: 転送されたデータを検証する
     */
    if (strcmp(dst, src) == 0) {
        printf("data verified: %s\n", dst);
    } else {
        fprintf(stderr, "data mismatch!\n");
        return 1;
    }

    return 0;
}
