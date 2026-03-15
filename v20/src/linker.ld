/*
 * linker.ld — 最終カーネルリンカスクリプト（32ビット ELF コンテナ）
 *
 * boot.asm（Multiboot + PVH + 32->64遷移 + 埋め込みカーネル）をリンクする。
 * QEMU の PVH ローダが 32ビット ELF を要求するため、elf_i386 で出力する。
 *
 * メモリレイアウト:
 *   0x100000 -          : カーネルイメージ
 *     .multiboot        : Multiboot ヘッダ
 *     .text             : ブートストラップコード（32ビット -> 64ビット遷移）
 *     .rodata           : 初期 GDT 等
 *     .kernel           : 64ビットカーネルコード（フラットバイナリ）
 *     .bss              : ページテーブル + スタック
 *
 * PHDRS で PT_NOTE セグメントを明示的に定義する。
 * QEMU 7.2+ がこの NOTE セグメントから PVH エントリポイントを読み取る。
 */

ENTRY(_entry32)

PHDRS {
    text PT_LOAD FLAGS(5);
    note PT_NOTE FLAGS(4);
}

SECTIONS {
    . = 0x100000;

    .multiboot : { *(.multiboot) } :text
    .text : { *(.text .text.*) } :text
    .note : { *(.note) } :note
    .rodata : { *(.rodata .rodata.*) } :text
    .kernel : { *(.kernel) } :text
    .data : { *(.data .data.*) } :text
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :text
}
