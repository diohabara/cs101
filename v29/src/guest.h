/*
 * guest.h --- ゲスト VM の定義
 *
 * ゲスト VM はハイパーバイザが管理する仮想マシン。
 * vmlaunch 命令でゲストコードの実行を開始し、
 * 特権命令や HLT で VM exit が発生してホストに制御が返る。
 *
 * v29 では最小限のゲスト:
 *   mov rax, 0x42
 *   hlt
 * を実行し、HLT による VM exit をハンドリングする。
 */

#ifndef GUEST_H
#define GUEST_H

#include <stdint.h>

/*
 * guest_run --- ゲスト VM を実行する
 *
 * VMX 対応環境: VMCS にゲストコードを設定して vmlaunch
 * VMX 非対応環境: シミュレーション出力
 *
 * 戻り値: 0 = 成功, -1 = 失敗/スキップ
 */
int guest_run(void);

/*
 * guest_get_code --- ゲストコードのアドレスを取得
 *
 * ゲストが実行するマシンコードのバイト列を返す。
 */
const uint8_t *guest_get_code(void);

/*
 * guest_get_code_size --- ゲストコードのサイズを取得
 */
uint32_t guest_get_code_size(void);

#endif /* GUEST_H */
