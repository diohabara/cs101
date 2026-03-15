# Delta

- vmlaunch によるゲスト VM 実行を追加
- 新しい概念: vmlaunch, ゲストコード（マシンコード直接配置）, VM exit (HLT), vmlaunch vs vmresume
- 新しいファイル: guest.h, guest.c
- 最小ゲスト（mov rax, 0x42; hlt）を実行して HLT による VM exit をハンドリング
- VMX 非対応環境ではシミュレーション出力にフォールバック
