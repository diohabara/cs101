# Delta

- Phase 1 総集編: メモリ内 inode ファイルシステム + パイプ/リダイレクト付きミニシェル
- 新しい概念: inode, ディレクトリエントリ, fd テーブル, pipe(), dup2(), fork+exec
- Aha: パイプもファイルも fd テーブルのエントリ。`ls | grep foo > out.txt` は fd の付け替え
- v9 の C 言語基礎の上に、UNIX の核心である「すべてはファイル」を実装
