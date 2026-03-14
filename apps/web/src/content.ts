import type { Chapter } from "./lib/types";

export const chapters: Chapter[] = [
  {
    id: "riscv-101",
    title: "RISC-V 101",
    subtitle: "レジスタ、加算、PC の進み方を最初に手で掴む",
    scenario: "riscv",
    starterCode: `start:
  addi x1, x0, 5
  addi x2, x0, 7
  add  x3, x1, x2
  sub  x4, x3, x1
`,
    observationChecklist: [
      "x0 が常に 0 のままであることを確認できる",
      "addi と add と sub の違いを説明できる",
      "1 命令ごとに PC が 4 ずつ進む理由を説明できる",
    ],
    sections: [
      {
        id: "riscv-read",
        kind: "read",
        title: "Read: まずはレジスタだけで動かす",
        anchorEventId: "riscv-step-1",
        copy: {
          beginner:
            "いきなり OS や例外に入る前に、まずは RISC-V が何を 1 命令として実行しているかを見ます。ここではメモリではなくレジスタの値だけを追います。",
          practitioner:
            "RV32I の最小部分だけに絞って、`addi` `add` `sub` の結果がどのレジスタへ書き込まれるかを観察します。命令ごとに PC がどう進むかも合わせて確認します。",
          theory:
            "この章では命令実行を small-step の状態遷移として読みます。状態は主に `pc` とレジスタファイルで、各命令がその写像をどう更新するかを見ます。",
        },
      },
      {
        id: "riscv-write",
        kind: "write",
        title: "Write: 即値とレジスタを変える",
        anchorEventId: "riscv-step-3",
        copy: {
          beginner:
            "`5` や `7` を別の数字に変えて実行すると、右下のレジスタ結果がそのまま変わります。まずは `x3` の値がどう変わるか見てください。",
          practitioner:
            "即値やソースレジスタを変えて、依存関係のある後続命令まで結果が伝播することを確認してください。`x0` を書き込み先にした場合も試せます。",
          theory:
            "プログラムの一部を変更したとき、後続状態列のどこが変わるかを追うと、データ依存と制御の独立性を切り分けやすくなります。",
        },
      },
      {
        id: "riscv-watch",
        kind: "watch",
        title: "Watch: ステップ実行を読む",
        anchorEventId: "riscv-step-4",
        copy: {
          beginner:
            "下の `Watch` では、各ステップを選ぶとその時点のレジスタと PC が見えます。最初は `x1` `x2` `x3` `x4` だけで十分です。",
          practitioner:
            "命令一覧と選択中ステップのスナップショットを対応付けて、各命令が何を読み何を書いたかを読んでください。",
          theory:
            "実行トレースを状態列として読み、各ステップの focus 文を遷移ラベルとして解釈すると、後続の trap や VM exit も同じ形式で読めます。",
        },
      },
    ],
  },
  {
    id: "cpu-privilege",
    title: "CPU と特権遷移",
    subtitle: "命令、例外、特権モードを読みながら壊して観る",
    scenario: "cpu",
    starterCode: `# Toggle "illegal" to force a fault.
start:
  addi x1, x0, 1
  ecall
  # illegal
`,
    observationChecklist: [
      "ユーザーモードから supervisor へ制御が渡る瞬間を説明できる",
      "illegal instruction と ecall の原因差分を説明できる",
      "レジスタのどれが trap 前後で変わるか追える",
    ],
    sections: [
      {
        id: "cpu-read",
        kind: "read",
        title: "Read: 特権境界を見る",
        anchorEventId: "cpu-fetch",
        copy: {
          beginner:
            "CPU は命令を読むだけではなく、特別な命令や不正な命令に出会うと安全側へ制御を移します。まずは命令の流れが突然切り替わる瞬間を観察します。",
          practitioner:
            "ECALL と illegal instruction はどちらも trap を起こしますが、原因コードと復帰方針が違います。まず通常 fetch から trap entry までの遷移を確認します。",
          theory:
            "命令列は partial function ではなく、例外を伴う遷移系として理解する必要があります。ここでは fetch/decode/privilege transfer を観察対象にします。",
        },
      },
      {
        id: "cpu-write",
        kind: "write",
        title: "Write: 命令を壊す",
        anchorEventId: "cpu-trap",
        copy: {
          beginner: "コメントの `illegal` を有効にして再実行し、CPU がどう止まるかを比べてください。",
          practitioner: "命令列を崩して trap 原因が切り替わることを確認し、復帰位置との差分を追ってください。",
          theory: "入力プログラムを変えたとき、遷移関係のどのラベルが変化するかを追跡してください。",
        },
      },
      {
        id: "cpu-watch",
        kind: "watch",
        title: "Watch: trap timeline",
        anchorEventId: "cpu-return",
        copy: {
          beginner: "タイムラインで `trap entry` と `trap return` を選ぶと、レジスタがどう変わったか観られます。",
          practitioner: "sepc/scause 相当の観測点を見て、例外コードと復帰先が整合しているか確認します。",
          theory: "状態スナップショットを通じて、例外が制御演算子として振る舞うことを確認します。",
        },
      },
    ],
  },
  {
    id: "os-virtual-memory",
    title: "OS と仮想メモリ",
    subtitle: "ELF、ページング、page fault を同じ画面で観る",
    scenario: "os",
    starterCode: `kernel_entry:
  map_kernel_page 0x8000 0x1000
  enable_paging
  jump user_task
  # remove map_kernel_page to force a fault
`,
    observationChecklist: [
      "ELF ロードからページング有効化までの順序を説明できる",
      "page fault の原因アドレスを trace から特定できる",
      "仮想アドレスと物理アドレスの対応を説明できる",
    ],
    sections: [
      {
        id: "os-read",
        kind: "read",
        title: "Read: ブートからページングへ",
        anchorEventId: "os-boot",
        copy: {
          beginner:
            "カーネルは突然動き出すのではなく、ELF をロードし、最初のページを用意してからユーザーコードへ渡します。",
          practitioner:
            "ブート後に最低限のマッピングを張り、ページングを有効化してから trap handler を持つ流れを確認します。",
          theory:
            "アドレス空間の切り替えは、抽象機械の状態空間を拡張する操作として読むと整理しやすくなります。",
        },
      },
      {
        id: "os-write",
        kind: "write",
        title: "Write: マッピングを外す",
        anchorEventId: "os-fault",
        copy: {
          beginner: "`map_kernel_page` を消して page fault を起こし、何が足りないか観察してください。",
          practitioner: "ページテーブル初期化を壊して faulting address と trap 原因を比較してください。",
          theory: "不完全な写像が実行意味論にどう現れるかを観察してください。",
        },
      },
      {
        id: "os-watch",
        kind: "watch",
        title: "Watch: ページテーブル差分",
        anchorEventId: "os-paging",
        copy: {
          beginner: "タイムラインを進めると、仮想メモリが有効になった瞬間にメモリ表示が変わります。",
          practitioner: "paging on の前後で変換結果と trap log を突き合わせてください。",
          theory: "状態遷移の中でアドレス解決規則が切り替わる点に注目してください。",
        },
      },
    ],
  },
  {
    id: "virtualization",
    title: "仮想化と VM Exit",
    subtitle: "guest と host の境界を観る",
    scenario: "virt",
    starterCode: `guest_main:
  LOAD r1, [0x1000]
  CSR_WRITE hvip, 1
  HALT
`,
    observationChecklist: [
      "guest と host の境界を説明できる",
      "trap-and-emulate の流れを追える",
      "なぜ privileged 命令が直接実行されないか説明できる",
    ],
    sections: [
      {
        id: "virt-read",
        kind: "read",
        title: "Read: VM はどこで止まるか",
        anchorEventId: "virt-guest",
        copy: {
          beginner: "仮想マシンはただの別 PC ではありません。危ない命令は host が受け取って代わりに処理します。",
          practitioner:
            "guest 実行中でも全命令が自由に走るわけではなく、privileged 操作は VM exit の原因になります。",
          theory: "仮想化は二層の遷移系であり、あるラベル集合だけが上位層へ持ち上がります。",
        },
      },
      {
        id: "virt-write",
        kind: "write",
        title: "Write: privileged 命令を試す",
        anchorEventId: "virt-exit",
        copy: {
          beginner: "`CSR_WRITE` を残したまま実行し、host がどこで介入するかを見てください。",
          practitioner: "guest 内の privileged 操作を変えて VM exit reason がどう変わるか観察してください。",
          theory: "guest→host の遷移を引き起こすラベル集合を観測してください。",
        },
      },
      {
        id: "virt-watch",
        kind: "watch",
        title: "Watch: state save / restore",
        anchorEventId: "virt-resume",
        copy: {
          beginner: "exit の前後で guest のレジスタが保存され、処理後に戻される様子を見ます。",
          practitioner: "guest state snapshot と host emulation の差分を追ってください。",
          theory: "コンテキスト保存を双対の継続操作として見てください。",
        },
      },
    ],
  },
  {
    id: "compiler",
    title: "コンパイラ、型推論、GC",
    subtitle: "型の理由と実行時メモリ管理を並べて観る",
    scenario: "compiler",
    starterCode: `let id = fn x => x in
let pair = fn a => fn b => (a, b) in
let alloc_many = fn n => if n == 0 then [] else pair n (alloc_many (n - 1)) in
alloc_many 3
`,
    observationChecklist: [
      "型推論の結果がどの式から導かれたか説明できる",
      "型エラーになったとき理由を trace から説明できる",
      "GC が live object だけを残すことを説明できる",
    ],
    sections: [
      {
        id: "compiler-read",
        kind: "read",
        title: "Read: 型と実行時の責務を分ける",
        anchorEventId: "compiler-parse",
        copy: {
          beginner:
            "コンパイラはコードを機械語にするだけではなく、実行前に型の整合性を調べます。GC は実行中のメモリを管理します。",
          practitioner:
            "型推論と GC は別レイヤですが、同じプログラム理解の一部として並べて観ると挙動が整理しやすくなります。",
          theory: "静的意味論と動的意味論を同じプログラム上で対照させるのがこの章の目的です。",
        },
      },
      {
        id: "compiler-write",
        kind: "write",
        title: "Write: 型エラーと GC を発生させる",
        anchorEventId: "compiler-type",
        copy: {
          beginner: "`1 + true` を追加すると型エラーになります。`alloc_many 7` に変えると GC の動きが増えます。",
          practitioner:
            "式の一部を壊して inference failure を起こし、別ケースでは allocation pressure を上げて GC trace を増やしてください。",
          theory: "静的失敗と動的回収を同じ入力空間上で比較してください。",
        },
      },
      {
        id: "compiler-watch",
        kind: "watch",
        title: "Watch: inference tree と heap",
        anchorEventId: "compiler-gc",
        copy: {
          beginner: "型推論木とヒープ表示を行き来すると、実行前と実行中の責務の違いが見えます。",
          practitioner: "constraint solving と heap tracing を別の観測面として比較してください。",
          theory: "静的証明対象と動的 reachable set の違いに注目してください。",
        },
      },
    ],
  },
];
