/*
 * vmx.h --- VMX (Virtual Machine Extensions) 定義
 *
 * Intel VT-x (VMX) はハードウェア仮想化支援機能。
 * CPU が「ホスト」と「ゲスト」の 2 つの実行モードを持ち、
 * ゲストの特権命令や I/O を自動的にトラップして
 * ホスト（ハイパーバイザ）に制御を返す。
 *
 * VMX の有効化手順:
 *   1. CPUID.1:ECX[5] で VMX 対応を確認
 *   2. IA32_FEATURE_CONTROL MSR で VMX が BIOS から許可されているか確認
 *   3. CR4.VMXE ビットを設定して VMX 命令を有効化
 *   4. VMXON 領域を準備し、vmxon 命令で VMX モードに入る
 */

#ifndef VMX_H
#define VMX_H

#include <stdint.h>

/* CPUID leaf 1, ECX bit 5 = VMX support */
#define CPUID_VMX_BIT (1 << 5)

/* CR4 bit 13 = VMXE (VMX 命令の有効化) */
#define CR4_VMXE (1UL << 13)

/*
 * IA32_FEATURE_CONTROL MSR (0x3A)
 *
 * BIOS が VMX の有効/無効を制御する MSR。
 * bit 0: Lock --- 1 で MSR がロック（以後変更不可）
 * bit 2: Enable VMXON outside SMX --- 1 で通常モードでの VMXON を許可
 */
#define MSR_IA32_FEATURE_CONTROL 0x3A
#define FEATURE_CONTROL_LOCKED   (1UL << 0)
#define FEATURE_CONTROL_VMXON    (1UL << 2)

/*
 * VMXON 領域 --- vmxon 命令に渡す 4KB の制御構造
 *
 * 先頭 4 バイトに VMCS リビジョン ID を書き込む必要がある。
 * リビジョン ID は IA32_VMX_BASIC MSR (0x480) のビット 30:0 から取得する。
 * 4KB アラインメント必須。
 */
struct vmxon_region {
    uint32_t revision_id;
    uint8_t  data[4092];
} __attribute__((packed, aligned(4096)));

/*
 * vmx_detect --- CPUID で VT-x サポートを検出
 *
 * 戻り値: 1 = VT-x 対応, 0 = 非対応
 */
int vmx_detect(void);

/*
 * vmx_enable --- VMX モードに入る
 *
 * MSR チェック → CR4 設定 → VMXON 実行
 * 戻り値: 0 = 成功, -1 = 失敗
 */
int vmx_enable(void);

/*
 * vmx_disable --- VMX モードから出る
 *
 * vmxoff 命令で VMX を無効化し、CR4.VMXE をクリアする。
 */
void vmx_disable(void);

#endif /* VMX_H */
