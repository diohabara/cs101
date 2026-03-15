# Delta

- VMCS (Virtual Machine Control Structure) の初期化を追加
- 新しい概念: VMCS, vmclear, vmptrld, vmwrite, ゲスト状態, ホスト状態, VM 実行制御
- 新しいファイル: vmcs.h, vmcs.c
- ゲスト/ホストの CPU 状態と VM exit 発生条件を設定
- VMX 非対応環境ではシミュレーション出力にフォールバック
