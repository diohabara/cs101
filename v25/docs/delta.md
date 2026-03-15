# Delta

- v24 の syscall 機構の上に DMA コントローラを追加
- 新しい概念: DMA デスクリプタリング, head/tail ポインタ, ゼロコピー転送
- 新しいファイル: dma_hw.h, dma_hw.c
- PMM で確保したページを DMA バッファとして使用し、データ転送と検証を実施
