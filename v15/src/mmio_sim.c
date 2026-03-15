/*
 * mmio_sim.c --- MMIO デバイスアクセスのシミュレーション
 *
 * Memory-Mapped I/O（MMIO）では、デバイスのレジスタが特定のメモリアドレスに
 * マッピングされる。CPU は通常の load/store 命令でデバイスと通信する。
 *
 * このプログラムでは mmap で匿名メモリを確保し、それを「仮想デバイスの
 * レジスタ領域」として使う。volatile ポインタ経由でアクセスすることで、
 * コンパイラの最適化によるアクセス省略を防ぐ。
 *
 * 流れ:
 *   1. mmap で MMIO 領域を確保
 *   2. COMMAND_REG に CMD_READ を書き込む（デバイスにコマンド送信）
 *   3. デバイス側をシミュレート: STATUS を BUSY → DONE に遷移させ、DATA を設定
 *   4. STATUS_REG をポーリングして DONE を待つ
 *   5. DATA_REG からデータを読み出す
 *   6. munmap で領域を解放
 *
 * 実際のハードウェアでは:
 *   - mmap の代わりにカーネルが物理アドレスを仮想アドレスにマッピング
 *   - ステータス遷移はデバイスのハードウェアが行う
 *   - ポーリングの代わりに割り込みを使うことも多い
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include "mmio.h"

/*
 * simulate_device --- デバイス側の動作をシミュレート
 *
 * 実際のハードウェアでは、デバイスのコントローラが行う処理。
 * ここでは CPU 側のコードで直接レジスタを操作して模擬する。
 *
 * base: MMIO 領域のベースアドレス
 */
static void simulate_device(volatile uint32_t *base) {
    volatile uint32_t *status_reg = (volatile uint32_t *)((char *)base + MMIO_STATUS_REG_OFFSET);
    volatile uint32_t *data_reg   = (volatile uint32_t *)((char *)base + MMIO_DATA_REG_OFFSET);

    /* デバイスがコマンドを受け取り、処理中になる */
    *status_reg = STATUS_BUSY;

    /* デバイスが処理を完了し、データを準備する */
    *data_reg   = 0xDEADBEEF;
    *status_reg = STATUS_DONE;
}

int main(void) {
    /*
     * mmap で匿名メモリを確保する。
     * MAP_ANONYMOUS: ファイルに紐付かない匿名マッピング
     * MAP_PRIVATE:   他のプロセスと共有しない
     *
     * 実際のデバイスドライバでは、ここでデバイスの物理アドレスを
     * mmap(fd, ...) でマッピングする。
     */
    void *mmio_base = mmap(
        NULL,                        /* カーネルにアドレスを任せる */
        MMIO_REGION_SIZE,            /* レジスタ領域のサイズ */
        PROT_READ | PROT_WRITE,      /* 読み書き可能 */
        MAP_ANONYMOUS | MAP_PRIVATE, /* 匿名プライベートマッピング */
        -1,                          /* ファイルなし */
        0                            /* オフセットなし */
    );
    if (mmio_base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* 領域をゼロ初期化（STATUS = IDLE, DATA = 0） */
    memset(mmio_base, 0, MMIO_REGION_SIZE);

    /*
     * volatile ポインタを設定する。
     *
     * volatile が必要な理由:
     *   - STATUS_REG はデバイス側が値を変更する可能性がある
     *   - volatile がないと、コンパイラがポーリングループを
     *     「どうせ値は変わらない」と判断して最適化で除去する恐れがある
     *
     * v9 の volatile_io.c で学んだ「最適化を防ぐ」効果が、
     * ここでは実際のデバイスポーリングパターンとして現れる。
     */
    volatile uint32_t *command_reg = (volatile uint32_t *)((char *)mmio_base + MMIO_COMMAND_REG_OFFSET);
    volatile uint32_t *status_reg  = (volatile uint32_t *)((char *)mmio_base + MMIO_STATUS_REG_OFFSET);
    volatile uint32_t *data_reg    = (volatile uint32_t *)((char *)mmio_base + MMIO_DATA_REG_OFFSET);

    /*
     * ステップ 1: コマンド送信
     *
     * COMMAND_REG に CMD_READ を書き込む。
     * 実際のハードウェアでは、この store 命令がバス経由でデバイスに届き、
     * デバイスのコマンドプロセッサが動き始める。
     */
    *command_reg = CMD_READ;
    printf("command: CMD_READ sent\n");

    /*
     * ステップ 2: デバイス動作のシミュレート
     *
     * 実際にはデバイスが非同期に処理するが、
     * ここではシングルスレッドでシミュレートする。
     */
    simulate_device((volatile uint32_t *)mmio_base);

    /*
     * ステップ 3: ステータスポーリング
     *
     * STATUS_REG を繰り返し読み出して、DONE になるまで待つ。
     * このループが正しく動くには volatile が必須。
     *
     * volatile がない場合:
     *   コンパイラが *status_reg の読み出しを 1 回にまとめ、
     *   ループが永久に回り続ける（または即座に抜ける）可能性がある。
     *
     * volatile がある場合:
     *   毎回メモリから読み直すため、デバイスがステータスを更新すれば検出できる。
     */
    printf("polling status...\n");
    while (*status_reg != STATUS_DONE) {
        /* ビジーウェイト（実際のドライバでは pause 命令や yield を入れる） */
    }

    /*
     * ステップ 4: データ読み出し
     *
     * STATUS が DONE になったので、DATA_REG からデバイスの応答を読み出す。
     */
    uint32_t data = *data_reg;
    printf("status: DONE\n");
    printf("data: 0x%X\n", data);

    /* MMIO 領域を解放 */
    munmap(mmio_base, MMIO_REGION_SIZE);

    return 0;
}
