/*
 * linker_code.ld — 64ビット C カーネルコードのリンカスクリプト
 *
 * C のカーネルコードをフラットバイナリに変換するための中間リンクに使う。
 * boot.asm が incbin でこのバイナリを埋め込む。
 *
 * .text.boot セクション（kernel_main）が先頭に来るように配置する。
 * boot.asm は kernel_code ラベル（= このバイナリの先頭）を call する。
 */

ENTRY(kernel_main)

SECTIONS {
    /* boot.asm の .kernel セクションが 0x101000 に配置される */
    . = 0x101000;

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

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
