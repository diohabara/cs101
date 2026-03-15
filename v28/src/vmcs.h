/*
 * vmcs.h --- VMCS (Virtual Machine Control Structure) 定義
 *
 * VMCS は VMX のコアデータ構造。ゲスト/ホストの CPU 状態と
 * VM 実行制御パラメータを格納する 4KB の領域。
 *
 * VMCS には以下の情報が含まれる:
 *   - ゲスト状態: RIP, RSP, CR0, CR3, CR4, セグメントレジスタ等
 *   - ホスト状態: VM exit 時に復元される RIP, RSP, CR0 等
 *   - VM 実行制御: どの操作で VM exit を発生させるか
 *   - VM exit 情報: exit の理由、命令長、修飾情報
 *
 * VMCS のフィールドは vmread/vmwrite 命令でアクセスする。
 * 各フィールドはエンコーディング番号で識別される。
 */

#ifndef VMCS_H
#define VMCS_H

#include <stdint.h>

/*
 * VMCS 領域 --- 4KB アラインメント必須
 *
 * VMXON 領域と同様に先頭 4 バイトが VMCS リビジョン ID。
 */
struct vmcs_region {
    uint32_t revision_id;
    uint32_t abort_indicator;
    uint8_t  data[4088];
} __attribute__((packed, aligned(4096)));

/* --- VMCS フィールドエンコーディング --- */

/*
 * ゲスト状態フィールド
 * vmwrite でゲストの初期 CPU 状態を設定する。
 */
#define VMCS_GUEST_ES_SELECTOR     0x0800
#define VMCS_GUEST_CS_SELECTOR     0x0802
#define VMCS_GUEST_SS_SELECTOR     0x0804
#define VMCS_GUEST_DS_SELECTOR     0x0806
#define VMCS_GUEST_FS_SELECTOR     0x0808
#define VMCS_GUEST_GS_SELECTOR     0x080A
#define VMCS_GUEST_LDTR_SELECTOR   0x080C
#define VMCS_GUEST_TR_SELECTOR     0x080E

#define VMCS_GUEST_CR0             0x6800
#define VMCS_GUEST_CR3             0x6802
#define VMCS_GUEST_CR4             0x6804
#define VMCS_GUEST_RSP             0x681C
#define VMCS_GUEST_RIP             0x681E
#define VMCS_GUEST_RFLAGS          0x6820
#define VMCS_GUEST_DR7             0x681A

/* ゲストセグメントのリミット、アクセス権、ベース */
#define VMCS_GUEST_ES_LIMIT        0x4800
#define VMCS_GUEST_CS_LIMIT        0x4802
#define VMCS_GUEST_SS_LIMIT        0x4804
#define VMCS_GUEST_DS_LIMIT        0x4806
#define VMCS_GUEST_FS_LIMIT        0x4808
#define VMCS_GUEST_GS_LIMIT        0x480A
#define VMCS_GUEST_LDTR_LIMIT      0x480C
#define VMCS_GUEST_TR_LIMIT        0x480E
#define VMCS_GUEST_GDTR_LIMIT      0x4810
#define VMCS_GUEST_IDTR_LIMIT      0x4812

#define VMCS_GUEST_ES_ACCESS       0x4814
#define VMCS_GUEST_CS_ACCESS       0x4816
#define VMCS_GUEST_SS_ACCESS       0x4818
#define VMCS_GUEST_DS_ACCESS       0x481A
#define VMCS_GUEST_FS_ACCESS       0x481C
#define VMCS_GUEST_GS_ACCESS       0x481E
#define VMCS_GUEST_LDTR_ACCESS     0x4820
#define VMCS_GUEST_TR_ACCESS       0x4822

#define VMCS_GUEST_ES_BASE         0x6806
#define VMCS_GUEST_CS_BASE         0x6808
#define VMCS_GUEST_SS_BASE         0x680A
#define VMCS_GUEST_DS_BASE         0x680C
#define VMCS_GUEST_FS_BASE         0x680E
#define VMCS_GUEST_GS_BASE         0x6810
#define VMCS_GUEST_LDTR_BASE       0x6812
#define VMCS_GUEST_TR_BASE         0x6814
#define VMCS_GUEST_GDTR_BASE       0x6816
#define VMCS_GUEST_IDTR_BASE       0x6818

/* ゲストリンク、IA32_DEBUGCTL 等 */
#define VMCS_GUEST_VMCS_LINK       0x2800
#define VMCS_GUEST_DEBUGCTL        0x2802
#define VMCS_GUEST_ACTIVITY_STATE  0x4826
#define VMCS_GUEST_INTERRUPTIBILITY 0x4824

/*
 * ホスト状態フィールド
 * VM exit 時に CPU が復元する値。
 */
#define VMCS_HOST_ES_SELECTOR      0x0C00
#define VMCS_HOST_CS_SELECTOR      0x0C02
#define VMCS_HOST_SS_SELECTOR      0x0C04
#define VMCS_HOST_DS_SELECTOR      0x0C06
#define VMCS_HOST_FS_SELECTOR      0x0C08
#define VMCS_HOST_GS_SELECTOR      0x0C0A
#define VMCS_HOST_TR_SELECTOR      0x0C0C

#define VMCS_HOST_CR0              0x6C00
#define VMCS_HOST_CR3              0x6C02
#define VMCS_HOST_CR4              0x6C04
#define VMCS_HOST_RSP              0x6C14
#define VMCS_HOST_RIP              0x6C16

#define VMCS_HOST_IA32_SYSENTER_CS  0x4C00
#define VMCS_HOST_IA32_SYSENTER_ESP 0x6C10
#define VMCS_HOST_IA32_SYSENTER_EIP 0x6C12

#define VMCS_HOST_GDTR_BASE        0x6C0C
#define VMCS_HOST_IDTR_BASE        0x6C0E
#define VMCS_HOST_FS_BASE          0x6C06
#define VMCS_HOST_GS_BASE          0x6C08
#define VMCS_HOST_TR_BASE          0x6C0A

/*
 * VM 実行制御フィールド
 * どの操作で VM exit を発生させるかを制御する。
 */
#define VMCS_CTRL_PIN_BASED        0x4000
#define VMCS_CTRL_PROC_BASED       0x4002
#define VMCS_CTRL_PROC_BASED2      0x401E
#define VMCS_CTRL_EXIT             0x400C
#define VMCS_CTRL_ENTRY            0x4012
#define VMCS_CTRL_EXCEPTION_BITMAP 0x4004

/* VM exit 情報 */
#define VMCS_EXIT_REASON           0x4402
#define VMCS_EXIT_QUALIFICATION    0x6400
#define VMCS_EXIT_INSTR_LENGTH     0x440C
#define VMCS_EXIT_INSTR_INFO       0x440E

/* VM entry 制御 */
#define VMCS_CTRL_ENTRY_MSR_LOAD_COUNT 0x4014
#define VMCS_CTRL_EXIT_MSR_LOAD_COUNT  0x4010
#define VMCS_CTRL_EXIT_MSR_STORE_COUNT 0x400E

/*
 * vmcs_init --- VMCS を初期化する
 *
 * VMX が利用可能な場合: vmclear + vmptrld + vmwrite でフィールド設定
 * VMX が利用不可の場合: シミュレーション出力
 *
 * 戻り値: 0 = 成功, -1 = スキップ
 */
int vmcs_init(void);

/*
 * vmcs_dump --- VMCS の主要フィールドをダンプする（教育用）
 */
void vmcs_dump(void);

#endif /* VMCS_H */
