use emu_core::{TraceResult, event, registers};

pub fn os_trace(source: &str) -> TraceResult {
    let mapped = source.contains("map_kernel_page");
    let summary = if mapped {
        "ELF ロード、ページング有効化、ユーザータスク移行を観察しました。"
    } else {
        "ページマッピング不足により page fault が発生しました。"
    };
    let diagnostics = if mapped {
        vec!["paging=enabled".to_string()]
    } else {
        vec![
            "page_fault=load_access".to_string(),
            "fault_address=0x8000".to_string(),
        ]
    };
    let observations = if mapped {
        vec![
            "ページテーブルの準備後に paging が有効化される。".to_string(),
            "ユーザータスクは仮想アドレス空間越しに実行される。".to_string(),
        ]
    } else {
        vec![
            "必要な仮想アドレス範囲が map されないと page fault になる。".to_string(),
            "fault address と trap log を突き合わせると原因を特定しやすい。".to_string(),
        ]
    };

    let end_event = if mapped {
        event(
            "os-paging",
            "Paging On",
            "satp 相当の切り替え完了",
            "仮想アドレス 0x8000 が物理 0x1000 に解決される。",
            registers(&[("pc", "0x8000"), ("mode", "S"), ("satp", "0x0001")]),
            vec![
                "virt 0x8000 -> phys 0x1000",
                "virt 0x9000 -> phys 0x2000",
                "user task entry = 0x9000",
            ],
            vec!["page table ready", "trap handler installed"],
            vec!["kernel heap warm"],
        )
    } else {
        event(
            "os-fault",
            "Page Fault",
            "未マップ領域へアクセス",
            "仮想アドレス 0x8000 の解決に失敗し、trap handler へ移行する。",
            registers(&[
                ("pc", "0x8000"),
                ("mode", "S"),
                ("scause", "load_page_fault"),
                ("stval", "0x8000"),
            ]),
            vec![
                "virt 0x8000 -> unresolved",
                "missing mapping for kernel text",
                "handler vector = 0x8000_0100",
            ],
            vec!["faulting translation step = leaf PTE miss"],
            vec!["kernel heap preserved for diagnostics"],
        )
    };

    TraceResult {
        summary: summary.to_string(),
        diagnostics,
        observations,
        timeline: vec![
            event(
                "os-boot",
                "ELF Load",
                "カーネルイメージを配置",
                "ELF セグメントが物理メモリに展開される。",
                registers(&[("pc", "0x1000"), ("mode", "M"), ("sp", "0x7fff")]),
                vec!["load text -> 0x1000", "load data -> 0x1800"],
                vec!["entry = kernel_entry"],
                vec!["boot allocator empty"],
            ),
            end_event,
        ],
        relevant_registers: Vec::new(),
        uses_stdin: false,
        active_branch: None,
        branches: None,
    }
}

#[cfg(test)]
mod tests {
    use super::os_trace;

    #[test]
    fn os_trace_faults_without_mapping() {
        let trace = os_trace("enable_paging");
        assert!(trace.summary.contains("page fault"));
        assert_eq!(trace.timeline[1].id, "os-fault");
    }
}
