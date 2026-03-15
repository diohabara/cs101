/*
 * linker.ld --- カーネルリンカスクリプト
 *
 * QEMU の -kernel オプションはフラットバイナリを 0x100000 (1MB) にロードする。
 * このスクリプトで各セクションのレイアウトを定義する。
 *
 * メモリレイアウト:
 *   0x000000 - 0x0FFFFF : BIOSとレガシーデバイス用（使わない）
 *   0x100000 -          : カーネルイメージ
 *     .text.boot        : エントリポイント（_start）
 *     .text             : コード
 *     .rodata           : 読み取り専用データ（文字列リテラル等）
 *     .data             : 初期化済みデータ
 *     .bss              : ゼロ初期化データ
 */

ENTRY(_start)

SECTIONS {
    . = 0x100000;

    .text : {
        *(.text.boot)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
}
