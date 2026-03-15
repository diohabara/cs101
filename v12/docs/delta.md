# Delta

- ucontext API でユーザー空間コンテキストスイッチを実装
- 新しい概念: Round-Robin スケジューリング、優先度スケジューリング、コンテキストスイッチ
- v4 の push/pop との対応: 個別レジスタ退避 → swapcontext による全レジスタ＋スタック一括切替
- ucontext_t: getcontext/makecontext/swapcontext の 3 段階でタスクを生成・切替
