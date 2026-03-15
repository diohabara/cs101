use serde::Serialize;
use std::collections::BTreeMap;

mod x86;

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct KeyValue {
    pub key: String,
    pub value: String,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct InstructionState {
    pub line: usize,
    pub rip_before: String,
    pub rip_after: String,
    pub source: String,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct TraceEvent {
    pub id: String,
    pub label: String,
    pub detail: String,
    pub focus: String,
    pub registers: Vec<KeyValue>,
    pub memory: Vec<String>,
    pub types: Vec<String>,
    pub heap: Vec<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub instruction: Option<InstructionState>,
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub register_diff: Vec<KeyValue>,
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub memory_diff: Vec<KeyValue>,
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub stack_snapshot: Vec<KeyValue>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub branch_id: Option<String>,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct TraceBranch {
    pub id: String,
    pub label: String,
    pub summary: String,
    pub stdout: Vec<String>,
    pub stderr: Vec<String>,
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub initial_registers: Vec<KeyValue>,
    pub timeline: Vec<TraceEvent>,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct TraceResult {
    pub summary: String,
    pub diagnostics: Vec<String>,
    pub observations: Vec<String>,
    pub timeline: Vec<TraceEvent>,
    #[serde(skip_serializing_if = "Vec::is_empty", default)]
    pub relevant_registers: Vec<String>,
    #[serde(default)]
    pub uses_stdin: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub active_branch: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub branches: Option<Vec<TraceBranch>>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ParsedInstruction {
    opcode: String,
    rd: usize,
    rs1: usize,
    rs2: usize,
    imm: i32,
    source: String,
}

pub fn registers(entries: &[(&str, &str)]) -> Vec<KeyValue> {
    entries
        .iter()
        .map(|(key, value)| KeyValue {
            key: (*key).to_string(),
            value: (*value).to_string(),
        })
        .collect()
}

pub fn event(
    id: &str,
    label: &str,
    detail: &str,
    focus: &str,
    registers: Vec<KeyValue>,
    memory: Vec<&str>,
    types: Vec<&str>,
    heap: Vec<&str>,
) -> TraceEvent {
    TraceEvent {
        id: id.to_string(),
        label: label.to_string(),
        detail: detail.to_string(),
        focus: focus.to_string(),
        registers,
        memory: memory.into_iter().map(str::to_string).collect(),
        types: types.into_iter().map(str::to_string).collect(),
        heap: heap.into_iter().map(str::to_string).collect(),
        instruction: None,
        register_diff: Vec::new(),
        memory_diff: Vec::new(),
        stack_snapshot: Vec::new(),
        branch_id: None,
    }
}

pub fn x86_trace(stage_id: &str, source: &str, stdin: &str) -> Result<TraceResult, String> {
    x86::x86_trace(stage_id, source, stdin)
}

pub fn riscv_trace(source: &str) -> Result<TraceResult, String> {
    let program = parse_program(source)?;
    if program.is_empty() {
        return Err("命令がありません。addi / add / sub を1つ以上書いてください。".to_string());
    }

    let mut regs = [0_i32; 32];
    let mut pc = 0_i32;
    let mut timeline = Vec::with_capacity(program.len());

    for (index, instruction) in program.iter().enumerate() {
        let before_pc = pc;
        let detail = instruction.source.clone();
        let focus = match instruction.opcode.as_str() {
            "addi" => {
                let lhs = read_reg(&regs, instruction.rs1);
                let value = lhs + instruction.imm;
                write_reg(&mut regs, instruction.rd, value);
                format!(
                    "x{} = x{}({}) + {} -> {}",
                    instruction.rd,
                    instruction.rs1,
                    lhs,
                    instruction.imm,
                    read_reg(&regs, instruction.rd)
                )
            }
            "add" => {
                let lhs = read_reg(&regs, instruction.rs1);
                let rhs = read_reg(&regs, instruction.rs2);
                write_reg(&mut regs, instruction.rd, lhs + rhs);
                format!(
                    "x{} = x{}({}) + x{}({}) -> {}",
                    instruction.rd,
                    instruction.rs1,
                    lhs,
                    instruction.rs2,
                    rhs,
                    read_reg(&regs, instruction.rd)
                )
            }
            "sub" => {
                let lhs = read_reg(&regs, instruction.rs1);
                let rhs = read_reg(&regs, instruction.rs2);
                write_reg(&mut regs, instruction.rd, lhs - rhs);
                format!(
                    "x{} = x{}({}) - x{}({}) -> {}",
                    instruction.rd,
                    instruction.rs1,
                    lhs,
                    instruction.rs2,
                    rhs,
                    read_reg(&regs, instruction.rd)
                )
            }
            other => return Err(format!("未対応の命令です: {other}")),
        };
        pc += 4;

        timeline.push(TraceEvent {
            id: format!("riscv-step-{}", index + 1),
            label: format!("Step {}", index + 1),
            detail,
            focus,
            registers: format_registers(&regs, before_pc, pc),
            memory: vec![
                format!("pc(before) = {before_pc:#06x}"),
                format!("pc(after) = {pc:#06x}"),
                "memory access = none".to_string(),
            ],
            types: vec![
                "decode -> execute -> writeback".to_string(),
                format!("opcode = {}", instruction.opcode),
            ],
            heap: vec![
                "runtime heap = not used".to_string(),
                "side effects = register file only".to_string(),
            ],
            instruction: None,
            register_diff: Vec::new(),
            memory_diff: Vec::new(),
            stack_snapshot: Vec::new(),
            branch_id: None,
        });
    }

    Ok(TraceResult {
        summary: format!(
            "{} 命令を実行しました。最終的に x3={}, x4={} です。",
            timeline.len(),
            read_reg(&regs, 3),
            read_reg(&regs, 4)
        ),
        diagnostics: vec![
            "実行モード = RV32I subset".to_string(),
            format!("最終 PC = {pc:#06x}"),
            "x0 への書き込みは無視されます".to_string(),
        ],
        observations: vec![
            "各ステップで PC は 4 ずつ進む。".to_string(),
            "x0 は常に 0 のままで、他のレジスタ計算の基準になる。".to_string(),
            "この章ではメモリアクセスや例外を外し、レジスタ更新だけに集中している。".to_string(),
        ],
        timeline,
        relevant_registers: Vec::new(),
        uses_stdin: false,
        active_branch: None,
        branches: None,
    })
}

fn parse_program(source: &str) -> Result<Vec<ParsedInstruction>, String> {
    let mut instructions = Vec::new();

    for (line_no, raw_line) in source.lines().enumerate() {
        let mut line = raw_line
            .split('#')
            .next()
            .unwrap_or_default()
            .trim()
            .to_string();
        if line.is_empty() {
            continue;
        }

        if line.ends_with(':') {
            continue;
        }

        if let Some((_, tail)) = line.split_once(':') {
            line = tail.trim().to_string();
            if line.is_empty() {
                continue;
            }
        }

        let sanitized = line.replace(',', " ");
        let parts: Vec<&str> = sanitized.split_whitespace().collect();
        if parts.is_empty() {
            continue;
        }

        let opcode = parts[0].to_ascii_lowercase();
        let instruction = match opcode.as_str() {
            "addi" => {
                if parts.len() != 4 {
                    return Err(format!(
                        "{} 行目: addi rd, rs1, imm の形式で書いてください。",
                        line_no + 1
                    ));
                }

                ParsedInstruction {
                    opcode,
                    rd: parse_register(parts[1], line_no)?,
                    rs1: parse_register(parts[2], line_no)?,
                    rs2: 0,
                    imm: parse_immediate(parts[3], line_no)?,
                    source: line.clone(),
                }
            }
            "add" | "sub" => {
                if parts.len() != 4 {
                    return Err(format!(
                        "{} 行目: {} rd, rs1, rs2 の形式で書いてください。",
                        line_no + 1,
                        opcode
                    ));
                }

                ParsedInstruction {
                    opcode,
                    rd: parse_register(parts[1], line_no)?,
                    rs1: parse_register(parts[2], line_no)?,
                    rs2: parse_register(parts[3], line_no)?,
                    imm: 0,
                    source: line.clone(),
                }
            }
            other => return Err(format!("{} 行目: 未対応の命令です: {}", line_no + 1, other)),
        };

        instructions.push(instruction);
    }

    Ok(instructions)
}

fn parse_register(input: &str, line_no: usize) -> Result<usize, String> {
    let stripped = input.trim().trim_start_matches('x');
    let index = stripped.parse::<usize>().map_err(|_| {
        format!(
            "{} 行目: レジスタ名 `{}` を解釈できません。",
            line_no + 1,
            input
        )
    })?;

    if index > 31 {
        return Err(format!(
            "{} 行目: x0 から x31 までしか使えません。",
            line_no + 1
        ));
    }

    Ok(index)
}

fn parse_immediate(input: &str, line_no: usize) -> Result<i32, String> {
    input
        .trim()
        .parse::<i32>()
        .map_err(|_| format!("{} 行目: 即値 `{}` を解釈できません。", line_no + 1, input))
}

fn read_reg(regs: &[i32; 32], index: usize) -> i32 {
    if index == 0 { 0 } else { regs[index] }
}

fn write_reg(regs: &mut [i32; 32], index: usize, value: i32) {
    if index != 0 {
        regs[index] = value;
    }
}

fn format_registers(regs: &[i32; 32], before_pc: i32, after_pc: i32) -> Vec<KeyValue> {
    let mut map = BTreeMap::new();
    map.insert("pc(before)".to_string(), format!("{before_pc:#06x}"));
    map.insert("pc(after)".to_string(), format!("{after_pc:#06x}"));
    for index in 0..8 {
        map.insert(format!("x{index}"), read_reg(regs, index).to_string());
    }

    map.into_iter()
        .map(|(key, value)| KeyValue { key, value })
        .collect()
}

pub fn cpu_trace(source: &str) -> TraceResult {
    let illegal = source.contains("illegal");
    let summary = if illegal {
        "Illegal instruction を検出し、supervisor へ trap しました。"
    } else {
        "ECALL による trap と復帰を観察しました。"
    };
    let diagnostics = if illegal {
        vec!["scause=illegal_instruction".to_string()]
    } else {
        vec!["scause=environment_call_from_u".to_string()]
    };
    let observations = if illegal {
        vec![
            "illegal instruction は命令の意味付けに失敗した瞬間に trap される。".to_string(),
            "trap entry 後に sepc は faulting instruction を指す。".to_string(),
        ]
    } else {
        vec![
            "ecall は明示的な特権移行要求として扱われる。".to_string(),
            "trap return 後は次の命令位置に復帰する。".to_string(),
        ]
    };

    let middle = if illegal {
        event(
            "cpu-trap",
            "Trap Entry",
            "illegal instruction を検出",
            "不正命令が decode で拒否され、制御が supervisor handler へ移る。",
            registers(&[
                ("pc", "0x0004"),
                ("mode", "S"),
                ("scause", "illegal_instruction"),
                ("sepc", "0x0004"),
            ]),
            vec![
                "instruction word = 0xffff_ffff",
                "handler vector = 0x8000_0100",
            ],
            vec!["decode(illegal) -> trap", "resume = faulting pc"],
            vec!["stack frame saved", "return blocked until handler decides"],
        )
    } else {
        event(
            "cpu-trap",
            "Trap Entry",
            "ecall で supervisor call",
            "ユーザーモードのコードが OS へ処理を委譲する。",
            registers(&[
                ("pc", "0x0004"),
                ("mode", "S"),
                ("scause", "environment_call_from_u"),
                ("sepc", "0x0004"),
            ]),
            vec!["syscall number = 1", "handler vector = 0x8000_0100"],
            vec!["ecall : U -> S transition", "resume = pc + 4"],
            vec!["stack frame saved", "syscall context active"],
        )
    };

    TraceResult {
        summary: summary.to_string(),
        diagnostics,
        observations,
        timeline: vec![
            event(
                "cpu-fetch",
                "Fetch / Decode",
                "命令を読み込み中",
                "ユーザーモードで命令を順に decode している。",
                registers(&[("pc", "0x0000"), ("mode", "U"), ("x1", "0x0000")]),
                vec!["text@0x0000", "next pc = 0x0004"],
                vec!["instruction stream ready"],
                vec!["heap inactive"],
            ),
            middle,
            event(
                "cpu-return",
                "Trap Return",
                "handler から復帰",
                if illegal {
                    "handler は fault を記録し、観察のため同じ pc に留める。"
                } else {
                    "handler は syscall を終え、次の命令へ復帰する。"
                },
                registers(&[
                    ("pc", if illegal { "0x0004" } else { "0x0008" }),
                    ("mode", "U"),
                    ("x1", "0x0001"),
                ]),
                vec!["user stack restored"],
                vec!["continuation restored"],
                vec!["kernel frame released"],
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
    use super::{cpu_trace, riscv_trace};

    #[test]
    fn cpu_trace_reports_ecall_by_default() {
        let trace = cpu_trace("ecall");
        assert!(trace.summary.contains("ECALL"));
        assert_eq!(trace.timeline[0].id, "cpu-fetch");
    }

    #[test]
    fn cpu_trace_switches_to_illegal_branch() {
        let trace = cpu_trace("illegal");
        assert!(trace.diagnostics[0].contains("illegal_instruction"));
    }

    #[test]
    fn riscv_trace_executes_register_program() {
        let trace = riscv_trace("addi x1, x0, 2\naddi x2, x0, 3\nadd x3, x1, x2").unwrap();
        assert_eq!(trace.timeline.len(), 3);
        let last = trace.timeline.last().unwrap();
        assert!(last.focus.contains("-> 5"));
    }

    #[test]
    fn riscv_trace_rejects_invalid_opcode() {
        let error = riscv_trace("mul x1, x2, x3").unwrap_err();
        assert!(error.contains("未対応"));
    }
}
