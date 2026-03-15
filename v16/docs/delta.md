# Delta

- DMA (Direct Memory Access) の概念を C でシミュレーション
- 新しい概念: リングバッファ（循環バッファ）、DMA ディスクリプタ、ポーリング
- NVMe との対応: リングバッファ → Submission/Completion Queue、ディスクリプタ → SQE/CQE
- v9 のポインタ・ビット演算を発展させ、デバイスとメモリの関係を学ぶ
