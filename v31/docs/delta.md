# Delta

- EPT (Extended Page Tables) によるゲスト間メモリ隔離を追加
- Phase 3（ハイパーバイザ編）を統合完成
- 新しい概念: EPT, GPA→HPA 変換, メモリ隔離, EPT violation, 2 段階アドレス変換
- 新しいファイル: ept.h, ept.c, hypervisor.c
- v27-v31 の全コンポーネント（VMX, VMCS, Guest, VM exit, EPT）を統合
