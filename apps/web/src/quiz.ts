export type QuizOption = {
  id: string;
  label: string;
  isCorrect: boolean;
};

export type QuizQuestion = {
  id: string;
  prompt: string;
  type: "single" | "multiple";
  options: QuizOption[];
  explanation: string;
};

export type StageQuiz = {
  title: string;
  intro: string;
  questions: QuizQuestion[];
  resultMessages: {
    all_correct: string;
    partially_correct: string;
    needs_review: string;
  };
};

export const stageQuizzes: Record<string, StageQuiz> = {
  v1: {
    title: "理解確認クイズ",
    intro: "CPU の最小状態とレジスタの役割を思い出しながら答えてください。",
    questions: [
      {
        id: "rip-role",
        prompt: "RIP が保持しているものはどれですか。",
        type: "single",
        options: [
          { id: "next-instruction", label: "次に実行する命令のアドレス", isCorrect: true },
          { id: "exit-code", label: "直前の終了コード", isCorrect: false },
          { id: "stack-base", label: "スタックの先頭アドレス", isCorrect: false },
        ],
        explanation: "RIP は命令ポインタで、次に実行する命令のアドレスを指します。",
      },
      {
        id: "register-characteristics",
        prompt: "レジスタについて正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "fast", label: "CPU 内部にあり、メモリより高速にアクセスできる", isCorrect: true },
          { id: "named", label: "`rax` や `rbx` のように名前で参照する", isCorrect: true },
          { id: "infinite", label: "必要に応じて無制限に増やせる", isCorrect: false },
          { id: "shared", label: "すべての命令で常に同じ値を共有する", isCorrect: false },
        ],
        explanation: "レジスタは CPU 内部の少数・高速な保存場所で、名前付きで扱います。",
      },
      {
        id: "syscall-exit42",
        prompt: "`exit42.asm` で `syscall` の直前に入っている値の組み合わせはどれですか。",
        type: "single",
        options: [
          { id: "60-42", label: "`rax=60`, `rdi=42`", isCorrect: true },
          { id: "42-60", label: "`rax=42`, `rdi=60`", isCorrect: false },
          { id: "1-42", label: "`rax=1`, `rdi=42`", isCorrect: false },
        ],
        explanation: "`sys_exit` の番号 60 を `rax` に、終了コード 42 を `rdi` に入れてから `syscall` します。",
      },
    ],
    resultMessages: {
      all_correct: "CPU の最小状態は掴めています。次は命令ごとの差分を意識して読めています。",
      partially_correct: "基本は押さえられています。RIP と syscall ABI の節をもう一度見直すと安定します。",
      needs_review: "まだ土台が曖昧です。レジスタ、RIP、`syscall` 前の状態遷移表を先に復習してください。",
    },
  },
  v2: {
    title: "理解確認クイズ",
    intro: "`cmp` と条件分岐の読み方を確認します。",
    questions: [
      {
        id: "cmp-meaning",
        prompt: "`cmp rax, 42` の説明として正しいものはどれですか。",
        type: "single",
        options: [
          { id: "flags-only", label: "`rax - 42` を計算し、結果は捨てて flags だけを更新する", isCorrect: true },
          { id: "writeback", label: "`rax` に 42 を書き込んでから比較する", isCorrect: false },
          { id: "swap", label: "`rax` と即値 42 を入れ替える", isCorrect: false },
        ],
        explanation: "`cmp` は内部的には減算しますが、レジスタへは書き戻しません。後続の分岐に使うために flags だけを更新します。",
      },
      {
        id: "signed-branch-rules",
        prompt: "signed 比較の条件として正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "jg", label: "`jg` は `ZF=0` かつ `SF=OF` を見る", isCorrect: true },
          { id: "jl", label: "`jl` は `SF!=OF` を見る", isCorrect: true },
          { id: "ja", label: "`ja` は `CF=1` なら成立する", isCorrect: false },
          { id: "jb", label: "`jb` は signed 比較で「小さい」を表す", isCorrect: false },
        ],
        explanation: "`jg` と `jl` は signed 比較です。`ja` と `jb` は unsigned 比較なので `CF` を中心に見ます。",
      },
      {
        id: "jump-alias",
        prompt: "`jz` と `je` の関係として正しいものはどれですか。",
        type: "single",
        options: [
          { id: "same-condition", label: "どちらも `ZF=1` を見る別名で、文脈に応じて名前を使い分ける", isCorrect: true },
          { id: "different-flags", label: "`jz` は `ZF`、`je` は `CF` を見る別命令", isCorrect: false },
          { id: "instruction-kind", label: "`jz` は条件ジャンプ、`je` は無条件ジャンプ", isCorrect: false },
        ],
        explanation: "`jz` と `je` は CPU から見れば同じ条件です。`sub` の直後なら `jz`、`cmp` の直後なら `je` のほうが人間には読みやすいことがあります。",
      },
    ],
    resultMessages: {
      all_correct: "`cmp` と条件分岐の関係を読めています。flags が後続の制御にどう効くかまで追えています。",
      partially_correct: "大筋は合っています。`cmp` が値を書き戻さないことと、signed/unsigned のジャンプ条件を見直すと安定します。",
      needs_review: "`cmp` と jump 命令の役割がまだ混ざっています。`ZF`/`SF`/`CF`/`OF` と `jz`/`jg`/`ja` の対応を先に整理してください。",
    },
  },
  v3: {
    title: "理解確認クイズ",
    intro: "メモリと flags のつながりを確認します。",
    questions: [
      {
        id: "store-load",
        prompt: "`store_load.asm` で観察したいこととして最も近いものはどれですか。",
        type: "single",
        options: [
          { id: "memory-roundtrip", label: "レジスタの値をメモリへ置き、再び読み戻せること", isCorrect: true },
          { id: "process-fork", label: "親子プロセスで値が分岐すること", isCorrect: false },
          { id: "file-open", label: "ファイルディスクリプタが増えること", isCorrect: false },
        ],
        explanation: "v3 ではレジスタだけでなく、メモリに書いた値を再度読む流れを観察します。",
      },
      {
        id: "flags-meaning",
        prompt: "flags について正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "result-reflect", label: "演算結果の性質を反映して更新される", isCorrect: true },
          { id: "branch-input", label: "後続の条件分岐の判断材料になる", isCorrect: true },
          { id: "fixed", label: "プログラム開始後は変化しない", isCorrect: false },
          { id: "memory-only", label: "メモリアクセス時にしか使われない", isCorrect: false },
        ],
        explanation: "flags は演算の結果に応じて更新され、その後の条件判断に使われます。",
      },
      {
        id: "array-sum",
        prompt: "`array_sum.asm` を追うときに重要なのはどれですか。",
        type: "single",
        options: [
          { id: "accumulate", label: "メモリ上の複数値を順に読み、累積結果を更新する流れ", isCorrect: true },
          { id: "syscall-order", label: "複数 syscall を並べる順序", isCorrect: false },
          { id: "stack-frame", label: "関数呼び出し時の return address", isCorrect: false },
        ],
        explanation: "配列和では load と加算が繰り返される流れを追うことが中心です。",
      },
    ],
    resultMessages: {
      all_correct: "メモリ操作と flags の対応を読めています。状態のつながりが見えています。",
      partially_correct: "方向性は合っています。flags がどの演算結果を表すかをもう一度確認してください。",
      needs_review: "レジスタとメモリの役割がまだ混ざっています。`store/load` と flags の節を優先して復習してください。",
    },
  },
  v4: {
    title: "理解確認クイズ",
    intro: "スタックと call/ret の最小動作を整理します。",
    questions: [
      {
        id: "push-pop",
        prompt: "`push` と `pop` の説明として正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "stack-use", label: "スタックを使って値を保存・復元する", isCorrect: true },
          { id: "rsp-change", label: "`rsp` の値が命令ごとに変化する", isCorrect: true },
          { id: "rip-fixed", label: "実行中も `rip` は変化しない", isCorrect: false },
          { id: "heap-only", label: "ヒープ領域だけを書き換える", isCorrect: false },
        ],
        explanation: "`push`/`pop` はスタックポインタとメモリ内容を更新しながら値を退避・復元します。",
      },
      {
        id: "call-ret",
        prompt: "`call` が自動で行うことはどれですか。",
        type: "single",
        options: [
          { id: "save-return", label: "戻り先アドレスをスタックに積んでジャンプする", isCorrect: true },
          { id: "syscall", label: "OS に処理を依頼する", isCorrect: false },
          { id: "allocate-page", label: "新しいページを確保する", isCorrect: false },
        ],
        explanation: "`call` は戻り先を保存してから呼び出し先へ制御を移します。",
      },
      {
        id: "nested-calls",
        prompt: "`nested_calls.asm` で特に確認したいことはどれですか。",
        type: "single",
        options: [
          { id: "return-order", label: "ネストした呼び出しが LIFO 順で戻ること", isCorrect: true },
          { id: "file-permission", label: "ファイル権限が変わること", isCorrect: false },
          { id: "signal-mask", label: "シグナルマスクが変わること", isCorrect: false },
        ],
        explanation: "複数の call が積まれると、return は最後に積んだものから順に戻ります。",
      },
    ],
    resultMessages: {
      all_correct: "スタックと call/ret の基本は掴めています。制御フローとデータ退避を結びつけて読めています。",
      partially_correct: "概ね理解できています。`call` が積む値と `rsp` の変化だけ再確認してください。",
      needs_review: "スタックの役割がまだ曖昧です。`push/pop` と `call/ret` の状態遷移表を見直してください。",
    },
  },
  v5: {
    title: "理解確認クイズ",
    intro: "syscall を連続して呼ぶときの読み方を確認します。",
    questions: [
      {
        id: "hello-syscall",
        prompt: "`hello.asm` で文字列出力に必要なものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "syscall-number", label: "`rax` に syscall 番号を入れる", isCorrect: true },
          { id: "buffer", label: "出力するバッファのアドレスを渡す", isCorrect: true },
          { id: "length", label: "出力バイト数を渡す", isCorrect: true },
          { id: "page-table", label: "ページテーブルを手動で編集する", isCorrect: false },
        ],
        explanation: "write 系 syscall では番号・出力先 fd・バッファ・長さが必要です。",
      },
      {
        id: "stderr",
        prompt: "`write_stderr.asm` が示している違いはどれですか。",
        type: "single",
        options: [
          { id: "fd-difference", label: "標準出力と標準エラーでは file descriptor が違う", isCorrect: true },
          { id: "cpu-mode", label: "CPU モードが 32bit に切り替わる", isCorrect: false },
          { id: "register-width", label: "レジスタ幅が 8bit になる", isCorrect: false },
        ],
        explanation: "stdout と stderr は別の file descriptor を使うため、同じ syscall でも宛先が変わります。",
      },
      {
        id: "multi-syscall",
        prompt: "`multi_syscall.asm` を読むときの観察ポイントとして最も重要なのはどれですか。",
        type: "single",
        options: [
          { id: "reload-registers", label: "syscall ごとに必要な引数レジスタを組み直すこと", isCorrect: true },
          { id: "keep-rax", label: "一度入れた `rax` は最後まで変えないこと", isCorrect: false },
          { id: "stack-frame", label: "必ず関数フレームを作ること", isCorrect: false },
        ],
        explanation: "連続する syscall では、その都度 `rax` と引数レジスタをセットし直します。",
      },
    ],
    resultMessages: {
      all_correct: "syscall の流れを正しく読めています。引数の組み直しも意識できています。",
      partially_correct: "大枠は理解できています。fd と引数レジスタの役割をもう一度整理してください。",
      needs_review: "syscall ABI の読み方がまだ不安定です。`hello.asm` と `write_stderr.asm` を先に復習してください。",
    },
  },
  v6: {
    title: "理解確認クイズ",
    intro: "ファイルディスクリプタと I/O の基礎を確認します。",
    questions: [
      {
        id: "stdin-read",
        prompt: "`read_stdin.asm` の主題として正しいものはどれですか。",
        type: "single",
        options: [
          { id: "read-stdin", label: "標準入力からバッファへデータを読む流れ", isCorrect: true },
          { id: "fork-process", label: "子プロセスに入力を分配する流れ", isCorrect: false },
          { id: "map-memory", label: "匿名メモリを確保する流れ", isCorrect: false },
        ],
        explanation: "標準入力から読み込んだバイト列をどこへ置くかが `read_stdin.asm` の中心です。",
      },
      {
        id: "fd-table",
        prompt: "file descriptor table について正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "integer-handle", label: "小さな整数で I/O 対象を表す", isCorrect: true },
          { id: "stdin-stdout-stderr", label: "0, 1, 2 は標準入力・出力・エラーに対応する", isCorrect: true },
          { id: "always-file-path", label: "常にファイルパス文字列そのものを保持する", isCorrect: false },
          { id: "cpu-register-only", label: "CPU レジスタの別名としてだけ存在する", isCorrect: false },
        ],
        explanation: "fd はカーネルが管理する I/O 対象へのハンドルで、0/1/2 は標準ストリームです。",
      },
      {
        id: "write-file",
        prompt: "`write_file.asm` を読むとき、どの順序が自然ですか。",
        type: "single",
        options: [
          { id: "open-write-close", label: "開く / 書く / 閉じる", isCorrect: true },
          { id: "close-open-read", label: "閉じる / 開く / 読む", isCorrect: false },
          { id: "fork-map-exec", label: "fork / mmap / execve", isCorrect: false },
        ],
        explanation: "ファイル書き込みでは fd を得てから書き込み、最後に閉じる流れを追います。",
      },
    ],
    resultMessages: {
      all_correct: "I/O と fd table の基礎は整理できています。OS との接点を順序立てて読めています。",
      partially_correct: "要点は掴めています。標準ストリーム番号と read/write の流れだけ見直してください。",
      needs_review: "I/O の基本順序がまだ曖昧です。`read_stdin` と `fd_table` の節を先に復習してください。",
    },
  },
  v7: {
    title: "理解確認クイズ",
    intro: "プロセス生成と実行イメージ切替の違いを確認します。",
    questions: [
      {
        id: "getpid",
        prompt: "`getpid.asm` が示しているものとして最も近いものはどれですか。",
        type: "single",
        options: [
          { id: "process-id", label: "実行中プロセス自身の PID を取得できること", isCorrect: true },
          { id: "memory-page", label: "現在のページサイズを取得できること", isCorrect: false },
          { id: "compiler-type", label: "型推論結果を取得できること", isCorrect: false },
        ],
        explanation: "`getpid` は現在のプロセス ID を返す syscall です。",
      },
      {
        id: "fork-basic",
        prompt: "`fork_basic.asm` について正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "parent-child", label: "`fork` 後は親と子の 2 つの実行流れができる", isCorrect: true },
          { id: "return-diff", label: "親と子で `fork` の戻り値の意味が違う", isCorrect: true },
          { id: "replace-image", label: "呼んだ瞬間に必ず別プログラムへ置き換わる", isCorrect: false },
          { id: "single-flow", label: "元の 1 本の制御フローしか存在しない", isCorrect: false },
        ],
        explanation: "`fork` は親子 2 系統を作り、戻り値の解釈が両者で異なります。",
      },
      {
        id: "execve-echo",
        prompt: "`execve_echo.asm` が `fork` と違う点はどれですか。",
        type: "single",
        options: [
          { id: "replace-program", label: "現在のプロセスの実行イメージを別プログラムへ置き換える", isCorrect: true },
          { id: "duplicate-process", label: "常に親子 2 つへ複製する", isCorrect: false },
          { id: "change-page-prot", label: "ページ権限だけを変更する", isCorrect: false },
        ],
        explanation: "`execve` は新しいプログラムに差し替える syscall で、`fork` とは役割が違います。",
      },
    ],
    resultMessages: {
      all_correct: "プロセス生成と置換の違いを整理できています。v7 の狙いは十分です。",
      partially_correct: "理解は進んでいます。`fork` の戻り値差分と `execve` の役割を再確認してください。",
      needs_review: "親子プロセスと実行イメージ置換が混ざっています。`fork_basic` と `execve_echo` を比較して復習してください。",
    },
  },
  v8: {
    title: "理解確認クイズ",
    intro: "`mmap` と `mprotect` を通じて仮想メモリの基礎を確認します。",
    questions: [
      {
        id: "page-unit",
        prompt: "x86-64 で通常のページサイズとして本文で扱っているものはどれですか。",
        type: "single",
        options: [
          { id: "4k", label: "4096 バイト", isCorrect: true },
          { id: "64k", label: "65536 バイト", isCorrect: false },
          { id: "512", label: "512 バイト", isCorrect: false },
        ],
        explanation: "本文では通常ページを 4096 バイト (4 KiB) として扱っています。",
      },
      {
        id: "mmap-mprotect",
        prompt: "`mmap` / `mprotect` について正しいものをすべて選んでください。",
        type: "multiple",
        options: [
          { id: "mmap-alloc", label: "`mmap` は新しいメモリ領域の確保に使う", isCorrect: true },
          { id: "mprotect-change", label: "`mprotect` は既存ページの保護属性を変える", isCorrect: true },
          { id: "prot-none", label: "`PROT_NONE` にするとアクセスが禁止される", isCorrect: true },
          { id: "always-safe-read", label: "`PROT_NONE` でも読み取りはできる", isCorrect: false },
        ],
        explanation: "`mmap` は確保、`mprotect` は権限変更、`PROT_NONE` はアクセス禁止です。",
      },
      {
        id: "segv-code",
        prompt: "`mprotect_crash.asm` で保護を外したページへアクセスした結果として正しいものはどれですか。",
        type: "single",
        options: [
          { id: "sigsegv", label: "SIGSEGV で終了し、終了コードは 139 になる", isCorrect: true },
          { id: "success", label: "そのまま 0 で正常終了する", isCorrect: false },
          { id: "execve", label: "別プログラムへ切り替わる", isCorrect: false },
        ],
        explanation: "アクセス禁止ページへの読み書きは SIGSEGV を起こし、Bash では 139 と観測されます。",
      },
    ],
    resultMessages: {
      all_correct: "仮想メモリの要点は押さえられています。`mmap` と保護属性の役割が明確です。",
      partially_correct: "理解は十分近いです。ページ単位と `PROT_NONE` の挙動だけ再確認してください。",
      needs_review: "仮想メモリの基礎がまだ曖昧です。`mmap` と `mprotect` の節を順に読み直してください。",
    },
  },
};
