# Delta

- Phase 2（ベアメタルカーネル）の開始
- 新しいビルド環境: clang -ffreestanding -nostdlib + ld.lld + QEMU
- 新しい概念: ベアメタル実行、UART 16550A MMIO、リンカスクリプト、I/Oポート
- v15 の MMIO シミュレーション → 実際のハードウェアレジスタ操作
