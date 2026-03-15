# Delta

- VM exit reason に応じたハンドリングを追加
- 新しい概念: exit reason (CPUID/HLT/I/O), exit qualification, vmresume, 仮想デバイスエミュレーション
- 新しいファイル: vmexit.h, vmexit_handler.c
- CPUID exit → フィルタ結果返却、HLT exit → 停止判断、I/O exit → 仮想デバイスエミュレーション
- VMX 非対応環境では VM exit ハンドリングのデモ出力を追加
