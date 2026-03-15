# Delta

- v25 までの全コンポーネント（UART, GDT, IDT, タイマー, PMM, ページング, syscall, DMA）を統合
- 新しい概念: 協調的マルチタスク, ラウンドロビンスケジューリング, タスク制御ブロック (TCB)
- 新しいファイル: kernel.h, kernel.c, task.c
- Phase 2 の最終段階: ベアメタルから OS カーネルまでの全体像が完成
