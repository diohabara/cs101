# Delta

- MMIO（Memory-Mapped I/O）デバイスアクセスのシミュレーション
- 新しい概念: MMIO レジスタレイアウト、コマンド/ステータス/データレジスタ、ステータスポーリング、LED ビットパターン
- mmap で匿名メモリを MMIO 領域として使用
- volatile ポインタ経由の読み書きで最適化を防止（v9 の volatile 概念の実践応用）
- COMMAND_REG → STATUS_REG ポーリング → DATA_REG 読み出しのデバイスアクセスパターン
- LED コントローラの MMIO シミュレーション（ビットごとの LED 制御と読み戻し）
