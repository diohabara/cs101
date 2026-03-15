; boot.asm — Multiboot ブートストラップ + 32ビット → 64ビットモード遷移
;
; QEMU の -kernel オプションで ELF をロードし、PVH (Para-Virtualized Hardware)
; プロトコルで 32ビット保護モードからエントリポイントを呼び出す。
;
; このコードで:
;   1. ページテーブルを構築（Identity mapping: 仮想アドレス = 物理アドレス）
;   2. PAE + Long Mode + Paging を有効化
;   3. 64ビット GDT をロード
;   4. 64ビットモードにジャンプ
;   5. 埋め込まれた 64ビットカーネルコードを実行
;
; ビルド手順:
;   1. C コードを 64ビット ELF にコンパイル・リンク
;   2. llvm-objcopy で フラットバイナリに変換 → build/kernel_code.bin
;   3. この boot.asm が incbin でそのバイナリを埋め込む
;   4. nasm -f elf32 でアセンブル → 32ビット ELF
;   5. ld -m elf_i386 でリンク → QEMU の PVH ローダが読める 32ビット ELF

bits 32

; ── Multiboot1 ヘッダ ─────────────────────────────────────
; QEMU が ELF をロードする際に Multiboot ヘッダを認識する。
; マジックナンバー + フラグ + チェックサムの合計が 0 になること。
section .multiboot
align 4
    dd 0x1BADB002                           ; magic number
    dd 0x00000000                           ; flags
    dd -(0x1BADB002 + 0x00000000)           ; checksum

; ── PVH ELF Note ──────────────────────────────────────────
; QEMU 7.2+ では PVH プロトコルの ELF Note が必要。
; Xen の XEN_ELFNOTE_PHYS32_ENTRY (type=18) で 32ビットエントリを指定する。
section .note
align 4
    dd 4                ; namesz ("Xen" + null = 4 bytes)
    dd 4                ; descsz (32-bit address = 4 bytes)
    dd 18               ; type = XEN_ELFNOTE_PHYS32_ENTRY
    db "Xen", 0         ; name
    dd _entry32          ; 32-bit physical entry point

; ── 32ビットブートストラップコード ─────────────────────────
section .text
global _entry32

_entry32:
    ; 割り込み禁止
    cli

    ; スタック設定
    mov esp, stack_top

    ;
    ; ページテーブル構築（Identity mapping）
    ;
    ; x86_64 のページング: 4 段階のテーブル
    ;   PML4 → PDPT → PD → PT（または 2MB 大ページ）
    ;
    ; ここでは PD の各エントリを 2MB ページ（PS=1）にして、
    ; 最初の 1GB を identity map する（仮想アドレス = 物理アドレス）。
    ;

    ; ページテーブル領域をゼロクリア
    mov edi, pml4
    xor eax, eax
    mov ecx, (4096 * 3) / 4    ; PML4 + PDPT + PD = 12KB
    rep stosd

    ; PML4[0] → PDPT のアドレス | Present | Writable
    mov dword [pml4], pdpt
    or  dword [pml4], 0x03

    ; PDPT[0] → PD のアドレス | Present | Writable
    mov dword [pdpt], pd
    or  dword [pdpt], 0x03

    ; PD: 512 エントリ × 2MB = 1GB をマッピング
    mov ecx, 0
    mov eax, 0x83               ; Present | Writable | Page Size (2MB)
.map_pd:
    mov [pd + ecx * 8], eax
    add eax, 0x200000           ; 次の 2MB ページ
    inc ecx
    cmp ecx, 512
    jne .map_pd

    ;
    ; ロングモードへの遷移
    ;

    ; CR3 に PML4 のアドレスを設定
    mov eax, pml4
    mov cr3, eax

    ; PAE (Physical Address Extension) を有効化
    mov eax, cr4
    or  eax, (1 << 5)          ; CR4.PAE
    mov cr4, eax

    ; EFER.LME (Long Mode Enable) を設定
    mov ecx, 0xC0000080         ; IA32_EFER MSR
    rdmsr
    or  eax, (1 << 8)          ; LME ビット
    wrmsr

    ; ページングを有効化 → ロングモードがアクティブになる
    mov eax, cr0
    or  eax, (1 << 31)         ; CR0.PG
    mov cr0, eax

    ; 64ビット GDT をロードして far jump
    lgdt [gdt64_ptr]
    jmp 0x08:realm64

; ── 64ビットモードのエントリ ──────────────────────────────
bits 64
realm64:
    ; データセグメントレジスタを Kernel Data セレクタ (0x10) に設定
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 64ビットスタックポインタを設定
    lea rsp, [stack_top]

    ; 埋め込まれた 64ビットカーネルコードにジャンプ
    ; kernel_code ラベルは .text.boot セクションの先頭（= kernel_main のアドレス）
    mov rax, kernel_code
    call rax

    ; kernel_main() から戻ったら hlt ループ
.halt:
    hlt
    jmp .halt

; ── 初期 GDT（64ビットモード用、最小構成） ────────────────
; kernel_main() 内の gdt_init() で本格的な GDT に置き換える
section .rodata
align 16
gdt64:
    dq 0                        ; [0] Null descriptor
    dq 0x00AF9A000000FFFF       ; [1] Code64: Long mode, Present, DPL=0, Exec+Read
    dq 0x00CF92000000FFFF       ; [2] Data: Present, DPL=0, Read+Write
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1   ; limit
    dd gdt64                    ; base (32-bit address, OK for early boot)

; ── 64ビットカーネルコード（フラットバイナリとして埋め込み） ──
section .kernel
align 4096
kernel_code:
    incbin "build/kernel_code.bin"

; ── ページテーブル（BSS、4KB アラインメント） ─────────────
section .bss
align 4096
pml4: resb 4096                 ; Page Map Level 4
pdpt: resb 4096                 ; Page Directory Pointer Table
pd:   resb 4096                 ; Page Directory

; ── スタック（16KB） ──────────────────────────────────────
align 16
resb 16384                      ; 16KB カーネルスタック
stack_top:
