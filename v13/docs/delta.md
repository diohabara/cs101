# Delta

- リアルタイムスケジューリングの 2 つの重要概念を導入
- 新しい概念: EDF (Earliest Deadline First)、優先度逆転、Priority Inheritance Protocol
- EDF: デッドラインが最も近いタスクを優先するスケジューリングアルゴリズム
- 優先度逆転: 低優先度タスクのロック保持が高優先度タスクをブロックする問題 (Mars Pathfinder 事例)
- Priority Inheritance: ロック保持中のタスクの実効優先度を引き上げることで逆転を防ぐ
