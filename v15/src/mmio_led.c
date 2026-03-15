/*
 * mmio_led.c --- MMIO による LED コントローラのシミュレーション
 *
 * 組み込みシステムでは、LED の ON/OFF もメモリマップドレジスタへの
 * 書き込みで制御する。各ビットが 1 つの LED に対応し、
 * ビットを 1 にすれば点灯、0 にすれば消灯する。
 *
 * このプログラムでは:
 *   1. mmap で LED コントローラの MMIO 領域を確保
 *   2. 3 つのパターンを順に書き込む
 *      - 0xF: 全 LED 点灯
 *      - 0x5: 交互点灯（LED1, LED3）
 *      - 0x0: 全 LED 消灯
 *   3. 各パターンを読み戻して ASCII アートで表示
 *
 * 読み戻し（read-back）の意味:
 *   MMIO では「書き込んだ値」と「読み戻した値」が異なる場合がある。
 *   デバイスが値を変更することがあるため、書き込み後に読み戻して
 *   実際の状態を確認するのが安全なプラクティス。
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include "mmio.h"

/*
 * print_leds --- LED の状態を ASCII アートで表示
 *
 * 各ビットを確認し、1 なら [*]（点灯）、0 なら [ ]（消灯）を表示する。
 *
 * pattern: LED ビットパターン（下位 4 ビットを使用）
 */
static void print_leds(uint32_t pattern) {
    printf("pattern 0x%X: ", pattern);
    for (int i = LED_COUNT - 1; i >= 0; i--) {
        if (pattern & (1U << i)) {
            printf("[*]");
        } else {
            printf("[ ]");
        }
    }
    printf("\n");
}

int main(void) {
    /*
     * LED コントローラの MMIO 領域を mmap で確保する。
     *
     * 実際の組み込みシステムでは、LED コントローラは固定の物理アドレス
     * （例: 0x40021000）にマッピングされている。ここでは匿名メモリで代用する。
     */
    void *led_base = mmap(
        NULL,
        LED_REGION_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0
    );
    if (led_base == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(led_base, 0, LED_REGION_SIZE);

    /*
     * volatile ポインタでレジスタにアクセスする。
     *
     * LED_CONTROL_REG: パターンを書き込むレジスタ
     * LED_STATUS_REG:  現在の LED 状態を読み出すレジスタ
     *
     * このシミュレーションでは CONTROL に書いた値がそのまま STATUS に
     * 反映される（実ハードウェアでもこのタイプのレジスタは多い）。
     */
    volatile uint32_t *control_reg = (volatile uint32_t *)((char *)led_base + LED_CONTROL_REG_OFFSET);
    volatile uint32_t *status_reg  = (volatile uint32_t *)((char *)led_base + LED_STATUS_REG_OFFSET);

    /*
     * 表示する LED パターンの配列。
     *
     * 0xF = 1111b: 全 LED 点灯
     * 0xA = 1010b: LED1 と LED3 が点灯（交互パターン）
     * 0x0 = 0000b: 全 LED 消灯
     */
    uint32_t patterns[] = { LED_ALL, LED1 | LED3, 0x0 };
    int num_patterns = sizeof(patterns) / sizeof(patterns[0]);

    for (int i = 0; i < num_patterns; i++) {
        /*
         * ステップ 1: LED パターンを CONTROL_REG に書き込む
         *
         * この store 命令が、実際のハードウェアでは LED コントローラに届き、
         * 各 LED の ON/OFF を切り替える。
         */
        *control_reg = patterns[i];

        /*
         * ステップ 2: デバイスのシミュレート
         *
         * 実際のハードウェアでは CONTROL に書いた値が自動的に反映される。
         * このシミュレーションでは手動で STATUS に反映する。
         */
        *status_reg = *control_reg;

        /*
         * ステップ 3: STATUS_REG から読み戻して表示
         *
         * 書き込んだ値ではなく、読み戻した値を使うのが MMIO のベストプラクティス。
         * デバイスが値を変更する可能性があるため。
         */
        uint32_t readback = *status_reg;
        print_leds(readback);
    }

    munmap(led_base, LED_REGION_SIZE);

    return 0;
}
