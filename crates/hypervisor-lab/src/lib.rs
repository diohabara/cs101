use emu_core::{TraceResult, event, registers};

pub fn virtualization_trace(source: &str) -> TraceResult {
    let privileged = source.contains("CSR_WRITE");
    let summary = if privileged {
        "guest の privileged 命令が VM exit を引き起こしました。"
    } else {
        "guest は privileged trap なしで最後まで進みました。"
    };
    let diagnostics = if privileged {
        vec!["vm_exit=csr_write".to_string()]
    } else {
        vec!["vm_exit=none".to_string()]
    };
    let observations = if privileged {
        vec![
            "guest は privileged 命令を直接完了できず host に制御が戻る。".to_string(),
            "host は emulation 後に guest state を再開する。".to_string(),
        ]
    } else {
        vec!["trap-and-emulate が不要な命令列では guest がそのまま進行する。".to_string()]
    };

    let exit_event = if privileged {
        event(
            "virt-exit",
            "VM Exit",
            "CSR write を host が捕捉",
            "guest の privileged 命令が host へ trap され、原因を emulation する。",
            registers(&[
                ("guest.pc", "0x1004"),
                ("host.pc", "0x8000_2000"),
                ("exit_reason", "csr_write"),
            ]),
            vec![
                "guest memory snapshot saved",
                "host sees hvip write",
                "emulate hvip = 1",
            ],
            vec!["trap-and-emulate", "resume guest after host patch"],
            vec!["guest stack saved", "host stack active"],
        )
    } else {
        event(
            "virt-exit",
            "Guest Halt",
            "VM exit なしで終了",
            "この命令列では privileged 操作がなく、guest は halt まで直進する。",
            registers(&[
                ("guest.pc", "0x1008"),
                ("host.pc", "0x8000_0000"),
                ("exit_reason", "none"),
            ]),
            vec!["guest memory unchanged"],
            vec!["no host emulation required"],
            vec!["guest state finalized"],
        )
    };

    TraceResult {
        summary: summary.to_string(),
        diagnostics,
        observations,
        timeline: vec![
            event(
                "virt-guest",
                "Guest Run",
                "guest を実行中",
                "host は vCPU 状態をロードし、guest 命令を進める。",
                registers(&[
                    ("guest.pc", "0x1000"),
                    ("mode", "VS"),
                    ("host.pc", "0x8000_0000"),
                ]),
                vec!["guest text @0x1000", "virtio console idle"],
                vec!["guest -> host boundary armed"],
                vec!["guest context resident"],
            ),
            exit_event,
            event(
                "virt-resume",
                "Guest Resume",
                "guest state を復元",
                if privileged {
                    "host が emulation を終え、guest を次命令へ戻す。"
                } else {
                    "halt 後は復元ではなく最終状態を確定する。"
                },
                registers(&[
                    ("guest.pc", if privileged { "0x1008" } else { "0x1008" }),
                    ("host.pc", "0x8000_0000"),
                    ("guest.r1", "0x0042"),
                ]),
                vec!["guest state restored to vcpu struct"],
                vec!["resume point committed"],
                vec!["host stack released"],
            ),
        ],
        relevant_registers: Vec::new(),
        uses_stdin: false,
        active_branch: None,
        branches: None,
    }
}

#[cfg(test)]
mod tests {
    use super::virtualization_trace;

    #[test]
    fn virtualization_trace_detects_vm_exit() {
        let trace = virtualization_trace("CSR_WRITE hvip, 1");
        assert_eq!(trace.timeline[1].id, "virt-exit");
        assert!(trace.diagnostics[0].contains("csr_write"));
    }
}
