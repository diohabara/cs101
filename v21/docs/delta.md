# Delta

- v19 の GDT の上に IDT、PIC、PIT を追加
- 新しい概念: IDT, 割り込みゲート, PIC (8259A) リマッピング, PIT タイマー, ISR スタブ
- 新しいファイル: idt.c, idt.h, timer.c, timer.h
