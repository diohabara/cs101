/*
 * vmexit.h --- VM exit ハンドリング定義
 *
 * VM exit はゲストが特定の操作を実行したときに CPU が自動的に
 * ホストに制御を返す仕組み。exit reason で理由を判別し、
 * 適切な処理を行ってからゲストに復帰する。
 *
 * 主な VM exit reason:
 *   10 (CPUID)     --- ゲストが CPUID 命令を実行
 *   12 (HLT)       --- ゲストが HLT 命令を実行
 *   30 (I/O)       --- ゲストが IN/OUT 命令を実行
 *   48 (EPT violation) --- ゲストが無効な物理アドレスにアクセス（v31）
 */

#ifndef VMEXIT_H
#define VMEXIT_H

#include <stdint.h>

/* VM exit reasons */
#define VMEXIT_REASON_CPUID         10
#define VMEXIT_REASON_HLT           12
#define VMEXIT_REASON_IO            30
#define VMEXIT_REASON_EPT_VIOLATION 48

/*
 * vmexit_handler --- VM exit を処理する
 *
 * exit reason に応じて適切な処理を行い、ゲストに復帰するか停止する。
 *
 * 戻り値: 0 = ゲストを再開 (vmresume), 1 = 停止
 */
int vmexit_handler(void);

/*
 * vmexit_demo --- VM exit ハンドリングのデモ（シミュレーション用）
 *
 * VMX 非対応環境で VM exit の概念を示す。
 */
void vmexit_demo(void);

#endif /* VMEXIT_H */
