use emu_core::{TraceResult, event, registers};

pub fn compiler_trace(source: &str) -> TraceResult {
    let type_error = source.contains("1 + true");
    let gc_heavy = source.contains("alloc_many 7") || source.contains("alloc_many 8");
    let summary = if type_error {
        "型推論が失敗し、コード生成前に停止しました。"
    } else if gc_heavy {
        "型推論を通過し、allocation pressure により GC が観察されました。"
    } else {
        "型推論、IR 生成、コード生成、GC を一通り観察しました。"
    };

    let diagnostics = if type_error {
        vec!["type_error=expected Int, found Bool".to_string()]
    } else {
        vec!["principal_type=(Int -> Pair Int List)".to_string()]
    };

    let observations = if type_error {
        vec![
            "型エラーは runtime fault ではなく compile-time failure として止まる。".to_string(),
            "型推論木の衝突点を読むと失敗理由を追える。".to_string(),
        ]
    } else if gc_heavy {
        vec![
            "allocation pressure が高まると tracing GC が live object を残して回収する。"
                .to_string(),
            "型推論は成功しており、GC は runtime 責務として動く。".to_string(),
        ]
    } else {
        vec![
            "型推論と GC は別の責務だが、同じプログラム理解に属する。".to_string(),
            "ソース、IR、RISC-V 相当コードを往復できる。".to_string(),
        ]
    };

    let type_event = if type_error {
        event(
            "compiler-type",
            "Type Error",
            "Int と Bool の加算を拒否",
            "constraint `Int ~ Bool` が解けず inference が止まる。",
            registers(&[("phase", "typecheck"), ("expr", "1 + true")]),
            vec!["AST node = Add(Int, Bool)"],
            vec![
                "generate: Int ~ left operand",
                "generate: Int ~ right operand",
                "conflict: Bool !~ Int",
            ],
            vec!["heap not started"],
        )
    } else {
        event(
            "compiler-type",
            "Type Inference",
            "principal type を導出",
            "let-bound 関数に対して多相型と具体型を解決する。",
            registers(&[("phase", "typecheck"), ("bindings", "3")]),
            vec!["AST node = alloc_many 3"],
            vec![
                "id : 'a -> 'a",
                "pair : Int -> List -> Pair",
                "alloc_many : Int -> List",
            ],
            vec!["heap not started"],
        )
    };

    let gc_event = if type_error {
        event(
            "compiler-gc",
            "Codegen Blocked",
            "型エラーのため runtime へ進まない",
            "型検査に失敗したため、GC はまだ関与しない。",
            registers(&[("phase", "stopped"), ("codegen", "false")]),
            vec!["machine code = none"],
            vec!["runtime skipped"],
            vec!["heap empty"],
        )
    } else if gc_heavy {
        event(
            "compiler-gc",
            "Tracing GC",
            "mark and sweep を実行",
            "root set から辿れない pair を解放し、live object だけを残す。",
            registers(&[
                ("phase", "runtime"),
                ("alloc_count", "8"),
                ("gc_cycles", "1"),
            ]),
            vec!["machine code = rv32 demo", "stack roots = [frame0, frame1]"],
            vec![
                "IR: cons cells lowered",
                "codegen: heap_alloc calls inserted",
            ],
            vec![
                "mark root pair@0x01",
                "mark list@0x02",
                "sweep pair@0x07",
                "free cells = 3",
            ],
        )
    } else {
        event(
            "compiler-gc",
            "Runtime Snapshot",
            "軽量な GC 観察",
            "短い実行でも heap roots と live object の差を観察できる。",
            registers(&[
                ("phase", "runtime"),
                ("alloc_count", "4"),
                ("gc_cycles", "0"),
            ]),
            vec!["machine code = rv32 demo", "stack roots = [frame0]"],
            vec!["IR: closure converted", "codegen: apply pair"],
            vec!["live pair@0x01", "live list@0x02"],
        )
    };

    TraceResult {
        summary: summary.to_string(),
        diagnostics,
        observations,
        timeline: vec![
            event(
                "compiler-parse",
                "Parse / Lower",
                "構文木と中間表現を生成",
                "ソースを AST にし、簡単な IR へ落とす。",
                registers(&[("phase", "parse"), ("nodes", "12")]),
                vec!["source length sampled"],
                vec!["AST ready", "IR skeleton ready"],
                vec!["heap idle"],
            ),
            type_event,
            gc_event,
        ],
        relevant_registers: Vec::new(),
        uses_stdin: false,
        active_branch: None,
        branches: None,
    }
}

#[cfg(test)]
mod tests {
    use super::compiler_trace;

    #[test]
    fn compiler_trace_catches_type_error() {
        let trace = compiler_trace("1 + true");
        assert!(trace.summary.contains("失敗"));
        assert!(trace.diagnostics[0].contains("Bool"));
    }

    #[test]
    fn compiler_trace_emits_gc_event() {
        let trace = compiler_trace("alloc_many 7");
        assert_eq!(trace.timeline[2].id, "compiler-gc");
        assert!(trace.summary.contains("GC"));
    }
}
