`v13` では、リアルタイムスケジューリングの 2 つの重要概念を学びます。
EDF (Earliest Deadline First) アルゴリズムによるデッドライン駆動のスケジューリングと、
優先度逆転問題およびその解決策である Priority Inheritance Protocol です。

## 概要

リアルタイムシステムでは、タスクが「正しい結果を」「決められた時間内に」返すことが求められます。
結果が正しくても期限を過ぎれば失敗です。この制約をデッドラインと呼びます。

本章では 2 つのテーマを扱います。

1. **EDF スケジューラ**: デッドラインが最も近いタスクを優先的に実行する
2. **優先度逆転と Priority Inheritance**: ロックを介した優先度の逆転現象と、その防止策

## EDF (Earliest Deadline First)

EDF は「デッドラインが最も近いタスクを最優先で実行する」スケジューリングアルゴリズムです。
1973 年に Liu & Layland が証明した通り、EDF は単一プロセッサ上で最適です。
スケジュール可能なタスクセットであれば、EDF は必ずスケジュールに成功します。

### アルゴリズム

1. 実行可能なタスクをデッドライン順にソートする
2. デッドラインが最も近いタスクを選び、CPU を割り当てる
3. タスクが完了したら次のタスクへ進む
4. 新しいタスクが到着したら再ソートする

### タスク定義

| タスク | デッドライン (tick) | 実行時間 (tick) |
|--------|---------------------|-----------------|
| A | 10 | 3 |
| B | 5 | 2 |
| C | 8 | 1 |

### EDF による実行順序

デッドラインでソートすると B(5) < C(8) < A(10) の順になります。

| tick | 実行タスク | 理由 | 完了? |
|------|-----------|------|-------|
| 0-1 | B | deadline=5 が最も近い | tick 2 で完了 |
| 2 | C | deadline=8 が次に近い | tick 3 で完了 |
| 3-5 | A | deadline=10 が残り | tick 6 で完了 |

全タスクがデッドライン前に完了しています。

| タスク | 完了 tick | デッドライン | 余裕 |
|--------|----------|-------------|------|
| B | 2 | 5 | 3 tick |
| C | 3 | 8 | 5 tick |
| A | 6 | 10 | 4 tick |

{{code:src/rt_sched.h}}

{{code:src/sched_edf.c}}

## 優先度逆転 (Priority Inversion)

### Mars Pathfinder 事件 (1997)

1997 年 7 月、火星に着陸した Mars Pathfinder 探査機は繰り返しリセットを起こしました。
原因は優先度逆転バグでした。

- **高優先度**: バス管理タスク（通信制御、デッドラインあり）
- **中優先度**: 科学データ収集タスク
- **低優先度**: 気象データ収集タスク

低優先度の気象タスクがバスの mutex を保持中に、中優先度の科学データタスクが CPU を奪いました。
高優先度のバス管理タスクは mutex を待ち続け、Watchdog Timer がタイムアウトしてシステムがリセットされました。

JPL のエンジニアは地球から VxWorks のデバッグ機能を使い、Priority Inheritance を有効化して問題を修正しました。

### なぜ起きるか

優先度逆転は以下の条件で発生します。

1. 低優先度タスク (LOW) がロックを取得する
2. 高優先度タスク (HIGH) が同じロックを要求するが、LOW が保持中なのでブロックされる
3. 中優先度タスク (MED) がロック不要な処理を開始し、LOW を割り込む
4. HIGH は LOW の完了を待つが、LOW は MED に割り込まれて進まない

結果として HIGH は MED が完了するまで待たされます。
高優先度であるはずの HIGH が、低優先度の LOW だけでなく中優先度の MED にも待たされる。
これが優先度の「逆転」です。

### WITHOUT inheritance のタイムライン

```
tick 0: LOW がロックを取得
tick 1: MED が到着、LOW より高優先度なので割り込み実行
tick 2: HIGH が到着、ロックが必要だが LOW が保持中 → ブロック
tick 3: MED 完了
tick 4: LOW が再開、ロック解放
tick 5: HIGH がロック取得、実行、完了

HIGH の待ち時間: 3 tick (tick 2 → tick 5)
```

![優先度逆転のタイムライン](/images/v13/priority-inversion.drawio.svg)

## Priority Inheritance Protocol

Priority Inheritance は優先度逆転を防ぐ仕組みです。

**ルール**: あるタスクがロックを保持しているとき、そのロックを待つより高い優先度のタスクが現れたら、ロック保持者の実効優先度を待機者と同じ水準に引き上げる。

これにより、ロック保持者は中優先度タスクに割り込まれなくなり、速やかにクリティカルセクションを完了してロックを解放できます。

### WITH inheritance のタイムライン

```
tick 0: LOW がロックを取得
tick 1: HIGH が到着、ロック待ち → LOW の実効優先度を HIGH に引き上げ
tick 2: LOW が HIGH 優先度で実行（MED は割り込めない）、ロック解放
tick 3: HIGH がロック取得、実行、完了
tick 4: MED が実行、完了

HIGH の待ち時間: 1 tick (tick 2 → tick 3)
```

### 比較

| 項目 | WITHOUT inheritance | WITH inheritance |
|------|--------------------|--------------------|
| HIGH の待ち時間 | 3 tick | 1 tick |
| MED の実行タイミング | HIGH より先 | HIGH より後 |
| 優先度順守 | 違反 | 正常 |

{{code:src/prio_inversion.c}}

## PlayStation との関連

- **オーディオスレッド**: PS4/PS5 のオーディオ処理はハードリアルタイム制約を持ちます。オーディオバッファは約 5ms (256 サンプル / 48kHz) ごとに埋める必要があり、デッドラインを逃すと音途切れ (underrun) が発生します。オーディオスレッドは最高優先度で動作し、EDF 的な考え方でデッドライン駆動のスケジューリングが行われます
- **GPU コマンドサブミット**: 描画コマンドのサブミットは、フレームデッドライン (16.6ms @60fps) 内に完了する必要があります。コマンドバッファの構築中に他のスレッドとリソースを共有する場合、優先度逆転が問題になり得ます
- **VxWorks の教訓**: Mars Pathfinder で使われた VxWorks の Priority Inheritance 機能は、現在のリアルタイム OS で標準的な機能となっています。PS の OS カーネルも同様の mutex 属性をサポートしています

## 参考文献

本章の技術的記述は以下の一次資料に基づいています。

- [Liu, C.L. & Layland, J.W. (1973). "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"](https://doi.org/10.1145/321738.321743) -- EDF の最適性証明を含む原論文。JACM 20(1), pp.46-61
- [Sha, L., Rajkumar, R. & Lehoczky, J.P. (1990). "Priority Inheritance Protocols: An Approach to Real-Time Synchronization"](https://doi.org/10.1109/2.57058) -- Priority Inheritance Protocol と Priority Ceiling Protocol の原論文。IEEE Trans. Computers 39(9)
- [JPL Pathfinder Bug Report (1997)](https://www.cs.unc.edu/~anderson/teach/comp790/papers/mars_pathfinder_long_version.html) -- Mars Pathfinder の優先度逆転問題の技術的詳細。Glenn Reeves による報告
