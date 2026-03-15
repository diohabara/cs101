# Delta

- v1-v8 のアセンブリ概念を C 言語で再表現する Phase 1 開始
- 新しい言語: C (gcc -std=c11)
- 新しい概念: ポインタ間接参照、volatile、ビット演算
- アセンブリとの対応: mov [rax],42 → *ptr=42、RFLAGS → ビットフラグ、load/store → ポインタ読み書き
