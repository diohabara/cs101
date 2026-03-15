/*
 * volatile_io.c — volatile キーワードの効果
 *
 * volatile は「この変数へのアクセスを最適化で省略するな」という指示。
 * ハードウェアレジスタ（MMIO）では、読み書きそのものが副作用を持つため、
 * コンパイラが「どうせ同じ値だろう」と省略すると正しく動かない。
 *
 * このプログラムでは:
 * 1. volatile int に値を書き込む
 * 2. 別の値で上書きする
 * 3. 読み出して exit code で返す
 *
 * -O2 最適化でも volatile があれば全ての読み書きが実行される。
 * volatile がなければコンパイラが中間の書き込みを省略する可能性がある。
 */

#include <stdlib.h>

int main(void) {
    volatile int device_reg = 0;

    /* 最初の書き込み — 実デバイスならコマンド送信に相当 */
    device_reg = 10;

    /* 上書き — 別のコマンド */
    device_reg = 20;

    /* 読み出し — ステータス確認に相当 */
    int status = device_reg;

    /* 10 + 20 = 30 ではなく、最後に書いた 20 が読める */
    return status;
}
