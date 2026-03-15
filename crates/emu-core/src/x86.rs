use crate::{InstructionState, KeyValue, TraceBranch, TraceEvent, TraceResult};
use std::collections::{BTreeMap, BTreeSet, HashMap};

const DATA_BASE: u64 = 0x0010_0000;
const TEXT_BASE: u64 = 0x0040_0000;
const STACK_TOP: u64 = 0x7fff_0000;
const STACK_SIZE: u64 = 0x1_0000; // 64KB
const MMAP_BASE: u64 = 0x0060_0000;
const FORK_CHILD_PID: u64 = 4321;
const GETPID_VALUE: u64 = 1234;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Section {
    Text,
    Data,
    Bss,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Ord, PartialOrd)]
enum Reg {
    Rax,
    Rbx,
    Rcx,
    Rdx,
    Rsi,
    Rdi,
    Rsp,
    Rbp,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
}

impl Reg {
    fn name(self) -> &'static str {
        match self {
            Reg::Rax => "rax",
            Reg::Rbx => "rbx",
            Reg::Rcx => "rcx",
            Reg::Rdx => "rdx",
            Reg::Rsi => "rsi",
            Reg::Rdi => "rdi",
            Reg::Rsp => "rsp",
            Reg::Rbp => "rbp",
            Reg::R8 => "r8",
            Reg::R9 => "r9",
            Reg::R10 => "r10",
            Reg::R11 => "r11",
            Reg::R12 => "r12",
            Reg::R13 => "r13",
        }
    }
}

#[derive(Debug, Clone)]
struct Flags {
    zf: bool,
    sf: bool,
    cf: bool,
    of: bool,
}

impl Flags {
    fn as_rflags(&self) -> u64 {
        (u64::from(self.cf) << 0)
            | (u64::from(self.zf) << 6)
            | (u64::from(self.sf) << 7)
            | (u64::from(self.of) << 11)
    }
}

#[derive(Debug, Clone)]
struct Program {
    instructions: Vec<Instruction>,
    text_labels: HashMap<String, usize>,
    data_memory: BTreeMap<u64, u8>,
    data_regions: Vec<Region>,
    entry_index: usize,
    relevant_registers: Vec<Reg>,
    uses_stdin: bool,
}

#[derive(Debug, Clone)]
struct Instruction {
    address: u64,
    line_no: usize,
    source: String,
    opcode: Opcode,
}

#[derive(Debug, Clone)]
enum Opcode {
    Mov(Operand, Operand),
    Add(Operand, Operand),
    Sub(Operand, Operand),
    Xor(Operand, Operand),
    Inc(Operand),
    Cmp(Operand, Operand),
    Lea(Reg, AddressExpr),
    Push(Operand),
    Pop(Operand),
    Call(String),
    Ret,
    Jmp(String),
    Jz(String),
    Jnz(String),
    Jg(String),
    Jge(String),
    Syscall,
}

#[derive(Debug, Clone)]
enum Operand {
    Reg(Reg),
    Imm(i64),
    Mem(AddressExpr),
}

#[derive(Debug, Clone)]
struct AddressExpr {
    base: Option<Reg>,
    index: Option<(Reg, i64)>,
    disp: i64,
}

#[derive(Debug, Clone)]
struct Region {
    start: u64,
    len: u64,
    read: bool,
    write: bool,
    label: String,
}

#[derive(Debug, Clone)]
struct RuntimeBranch {
    id: String,
    label: String,
    pc: usize,
    regs: HashMap<Reg, u64>,
    flags: Flags,
    memory: BTreeMap<u64, u8>,
    regions: Vec<Region>,
    next_mmap: u64,
    stdin: Vec<u8>,
    stdin_cursor: usize,
    stdout: String,
    stderr: String,
    fd_table: HashMap<u64, String>,
    next_fd: u64,
    trace: Vec<TraceEvent>,
    initial_registers: Vec<KeyValue>,
    diagnostics: Vec<String>,
    exit_code: Option<u64>,
    pending_exec_output: Option<String>,
}

#[derive(Debug)]
enum BranchOutcome {
    Complete(RuntimeBranch),
    Split {
        parent: RuntimeBranch,
        child: RuntimeBranch,
    },
}

pub fn x86_trace(stage_id: &str, source: &str, stdin: &str) -> Result<TraceResult, String> {
    if !matches!(stage_id, "v1" | "v2" | "v3" | "v4" | "v5" | "v6" | "v7" | "v8") {
        return Err(format!("unsupported x86 stage: {stage_id}"));
    }

    let program = parse_program(source)?;
    let main_branch = RuntimeBranch::new("main", "main", &program, stdin);
    let mut branches = execute_branch(&program, main_branch)?;

    let active_branch = if branches.iter().any(|branch| branch.id == "parent") {
        "parent".to_string()
    } else {
        branches
            .first()
            .map(|branch| branch.id.clone())
            .unwrap_or_else(|| "main".to_string())
    };

    let observations = vec![
        "RIP は各 step の instruction.rip_before / rip_after に入る。".to_string(),
        "register_diff は直前 step から変わったレジスタだけを示す。".to_string(),
        "fork は parent / child の 2 branch として追える。".to_string(),
    ];

    let diagnostics = branches
        .iter()
        .flat_map(|branch| branch.diagnostics.iter().map(|entry| format!("{}: {}", branch.label, entry)))
        .collect::<Vec<_>>();

    let summary = format!(
        "{} branch / {} step を生成しました。",
        branches.len(),
        branches.iter().map(|branch| branch.trace.len()).sum::<usize>()
    );

    let serialized_branches = branches
        .drain(..)
        .map(|branch| TraceBranch {
            id: branch.id.clone(),
            label: branch.label.clone(),
            summary: branch_summary(&branch),
            stdout: to_output_lines(&branch.stdout),
            stderr: to_output_lines(&branch.stderr),
            initial_registers: branch.initial_registers,
            timeline: branch.trace,
        })
        .collect::<Vec<_>>();

    let active_timeline = serialized_branches
        .iter()
        .find(|branch| branch.id == active_branch)
        .map(|branch| branch.timeline.clone())
        .unwrap_or_default();

    Ok(TraceResult {
        summary,
        diagnostics,
        observations,
        timeline: active_timeline,
        relevant_registers: program
            .relevant_registers
            .iter()
            .map(|reg| reg.name().to_string())
            .collect(),
        uses_stdin: program.uses_stdin,
        active_branch: Some(active_branch),
        branches: Some(serialized_branches),
    })
}

impl RuntimeBranch {
    fn new(id: &str, label: &str, program: &Program, stdin: &str) -> Self {
        let mut regs = HashMap::new();
        regs.insert(Reg::Rsp, STACK_TOP);
        regs.insert(Reg::Rbp, STACK_TOP);

        Self {
            id: id.to_string(),
            label: label.to_string(),
            pc: program.entry_index,
            regs,
            flags: Flags {
                zf: false,
                sf: false,
                cf: false,
                of: false,
            },
            memory: program.data_memory.clone(),
            regions: {
                let mut regions = program.data_regions.clone();
                regions.push(Region {
                    start: STACK_TOP - STACK_SIZE,
                    len: STACK_SIZE,
                    read: true,
                    write: true,
                    label: "stack".to_string(),
                });
                regions
            },
            next_mmap: MMAP_BASE,
            stdin: stdin.as_bytes().to_vec(),
            stdin_cursor: 0,
            stdout: String::new(),
            stderr: String::new(),
            fd_table: HashMap::new(),
            next_fd: 3,
            trace: Vec::new(),
            initial_registers: Vec::new(),
            diagnostics: Vec::new(),
            exit_code: None,
            pending_exec_output: None,
        }
    }
}

fn execute_branch(program: &Program, mut branch: RuntimeBranch) -> Result<Vec<RuntimeBranch>, String> {
    loop {
        if let Some(output) = branch.pending_exec_output.take() {
            branch.stdout.push_str(&output);
            branch.exit_code.get_or_insert(0);
        }

        if branch.exit_code.is_some() {
            return Ok(vec![branch]);
        }

        let instruction = program
            .instructions
            .get(branch.pc)
            .ok_or_else(|| format!("pc {} is out of range", branch.pc))?
            .clone();

        let before_regs = branch.regs.clone();
        let before_flags = branch.flags.clone();
        let before_pc = branch.pc;
        let before_rip = instruction.address;
        if branch.trace.is_empty() {
            branch.initial_registers = snapshot_registers(&before_regs, &before_flags, before_rip, before_rip);
        }
        let mut memory_effects = Vec::new();
        let mut type_effects = Vec::new();
        let mut heap_effects = Vec::new();
        let mut memory_diff = Vec::new();

        let outcome = match execute_instruction(
            program,
            &mut branch,
            &instruction,
            &mut memory_effects,
            &mut type_effects,
            &mut heap_effects,
            &mut memory_diff,
        ) {
            Ok(outcome) => outcome,
            Err(error) if branch.exit_code.is_some() => {
                let register_diff = diff_registers(&before_regs, &branch.regs, &before_flags, &branch.flags);
                let stack_snapshot = snapshot_stack(&branch);
                let event = TraceEvent {
                    id: format!("{}-step-{}", branch.id, branch.trace.len() + 1),
                    label: format!("Step {}", branch.trace.len() + 1),
                    detail: instruction.source.clone(),
                    focus: error,
                    registers: snapshot_registers(&branch.regs, &branch.flags, before_rip, before_rip),
                    memory: memory_effects,
                    types: type_effects,
                    heap: heap_effects,
                    instruction: Some(InstructionState {
                        line: instruction.line_no,
                        rip_before: format!("{before_rip:#010x}"),
                        rip_after: format!("{before_rip:#010x}"),
                        source: instruction.source.clone(),
                    }),
                    register_diff,
                    memory_diff,
                    stack_snapshot,
                    branch_id: Some(branch.id.clone()),
                };
                branch.trace.push(event);
                return Ok(vec![branch]);
            }
            Err(error) => return Err(error),
        };

        match outcome {
            BranchOutcome::Complete(mut completed) => {
                let after_rip = if let Some(next) = program.instructions.get(completed.pc) {
                    next.address
                } else {
                    instruction.address + 4
                };

                let register_diff = diff_registers(&before_regs, &completed.regs, &before_flags, &completed.flags);
                let stack_snapshot = snapshot_stack(&completed);
                let event = TraceEvent {
                    id: format!("{}-step-{}", completed.id, completed.trace.len() + 1),
                    label: format!("Step {}", completed.trace.len() + 1),
                    detail: instruction.source.clone(),
                    focus: focus_for(&instruction, &register_diff, &memory_effects, completed.exit_code),
                    registers: snapshot_registers(&completed.regs, &completed.flags, before_rip, after_rip),
                    memory: memory_effects,
                    types: type_effects,
                    heap: heap_effects,
                    instruction: Some(InstructionState {
                        line: instruction.line_no,
                        rip_before: format!("{before_rip:#010x}"),
                        rip_after: format!("{after_rip:#010x}"),
                        source: instruction.source.clone(),
                    }),
                    register_diff,
                    memory_diff,
                    stack_snapshot,
                    branch_id: Some(completed.id.clone()),
                };
                completed.trace.push(event);
                branch = completed;
            }
            BranchOutcome::Split { mut parent, mut child } => {
                let after_rip = if let Some(next) = program.instructions.get(parent.pc) {
                    next.address
                } else {
                    instruction.address + 4
                };

                let parent_diff = diff_registers(&before_regs, &parent.regs, &before_flags, &parent.flags);
                let child_diff = diff_registers(&before_regs, &child.regs, &before_flags, &child.flags);
                let parent_stack = snapshot_stack(&parent);
                let child_stack = snapshot_stack(&child);
                let parent_event = TraceEvent {
                    id: format!("{}-step-{}", parent.id, parent.trace.len() + 1),
                    label: format!("Step {}", parent.trace.len() + 1),
                    detail: instruction.source.clone(),
                    focus: "sys_fork で親 branch を継続".to_string(),
                    registers: snapshot_registers(&parent.regs, &parent.flags, before_rip, after_rip),
                    memory: vec!["fork -> parent branch".to_string()],
                    types: vec!["branch = parent".to_string()],
                    heap: vec![format!("child pid = {FORK_CHILD_PID}")],
                    instruction: Some(InstructionState {
                        line: instruction.line_no,
                        rip_before: format!("{before_rip:#010x}"),
                        rip_after: format!("{after_rip:#010x}"),
                        source: instruction.source.clone(),
                    }),
                    register_diff: parent_diff,
                    memory_diff: memory_diff.clone(),
                    stack_snapshot: parent_stack,
                    branch_id: Some(parent.id.clone()),
                };
                let child_event = TraceEvent {
                    id: format!("{}-step-{}", child.id, child.trace.len() + 1),
                    label: format!("Step {}", child.trace.len() + 1),
                    detail: instruction.source.clone(),
                    focus: "sys_fork で子 branch を継続".to_string(),
                    registers: snapshot_registers(&child.regs, &child.flags, before_rip, after_rip),
                    memory: vec!["fork -> child branch".to_string()],
                    types: vec!["branch = child".to_string()],
                    heap: vec![format!("child pid = {FORK_CHILD_PID}")],
                    instruction: Some(InstructionState {
                        line: instruction.line_no,
                        rip_before: format!("{before_rip:#010x}"),
                        rip_after: format!("{after_rip:#010x}"),
                        source: instruction.source.clone(),
                    }),
                    register_diff: child_diff,
                    memory_diff,
                    stack_snapshot: child_stack,
                    branch_id: Some(child.id.clone()),
                };
                parent.trace.push(parent_event);
                child.trace.push(child_event);

                let mut branches = execute_branch(program, parent)?;
                branches.extend(execute_branch(program, child)?);
                return Ok(branches);
            }
        }

        if branch.pc == before_pc && branch.exit_code.is_none() {
            return Err(format!("instruction at pc {before_pc} did not advance"));
        }
    }
}

fn execute_instruction(
    program: &Program,
    branch: &mut RuntimeBranch,
    instruction: &Instruction,
    memory_effects: &mut Vec<String>,
    _type_effects: &mut Vec<String>,
    heap_effects: &mut Vec<String>,
    memory_diff: &mut Vec<KeyValue>,
) -> Result<BranchOutcome, String> {
    let next_pc = branch.pc + 1;
    match &instruction.opcode {
        Opcode::Mov(dst, src) => {
            let value = read_operand(program, branch, src, memory_effects)?;
            write_operand(program, branch, dst, value, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Add(dst, src) => {
            let lhs = read_operand(program, branch, dst, memory_effects)?;
            let rhs = read_operand(program, branch, src, memory_effects)?;
            let result = lhs.wrapping_add(rhs);
            update_add_flags(&mut branch.flags, lhs, rhs, result);
            write_operand(program, branch, dst, result, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Sub(dst, src) => {
            let lhs = read_operand(program, branch, dst, memory_effects)?;
            let rhs = read_operand(program, branch, src, memory_effects)?;
            let result = lhs.wrapping_sub(rhs);
            update_sub_flags(&mut branch.flags, lhs, rhs, result);
            write_operand(program, branch, dst, result, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Xor(dst, src) => {
            let lhs = read_operand(program, branch, dst, memory_effects)?;
            let rhs = read_operand(program, branch, src, memory_effects)?;
            let result = lhs ^ rhs;
            branch.flags.zf = result == 0;
            branch.flags.sf = (result as i64) < 0;
            branch.flags.cf = false;
            branch.flags.of = false;
            write_operand(program, branch, dst, result, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Inc(op) => {
            let lhs = read_operand(program, branch, op, memory_effects)?;
            let result = lhs.wrapping_add(1);
            let cf = branch.flags.cf;
            update_add_flags(&mut branch.flags, lhs, 1, result);
            branch.flags.cf = cf;
            write_operand(program, branch, op, result, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Cmp(lhs, rhs) => {
            let left = read_operand(program, branch, lhs, memory_effects)?;
            let right = read_operand(program, branch, rhs, memory_effects)?;
            let result = left.wrapping_sub(right);
            update_sub_flags(&mut branch.flags, left, right, result);
            branch.pc = next_pc;
        }
        Opcode::Lea(reg, expr) => {
            let address = eval_address(program, branch, expr)?;
            set_reg(branch, *reg, address);
            memory_effects.push(format!("lea -> {} = {address:#010x}", reg.name()));
            branch.pc = next_pc;
        }
        Opcode::Push(op) => {
            let value = read_operand(program, branch, op, memory_effects)?;
            let rsp = get_reg(branch, Reg::Rsp).wrapping_sub(8);
            set_reg(branch, Reg::Rsp, rsp);
            store_u64(branch, rsp, value, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Pop(op) => {
            let rsp = get_reg(branch, Reg::Rsp);
            let value = load_u64(branch, rsp, memory_effects)?;
            set_reg(branch, Reg::Rsp, rsp.wrapping_add(8));
            write_operand(program, branch, op, value, memory_effects, memory_diff)?;
            branch.pc = next_pc;
        }
        Opcode::Call(label) => {
            let target = *program
                .text_labels
                .get(label)
                .ok_or_else(|| format!("unknown call target: {label}"))?;
            let return_address = program
                .instructions
                .get(next_pc)
                .map(|entry| entry.address)
                .unwrap_or(instruction.address + 4);
            let rsp = get_reg(branch, Reg::Rsp).wrapping_sub(8);
            set_reg(branch, Reg::Rsp, rsp);
            store_u64(branch, rsp, return_address, memory_effects, memory_diff)?;
            branch.pc = target;
        }
        Opcode::Ret => {
            let rsp = get_reg(branch, Reg::Rsp);
            let return_address = load_u64(branch, rsp, memory_effects)?;
            set_reg(branch, Reg::Rsp, rsp.wrapping_add(8));
            let target = program
                .instructions
                .iter()
                .position(|entry| entry.address == return_address)
                .ok_or_else(|| format!("unknown return address: {return_address:#x}"))?;
            branch.pc = target;
        }
        Opcode::Jmp(label) => {
            branch.pc = resolve_jump(program, label)?;
        }
        Opcode::Jz(label) => {
            branch.pc = if branch.flags.zf {
                resolve_jump(program, label)?
            } else {
                next_pc
            };
        }
        Opcode::Jnz(label) => {
            branch.pc = if !branch.flags.zf {
                resolve_jump(program, label)?
            } else {
                next_pc
            };
        }
        Opcode::Jg(label) => {
            branch.pc = if !branch.flags.zf && branch.flags.sf == branch.flags.of {
                resolve_jump(program, label)?
            } else {
                next_pc
            };
        }
        Opcode::Jge(label) => {
            branch.pc = if branch.flags.sf == branch.flags.of {
                resolve_jump(program, label)?
            } else {
                next_pc
            };
        }
        Opcode::Syscall => match get_reg(branch, Reg::Rax) {
            0 => {
                let fd = get_reg(branch, Reg::Rdi);
                if fd != 0 {
                    return Err("only stdin fd=0 is supported for sys_read".to_string());
                }
                let buf = get_reg(branch, Reg::Rsi);
                let count = get_reg(branch, Reg::Rdx) as usize;
                let remaining = branch.stdin[branch.stdin_cursor..].to_vec();
                let n = remaining.len().min(count);
                ensure_region(branch, buf, count as u64, true, true, "stdin buffer");
                for (index, byte) in remaining.iter().take(n).enumerate() {
                    store_u8(branch, buf + index as u64, *byte, memory_effects, memory_diff)?;
                }
                branch.stdin_cursor += n;
                set_reg(branch, Reg::Rax, n as u64);
                heap_effects.push(format!("stdin -> {n} bytes"));
                branch.pc = next_pc;
            }
            1 => {
                let fd = get_reg(branch, Reg::Rdi);
                let buf = get_reg(branch, Reg::Rsi);
                let count = get_reg(branch, Reg::Rdx) as usize;
                let text = load_capped_string(branch, buf, count, memory_effects)?;
                match fd {
                    1 => branch.stdout.push_str(&text),
                    2 => branch.stderr.push_str(&text),
                    other => {
                        branch
                            .fd_table
                            .entry(other)
                            .and_modify(|entry| entry.push_str(&text))
                            .or_insert(text.clone());
                    }
                }
                set_reg(branch, Reg::Rax, count as u64);
                heap_effects.push(format!("write(fd={fd}) -> {:?}", text));
                branch.pc = next_pc;
            }
            2 => {
                let path_addr = get_reg(branch, Reg::Rdi);
                let path = load_c_string(branch, path_addr, memory_effects)?;
                let fd = branch.next_fd;
                branch.next_fd += 1;
                branch.fd_table.insert(fd, String::new());
                set_reg(branch, Reg::Rax, fd);
                heap_effects.push(format!("open({path}) -> fd {fd}"));
                branch.pc = next_pc;
            }
            3 => {
                let fd = get_reg(branch, Reg::Rdi);
                branch.fd_table.remove(&fd);
                set_reg(branch, Reg::Rax, 0);
                heap_effects.push(format!("close(fd={fd})"));
                branch.pc = next_pc;
            }
            9 => {
                let len = get_reg(branch, Reg::Rsi);
                let prot = get_reg(branch, Reg::Rdx);
                let addr = branch.next_mmap;
                branch.next_mmap += align_up(len.max(4096), 4096);
                ensure_region(branch, addr, len, prot & 0x1 != 0, prot & 0x2 != 0, "mmap");
                set_reg(branch, Reg::Rax, addr);
                heap_effects.push(format!("mmap(len={len}, prot={prot:#x}) -> {addr:#010x}"));
                branch.pc = next_pc;
            }
            10 => {
                let addr = get_reg(branch, Reg::Rdi);
                let len = get_reg(branch, Reg::Rsi);
                let prot = get_reg(branch, Reg::Rdx);
                update_region_perms(branch, addr, len, prot & 0x1 != 0, prot & 0x2 != 0);
                set_reg(branch, Reg::Rax, 0);
                heap_effects.push(format!("mprotect({addr:#010x}, len={len}, prot={prot:#x})"));
                branch.pc = next_pc;
            }
            39 => {
                set_reg(branch, Reg::Rax, GETPID_VALUE);
                heap_effects.push(format!("getpid() -> {GETPID_VALUE}"));
                branch.pc = next_pc;
            }
            57 => {
                let mut parent = branch.clone();
                parent.id = "parent".to_string();
                parent.label = "parent".to_string();
                parent.pc = next_pc;
                set_reg(&mut parent, Reg::Rax, FORK_CHILD_PID);

                let mut child = branch.clone();
                child.id = "child".to_string();
                child.label = "child".to_string();
                child.pc = next_pc;
                set_reg(&mut child, Reg::Rax, 0);

                return Ok(BranchOutcome::Split { parent, child });
            }
            59 => {
                let prog = load_c_string(branch, get_reg(branch, Reg::Rdi), memory_effects)?;
                let argv_addr = get_reg(branch, Reg::Rsi);
                let arg0 = load_u64(branch, argv_addr, memory_effects)?;
                let arg1 = load_u64(branch, argv_addr + 8, memory_effects)?;
                let mut pieces = Vec::new();
                if arg0 != 0 {
                    pieces.push(load_c_string(branch, arg0, memory_effects)?);
                }
                if arg1 != 0 {
                    pieces.push(load_c_string(branch, arg1, memory_effects)?);
                }
                let output = if prog == "/bin/echo" && pieces.len() > 1 {
                    format!("{}\n", pieces[1])
                } else {
                    String::new()
                };
                branch.pending_exec_output = Some(output);
                branch.exit_code = Some(0);
                heap_effects.push(format!("execve({prog}) -> image replaced"));
                branch.pc = next_pc;
            }
            60 => {
                let code = get_reg(branch, Reg::Rdi);
                branch.exit_code = Some(code);
                heap_effects.push(format!("exit({code})"));
                branch.pc = next_pc;
            }
            61 => {
                let pid = get_reg(branch, Reg::Rdi);
                set_reg(branch, Reg::Rax, pid);
                heap_effects.push(format!("wait4(pid={pid})"));
                branch.pc = next_pc;
            }
            87 => {
                let path_addr = get_reg(branch, Reg::Rdi);
                let path = load_c_string(branch, path_addr, memory_effects)?;
                set_reg(branch, Reg::Rax, 0);
                heap_effects.push(format!("unlink({path})"));
                branch.pc = next_pc;
            }
            other => {
                return Err(format!("unsupported syscall number: {other}"));
            }
        },
    }

    Ok(BranchOutcome::Complete(branch.clone()))
}

fn parse_program(source: &str) -> Result<Program, String> {
    let mut section = Section::Text;
    let mut current_global = "_start".to_string();
    let mut symbol_values = HashMap::new();
    let mut text_labels = HashMap::new();
    let mut data_memory = BTreeMap::new();
    let mut data_regions = Vec::new();
    let mut instructions = Vec::new();
    let mut data_cursor = DATA_BASE as i64;

    for (index, raw_line) in source.lines().enumerate() {
        let line_no = index + 1;
        let without_comment = raw_line.split(';').next().unwrap_or_default().trim();
        if without_comment.is_empty() {
            continue;
        }

        let mut line = without_comment.to_string();
        if line.eq_ignore_ascii_case("section .text") {
            section = Section::Text;
            continue;
        }
        if line.eq_ignore_ascii_case("section .data") {
            section = Section::Data;
            continue;
        }
        if line.eq_ignore_ascii_case("section .bss") {
            section = Section::Bss;
            continue;
        }
        if line.starts_with("global ") {
            continue;
        }

        let mut label_for_line = None::<String>;
        if let Some((label, rest)) = split_label(&line) {
            let resolved = resolve_label_name(label, &current_global);
            if !label.starts_with('.') {
                current_global = resolved.clone();
            }
            if section == Section::Text {
                text_labels.insert(resolved.clone(), instructions.len());
                symbol_values.insert(resolved.clone(), (TEXT_BASE + (instructions.len() as u64 * 4)) as i64);
            } else {
                symbol_values.insert(resolved.clone(), data_cursor);
            }
            label_for_line = Some(resolved);
            line = rest.trim().to_string();
            if line.is_empty() {
                continue;
            }
        }

        match section {
            Section::Data => {
                if let Some(name) = label_for_line.as_deref() {
                    parse_data_line(
                        &line,
                        line_no,
                        name,
                        &mut symbol_values,
                        &mut data_memory,
                        &mut data_cursor,
                        &mut data_regions,
                    )?;
                } else {
                    return Err(format!("{line_no} 行目: .data/.bss 行にはラベルが必要です。"));
                }
            }
            Section::Bss => {
                if let Some(name) = label_for_line.as_deref() {
                    parse_bss_line(
                        &line,
                        line_no,
                        name,
                        &mut symbol_values,
                        &mut data_memory,
                        &mut data_cursor,
                        &mut data_regions,
                    )?;
                } else {
                    return Err(format!("{line_no} 行目: .bss 行にはラベルが必要です。"));
                }
            }
            Section::Text => {
                let opcode = parse_instruction(&line, line_no, &symbol_values, &current_global)?;
                instructions.push(Instruction {
                    address: TEXT_BASE + (instructions.len() as u64 * 4),
                    line_no,
                    source: line,
                    opcode,
                });
            }
        }
    }

    let entry_index = *text_labels
        .get("_start")
        .ok_or_else(|| "_start label is required".to_string())?;

    let relevant_registers = collect_relevant_registers(&instructions);
    let uses_stdin = detect_stdin_usage(&instructions);

    Ok(Program {
        instructions,
        text_labels,
        data_memory,
        data_regions,
        entry_index,
        relevant_registers,
        uses_stdin,
    })
}

fn parse_data_line(
    line: &str,
    line_no: usize,
    label: &str,
    symbol_values: &mut HashMap<String, i64>,
    data_memory: &mut BTreeMap<u64, u8>,
    data_cursor: &mut i64,
    data_regions: &mut Vec<Region>,
) -> Result<(), String> {
    let lower = line.to_ascii_lowercase();
    if let Some(rest) = lower.strip_prefix("equ ") {
        let expr = &line[(line.len() - rest.len())..];
        let value = eval_expression(expr.trim(), symbol_values, *data_cursor)?;
        symbol_values.insert(label.to_string(), value);
        return Ok(());
    }

    if let Some(rest) = line.strip_prefix("dq ") {
        let values = split_csv(rest)
            .into_iter()
            .map(|entry| eval_expression(entry.trim(), symbol_values, *data_cursor))
            .collect::<Result<Vec<_>, _>>()?;
        let start = *data_cursor as u64;
        for value in values {
            store_literal_u64(data_memory, *data_cursor as u64, value as u64);
            *data_cursor += 8;
        }
        data_regions.push(Region {
            start,
            len: (*data_cursor as u64) - start,
            read: true,
            write: true,
            label: label.to_string(),
        });
        return Ok(());
    }

    if let Some(rest) = line.strip_prefix("db ") {
        let items = split_csv(rest);
        let start = *data_cursor as u64;
        for item in items {
            let trimmed = item.trim();
            if trimmed.starts_with('"') && trimmed.ends_with('"') {
                for byte in trimmed[1..trimmed.len() - 1].bytes() {
                    data_memory.insert(*data_cursor as u64, byte);
                    *data_cursor += 1;
                }
            } else {
                let value = eval_expression(trimmed, symbol_values, *data_cursor)? as u8;
                data_memory.insert(*data_cursor as u64, value);
                *data_cursor += 1;
            }
        }
        data_regions.push(Region {
            start,
            len: (*data_cursor as u64) - start,
            read: true,
            write: true,
            label: label.to_string(),
        });
        return Ok(());
    }

    Err(format!("{line_no} 行目: 未対応の .data 定義です: {line}"))
}

fn parse_bss_line(
    line: &str,
    line_no: usize,
    label: &str,
    symbol_values: &mut HashMap<String, i64>,
    data_memory: &mut BTreeMap<u64, u8>,
    data_cursor: &mut i64,
    data_regions: &mut Vec<Region>,
) -> Result<(), String> {
    let (directive, rest) = line
        .split_once(' ')
        .ok_or_else(|| format!("{line_no} 行目: .bss 定義を解釈できません。"))?;
    let count = eval_expression(rest.trim(), symbol_values, *data_cursor)? as u64;
    let bytes = match directive.to_ascii_lowercase().as_str() {
        "resb" => count,
        "resq" => count * 8,
        other => {
            return Err(format!("{line_no} 行目: 未対応の .bss 定義です: {other}"));
        }
    };
    let start = *data_cursor as u64;
    for offset in 0..bytes {
        data_memory.insert(start + offset, 0);
    }
    *data_cursor += bytes as i64;
    data_regions.push(Region {
        start,
        len: bytes,
        read: true,
        write: true,
        label: label.to_string(),
    });
    Ok(())
}

fn parse_instruction(
    line: &str,
    line_no: usize,
    symbols: &HashMap<String, i64>,
    current_global: &str,
) -> Result<Opcode, String> {
    let normalized = line.replace(',', " ");
    let mut parts = normalized.split_whitespace();
    let opcode = parts
        .next()
        .ok_or_else(|| format!("{line_no} 行目: 命令がありません。"))?
        .to_ascii_lowercase();
    let operands = split_operands(line);

    match opcode.as_str() {
        "mov" => expect_two_operands(&opcode, line_no, &operands)
            .and_then(|(lhs, rhs)| Ok(Opcode::Mov(parse_operand(lhs, line_no, symbols, current_global)?, parse_operand(rhs, line_no, symbols, current_global)?))),
        "add" => expect_two_operands(&opcode, line_no, &operands)
            .and_then(|(lhs, rhs)| Ok(Opcode::Add(parse_operand(lhs, line_no, symbols, current_global)?, parse_operand(rhs, line_no, symbols, current_global)?))),
        "sub" => expect_two_operands(&opcode, line_no, &operands)
            .and_then(|(lhs, rhs)| Ok(Opcode::Sub(parse_operand(lhs, line_no, symbols, current_global)?, parse_operand(rhs, line_no, symbols, current_global)?))),
        "xor" => expect_two_operands(&opcode, line_no, &operands)
            .and_then(|(lhs, rhs)| Ok(Opcode::Xor(parse_operand(lhs, line_no, symbols, current_global)?, parse_operand(rhs, line_no, symbols, current_global)?))),
        "cmp" => expect_two_operands(&opcode, line_no, &operands)
            .and_then(|(lhs, rhs)| Ok(Opcode::Cmp(parse_operand(lhs, line_no, symbols, current_global)?, parse_operand(rhs, line_no, symbols, current_global)?))),
        "lea" => {
            let (lhs, rhs) = expect_two_operands(&opcode, line_no, &operands)?;
            let reg = parse_reg(lhs).ok_or_else(|| format!("{line_no} 行目: lea の左辺はレジスタである必要があります。"))?;
            let address = parse_address(rhs, line_no, symbols, current_global)?;
            Ok(Opcode::Lea(reg, address))
        }
        "inc" => expect_one_operand(&opcode, line_no, &operands)
            .and_then(|operand| Ok(Opcode::Inc(parse_operand(operand, line_no, symbols, current_global)?))),
        "push" => expect_one_operand(&opcode, line_no, &operands)
            .and_then(|operand| Ok(Opcode::Push(parse_operand(operand, line_no, symbols, current_global)?))),
        "pop" => expect_one_operand(&opcode, line_no, &operands)
            .and_then(|operand| Ok(Opcode::Pop(parse_operand(operand, line_no, symbols, current_global)?))),
        "call" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Call(resolve_label_name(operand.trim(), current_global))),
        "jmp" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Jmp(resolve_label_name(operand.trim(), current_global))),
        "jz" | "je" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Jz(resolve_label_name(operand.trim(), current_global))),
        "jnz" | "jne" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Jnz(resolve_label_name(operand.trim(), current_global))),
        "jg" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Jg(resolve_label_name(operand.trim(), current_global))),
        "jge" => expect_one_operand(&opcode, line_no, &operands)
            .map(|operand| Opcode::Jge(resolve_label_name(operand.trim(), current_global))),
        "ret" => Ok(Opcode::Ret),
        "syscall" => Ok(Opcode::Syscall),
        other => Err(format!("{line_no} 行目: 未対応の命令です: {other}")),
    }
}

fn parse_operand(
    text: &str,
    line_no: usize,
    symbols: &HashMap<String, i64>,
    current_global: &str,
) -> Result<Operand, String> {
    let operand = text.trim();
    let operand = operand.strip_prefix("qword ").unwrap_or(operand).trim();

    if let Some(reg) = parse_reg(operand) {
        return Ok(Operand::Reg(reg));
    }

    if operand.starts_with('[') {
        return Ok(Operand::Mem(parse_address(operand, line_no, symbols, current_global)?));
    }

    Ok(Operand::Imm(eval_expression(
        &resolve_local_references(operand, current_global),
        symbols,
        0,
    )?))
}

fn parse_address(
    text: &str,
    line_no: usize,
    symbols: &HashMap<String, i64>,
    current_global: &str,
) -> Result<AddressExpr, String> {
    let trimmed = text.trim();
    if !(trimmed.starts_with('[') && trimmed.ends_with(']')) {
        return Err(format!("{line_no} 行目: アドレス式を解釈できません: {text}"));
    }
    let inner = resolve_local_references(&trimmed[1..trimmed.len() - 1], current_global);
    let mut expr = AddressExpr {
        base: None,
        index: None,
        disp: 0,
    };

    for raw_part in inner.replace('-', "+-").split('+') {
        let part = raw_part.trim();
        if part.is_empty() {
            continue;
        }
        if let Some((left, right)) = part.split_once('*') {
            let reg = parse_reg(left.trim())
                .ok_or_else(|| format!("{line_no} 行目: index レジスタを解釈できません: {part}"))?;
            let scale = eval_expression(right.trim(), symbols, 0)?;
            expr.index = Some((reg, scale));
            continue;
        }
        if let Some(reg) = parse_reg(part) {
            expr.base = Some(reg);
            continue;
        }
        expr.disp += eval_expression(part, symbols, 0)?;
    }

    Ok(expr)
}

fn collect_relevant_registers(instructions: &[Instruction]) -> Vec<Reg> {
    let mut regs = BTreeSet::new();

    for instruction in instructions {
        collect_opcode_registers(&instruction.opcode, &mut regs);
    }

    regs.into_iter().collect()
}

fn detect_stdin_usage(instructions: &[Instruction]) -> bool {
    let mut rax_value = None::<u64>;

    for instruction in instructions {
        match &instruction.opcode {
            Opcode::Mov(Operand::Reg(Reg::Rax), Operand::Imm(value)) => {
                rax_value = Some(*value as u64);
            }
            Opcode::Xor(Operand::Reg(Reg::Rax), Operand::Reg(Reg::Rax))
            | Opcode::Sub(Operand::Reg(Reg::Rax), Operand::Reg(Reg::Rax)) => {
                rax_value = Some(0);
            }
            Opcode::Inc(Operand::Reg(Reg::Rax)) => {
                rax_value = rax_value.map(|value| value.wrapping_add(1));
            }
            Opcode::Syscall => {
                if rax_value == Some(0) {
                    return true;
                }
                rax_value = None;
            }
            opcode if writes_reg(opcode, Reg::Rax) => {
                rax_value = None;
            }
            _ => {}
        }
    }

    false
}

fn writes_reg(opcode: &Opcode, reg: Reg) -> bool {
    match opcode {
        Opcode::Mov(Operand::Reg(dst), _)
        | Opcode::Add(Operand::Reg(dst), _)
        | Opcode::Sub(Operand::Reg(dst), _)
        | Opcode::Xor(Operand::Reg(dst), _)
        | Opcode::Inc(Operand::Reg(dst))
        | Opcode::Pop(Operand::Reg(dst)) => *dst == reg,
        Opcode::Lea(dst, _) => *dst == reg,
        _ => false,
    }
}

fn collect_opcode_registers(opcode: &Opcode, regs: &mut BTreeSet<Reg>) {
    match opcode {
        Opcode::Mov(lhs, rhs)
        | Opcode::Add(lhs, rhs)
        | Opcode::Sub(lhs, rhs)
        | Opcode::Xor(lhs, rhs)
        | Opcode::Cmp(lhs, rhs) => {
            collect_operand_registers(lhs, regs);
            collect_operand_registers(rhs, regs);
        }
        Opcode::Inc(operand) | Opcode::Push(operand) | Opcode::Pop(operand) => {
            collect_operand_registers(operand, regs);
            if matches!(opcode, Opcode::Push(_) | Opcode::Pop(_)) {
                regs.insert(Reg::Rsp);
            }
        }
        Opcode::Lea(reg, expr) => {
            regs.insert(*reg);
            collect_address_registers(expr, regs);
        }
        Opcode::Call(_) | Opcode::Ret => {
            regs.insert(Reg::Rsp);
        }
        Opcode::Jmp(_) | Opcode::Jz(_) | Opcode::Jnz(_) | Opcode::Jg(_) | Opcode::Jge(_) | Opcode::Syscall => {}
    }
}

fn collect_operand_registers(operand: &Operand, regs: &mut BTreeSet<Reg>) {
    match operand {
        Operand::Reg(reg) => {
            regs.insert(*reg);
        }
        Operand::Mem(expr) => collect_address_registers(expr, regs),
        Operand::Imm(_) => {}
    }
}

fn collect_address_registers(expr: &AddressExpr, regs: &mut BTreeSet<Reg>) {
    if let Some(base) = expr.base {
        regs.insert(base);
    }
    if let Some((index, _)) = expr.index {
        regs.insert(index);
    }
}

fn read_operand(
    program: &Program,
    branch: &mut RuntimeBranch,
    operand: &Operand,
    memory_effects: &mut Vec<String>,
) -> Result<u64, String> {
    match operand {
        Operand::Reg(reg) => Ok(get_reg(branch, *reg)),
        Operand::Imm(value) => Ok(*value as u64),
        Operand::Mem(expr) => {
            let addr = eval_address(program, branch, expr)?;
            load_u64(branch, addr, memory_effects)
        }
    }
}

fn write_operand(
    program: &Program,
    branch: &mut RuntimeBranch,
    operand: &Operand,
    value: u64,
    memory_effects: &mut Vec<String>,
    memory_diff: &mut Vec<KeyValue>,
) -> Result<(), String> {
    match operand {
        Operand::Reg(reg) => {
            set_reg(branch, *reg, value);
            Ok(())
        }
        Operand::Mem(expr) => {
            let addr = eval_address(program, branch, expr)?;
            store_u64(branch, addr, value, memory_effects, memory_diff)
        }
        Operand::Imm(_) => Err("immediate cannot be written".to_string()),
    }
}

fn eval_address(program: &Program, branch: &RuntimeBranch, expr: &AddressExpr) -> Result<u64, String> {
    let mut value = expr.disp;
    if let Some(base) = expr.base {
        value += get_reg(branch, base) as i64;
    }
    if let Some((index, scale)) = expr.index {
        value += (get_reg(branch, index) as i64) * scale;
    }
    if value < 0 {
        return Err(format!("negative address is unsupported: {value}"));
    }
    let address = value as u64;
    if address == 0 && expr.base.is_none() && expr.index.is_none() && expr.disp == 0 {
        return Err(format!("invalid address expression in program with {} instructions", program.instructions.len()));
    }
    Ok(address)
}

fn load_u64(
    branch: &mut RuntimeBranch,
    addr: u64,
    memory_effects: &mut Vec<String>,
) -> Result<u64, String> {
    ensure_access(branch, addr, 8, true, false)?;
    let mut value = 0_u64;
    for offset in 0..8 {
        value |= (u64::from(*branch.memory.get(&(addr + offset)).unwrap_or(&0))) << (offset * 8);
    }
    memory_effects.push(format!("load qword [{addr:#010x}] -> {value:#x}"));
    Ok(value)
}

fn store_u64(
    branch: &mut RuntimeBranch,
    addr: u64,
    value: u64,
    memory_effects: &mut Vec<String>,
    memory_diff: &mut Vec<KeyValue>,
) -> Result<(), String> {
    ensure_access(branch, addr, 8, false, true)?;
    store_literal_u64(&mut branch.memory, addr, value);
    memory_effects.push(format!("store qword [{addr:#010x}] <- {value:#x}"));
    memory_diff.push(KeyValue {
        key: format!("[{addr:#010x}]"),
        value: format!("{value:#x}"),
    });
    Ok(())
}

fn store_u8(
    branch: &mut RuntimeBranch,
    addr: u64,
    value: u8,
    memory_effects: &mut Vec<String>,
    memory_diff: &mut Vec<KeyValue>,
) -> Result<(), String> {
    ensure_access(branch, addr, 1, false, true)?;
    branch.memory.insert(addr, value);
    memory_effects.push(format!("store byte [{addr:#010x}] <- {value:#x}"));
    memory_diff.push(KeyValue {
        key: format!("[{addr:#010x}]"),
        value: format!("{value:#x}"),
    });
    Ok(())
}

fn ensure_access(
    branch: &mut RuntimeBranch,
    addr: u64,
    len: u64,
    read: bool,
    write: bool,
) -> Result<(), String> {
    if let Some(region) = branch
        .regions
        .iter()
        .find(|region| addr >= region.start && addr + len <= region.start + region.len)
    {
        if read && !region.read {
            branch
                .diagnostics
                .push(format!("fault at {addr:#010x}: read denied in {}", region.label));
            branch.exit_code = Some(139);
            return Err(format!("memory read fault at {addr:#010x}"));
        }
        if write && !region.write {
            branch
                .diagnostics
                .push(format!("fault at {addr:#010x}: write denied in {}", region.label));
            branch.exit_code = Some(139);
            return Err(format!("memory write fault at {addr:#010x}"));
        }
        return Ok(());
    }

    branch
        .diagnostics
        .push(format!("fault at {addr:#010x}: unmapped memory"));
    branch.exit_code = Some(139);
    Err(format!("unmapped memory access at {addr:#010x}"))
}

fn ensure_region(branch: &mut RuntimeBranch, start: u64, len: u64, read: bool, write: bool, label: &str) {
    if branch
        .regions
        .iter()
        .any(|region| region.start == start && region.len == len)
    {
        return;
    }

    branch.regions.push(Region {
        start,
        len,
        read,
        write,
        label: label.to_string(),
    });
    for offset in 0..len {
        branch.memory.entry(start + offset).or_insert(0);
    }
}

fn update_region_perms(branch: &mut RuntimeBranch, start: u64, len: u64, read: bool, write: bool) {
    for region in &mut branch.regions {
        if region.start == start && region.len == len {
            region.read = read;
            region.write = write;
        }
    }
}

fn get_reg(branch: &RuntimeBranch, reg: Reg) -> u64 {
    branch.regs.get(&reg).copied().unwrap_or_default()
}

fn set_reg(branch: &mut RuntimeBranch, reg: Reg, value: u64) {
    branch.regs.insert(reg, value);
}

fn resolve_jump(program: &Program, label: &str) -> Result<usize, String> {
    program
        .text_labels
        .get(label)
        .copied()
        .ok_or_else(|| format!("unknown jump target: {label}"))
}

fn focus_for(
    instruction: &Instruction,
    register_diff: &[KeyValue],
    memory_effects: &[String],
    exit_code: Option<u64>,
) -> String {
    if let Some(code) = exit_code {
        return format!("{} を実行し、exit code {} で停止", instruction.source, code);
    }

    if !register_diff.is_empty() {
        return register_diff
            .iter()
            .map(|entry| format!("{}={}", entry.key, entry.value))
            .collect::<Vec<_>>()
            .join(", ");
    }

    if !memory_effects.is_empty() {
        return memory_effects.join(" / ");
    }

    instruction.source.clone()
}

fn format_signed_register(value: u64) -> String {
    format!("{value:#x} / {}", value as i64)
}

fn format_unsigned_value(value: u64) -> String {
    format!("{value:#010x} / {value}")
}

fn snapshot_registers(regs: &HashMap<Reg, u64>, flags: &Flags, rip_before: u64, rip_after: u64) -> Vec<KeyValue> {
    let mut entries = vec![
        KeyValue {
            key: "rip(before)".to_string(),
            value: format_unsigned_value(rip_before),
        },
        KeyValue {
            key: "rip(after)".to_string(),
            value: format_unsigned_value(rip_after),
        },
    ];

    for reg in [
        Reg::Rax,
        Reg::Rbx,
        Reg::Rcx,
        Reg::Rdx,
        Reg::Rsi,
        Reg::Rdi,
        Reg::Rsp,
        Reg::Rbp,
        Reg::R8,
        Reg::R9,
        Reg::R10,
        Reg::R11,
        Reg::R12,
        Reg::R13,
    ] {
        entries.push(KeyValue {
            key: reg.name().to_string(),
            value: format_signed_register(regs.get(&reg).copied().unwrap_or_default()),
        });
    }
    entries.push(KeyValue {
        key: "ZF".to_string(),
        value: u8::from(flags.zf).to_string(),
    });
    entries.push(KeyValue {
        key: "SF".to_string(),
        value: u8::from(flags.sf).to_string(),
    });
    entries.push(KeyValue {
        key: "CF".to_string(),
        value: u8::from(flags.cf).to_string(),
    });
    entries.push(KeyValue {
        key: "OF".to_string(),
        value: u8::from(flags.of).to_string(),
    });
    entries.push(KeyValue {
        key: "rflags".to_string(),
        value: format_unsigned_value(flags.as_rflags()),
    });
    entries
}

fn snapshot_stack(branch: &RuntimeBranch) -> Vec<KeyValue> {
    let rsp = branch.regs.get(&Reg::Rsp).copied().unwrap_or(STACK_TOP);
    if rsp >= STACK_TOP {
        return Vec::new();
    }
    let mut entries = Vec::new();
    let mut addr = rsp;
    while addr < STACK_TOP && entries.len() < 32 {
        let mut qword = 0_u64;
        for offset in 0..8u64 {
            qword |= u64::from(*branch.memory.get(&(addr + offset)).unwrap_or(&0)) << (offset * 8);
        }
        entries.push(KeyValue {
            key: format!("{addr:#010x}"),
            value: format!("{qword:#018x}"),
        });
        addr += 8;
    }
    entries
}

fn diff_registers(
    before_regs: &HashMap<Reg, u64>,
    after_regs: &HashMap<Reg, u64>,
    before_flags: &Flags,
    after_flags: &Flags,
) -> Vec<KeyValue> {
    let mut diff = Vec::new();
    for reg in [
        Reg::Rax,
        Reg::Rbx,
        Reg::Rcx,
        Reg::Rdx,
        Reg::Rsi,
        Reg::Rdi,
        Reg::Rsp,
        Reg::Rbp,
        Reg::R8,
        Reg::R9,
        Reg::R10,
        Reg::R11,
        Reg::R12,
        Reg::R13,
    ] {
        let before = before_regs.get(&reg).copied().unwrap_or_default();
        let after = after_regs.get(&reg).copied().unwrap_or_default();
        if before != after {
            diff.push(KeyValue {
                key: reg.name().to_string(),
                value: format_signed_register(after),
            });
        }
    }

    for (name, before, after) in [
        ("ZF", before_flags.zf, after_flags.zf),
        ("SF", before_flags.sf, after_flags.sf),
        ("CF", before_flags.cf, after_flags.cf),
        ("OF", before_flags.of, after_flags.of),
    ] {
        if before != after {
            diff.push(KeyValue {
                key: name.to_string(),
                value: u8::from(after).to_string(),
            });
        }
    }

    diff
}

fn branch_summary(branch: &RuntimeBranch) -> String {
    let exit = branch
        .exit_code
        .map(|code| format!("exit={code}"))
        .unwrap_or_else(|| "running".to_string());
    format!("{} steps, {}", branch.trace.len(), exit)
}

fn to_output_lines(text: &str) -> Vec<String> {
    if text.is_empty() {
        Vec::new()
    } else {
        text.lines().map(str::to_string).collect()
    }
}

fn load_capped_string(
    branch: &mut RuntimeBranch,
    addr: u64,
    len: usize,
    memory_effects: &mut Vec<String>,
) -> Result<String, String> {
    ensure_access(branch, addr, len as u64, true, false)?;
    let bytes = (0..len)
        .map(|offset| *branch.memory.get(&(addr + offset as u64)).unwrap_or(&0))
        .collect::<Vec<_>>();
    memory_effects.push(format!("read bytes [{addr:#010x}] len={len}"));
    Ok(String::from_utf8_lossy(&bytes).to_string())
}

fn load_c_string(
    branch: &mut RuntimeBranch,
    addr: u64,
    memory_effects: &mut Vec<String>,
) -> Result<String, String> {
    let mut bytes = Vec::new();
    for offset in 0..256_u64 {
        ensure_access(branch, addr + offset, 1, true, false)?;
        let byte = *branch.memory.get(&(addr + offset)).unwrap_or(&0);
        if byte == 0 {
            break;
        }
        bytes.push(byte);
    }
    memory_effects.push(format!("read c-string [{addr:#010x}]"));
    Ok(String::from_utf8_lossy(&bytes).to_string())
}

fn split_label(line: &str) -> Option<(&str, &str)> {
    let (label, rest) = line.split_once(':')?;
    if label.chars().all(|ch| ch.is_ascii_alphanumeric() || ch == '_' || ch == '.') {
        Some((label.trim(), rest))
    } else {
        None
    }
}

fn resolve_label_name(label: &str, current_global: &str) -> String {
    if label.starts_with('.') {
        format!("{current_global}{label}")
    } else {
        label.to_string()
    }
}

fn resolve_local_references(input: &str, current_global: &str) -> String {
    input
        .split_whitespace()
        .map(|part| {
            if part.starts_with('.') {
                format!("{current_global}{part}")
            } else {
                part.to_string()
            }
        })
        .collect::<Vec<_>>()
        .join(" ")
}

fn parse_reg(input: &str) -> Option<Reg> {
    match input.trim().to_ascii_lowercase().as_str() {
        "rax" => Some(Reg::Rax),
        "rbx" => Some(Reg::Rbx),
        "rcx" => Some(Reg::Rcx),
        "rdx" => Some(Reg::Rdx),
        "rsi" => Some(Reg::Rsi),
        "rdi" => Some(Reg::Rdi),
        "rsp" => Some(Reg::Rsp),
        "rbp" => Some(Reg::Rbp),
        "r8" => Some(Reg::R8),
        "r9" => Some(Reg::R9),
        "r10" => Some(Reg::R10),
        "r11" => Some(Reg::R11),
        "r12" => Some(Reg::R12),
        "r13" => Some(Reg::R13),
        _ => None,
    }
}

fn expect_one_operand<'a>(opcode: &str, line_no: usize, operands: &'a [String]) -> Result<&'a str, String> {
    if operands.len() != 1 {
        return Err(format!("{line_no} 行目: {opcode} は 1 オペランドです。"));
    }
    Ok(operands[0].as_str())
}

fn expect_two_operands<'a>(
    opcode: &str,
    line_no: usize,
    operands: &'a [String],
) -> Result<(&'a str, &'a str), String> {
    if operands.len() != 2 {
        return Err(format!("{line_no} 行目: {opcode} は 2 オペランドです。"));
    }
    Ok((operands[0].as_str(), operands[1].as_str()))
}

fn split_operands(line: &str) -> Vec<String> {
    let mut result = Vec::new();
    let mut current = String::new();
    let mut bracket_depth = 0_i32;
    let mut after_opcode = false;

    for ch in line.chars() {
        if !after_opcode {
            if ch.is_whitespace() {
                after_opcode = true;
            }
            continue;
        }

        match ch {
            '[' => {
                bracket_depth += 1;
                current.push(ch);
            }
            ']' => {
                bracket_depth -= 1;
                current.push(ch);
            }
            ',' if bracket_depth == 0 => {
                if !current.trim().is_empty() {
                    result.push(current.trim().to_string());
                }
                current.clear();
            }
            _ => current.push(ch),
        }
    }

    if !current.trim().is_empty() {
        result.push(current.trim().to_string());
    }

    result
}

fn split_csv(input: &str) -> Vec<String> {
    let mut result = Vec::new();
    let mut current = String::new();
    let mut in_string = false;

    for ch in input.chars() {
        match ch {
            '"' => {
                in_string = !in_string;
                current.push(ch);
            }
            ',' if !in_string => {
                result.push(current.trim().to_string());
                current.clear();
            }
            _ => current.push(ch),
        }
    }

    if !current.trim().is_empty() {
        result.push(current.trim().to_string());
    }

    result
}

fn eval_expression(input: &str, symbols: &HashMap<String, i64>, current: i64) -> Result<i64, String> {
    let normalized = input.replace('$', &current.to_string()).replace(' ', "");
    let mut total = 0_i64;
    let mut sign = 1_i64;
    let mut token = String::new();

    for ch in normalized.chars().chain(std::iter::once('+')) {
        if ch == '+' || ch == '-' {
            if !token.is_empty() {
                total += sign * parse_atom(&token, symbols)?;
                token.clear();
            }
            sign = if ch == '+' { 1 } else { -1 };
        } else {
            token.push(ch);
        }
    }

    Ok(total)
}

fn parse_atom(token: &str, symbols: &HashMap<String, i64>) -> Result<i64, String> {
    if let Some(value) = symbols.get(token) {
        return Ok(*value);
    }
    if let Some(stripped) = token.strip_prefix("0x") {
        return i64::from_str_radix(stripped, 16).map_err(|_| format!("cannot parse hex literal: {token}"));
    }
    if let Some(stripped) = token.strip_suffix('o') {
        return i64::from_str_radix(stripped, 8).map_err(|_| format!("cannot parse octal literal: {token}"));
    }
    token
        .parse::<i64>()
        .map_err(|_| format!("cannot parse expression atom: {token}"))
}

fn update_add_flags(flags: &mut Flags, lhs: u64, rhs: u64, result: u64) {
    flags.zf = result == 0;
    flags.sf = (result as i64) < 0;
    flags.cf = result < lhs;
    let lhs_signed = lhs as i64;
    let rhs_signed = rhs as i64;
    let result_signed = result as i64;
    flags.of = (lhs_signed > 0 && rhs_signed > 0 && result_signed < 0)
        || (lhs_signed < 0 && rhs_signed < 0 && result_signed > 0);
}

fn update_sub_flags(flags: &mut Flags, lhs: u64, rhs: u64, result: u64) {
    flags.zf = result == 0;
    flags.sf = (result as i64) < 0;
    flags.cf = lhs < rhs;
    let lhs_signed = lhs as i64;
    let rhs_signed = rhs as i64;
    let result_signed = result as i64;
    flags.of = ((lhs_signed ^ rhs_signed) & (lhs_signed ^ result_signed)) < 0;
}

fn store_literal_u64(memory: &mut BTreeMap<u64, u8>, addr: u64, value: u64) {
    for offset in 0..8 {
        memory.insert(addr + offset, ((value >> (offset * 8)) & 0xff) as u8);
    }
}

fn align_up(value: u64, align: u64) -> u64 {
    if value % align == 0 {
        value
    } else {
        value + (align - value % align)
    }
}

#[cfg(test)]
mod tests {
    use super::x86_trace;

    #[test]
    fn traces_cmp_and_branch() {
        let trace = x86_trace(
            "v2",
            "section .text\nglobal _start\n_start:\n mov rax, 42\n cmp rax, 42\n jz .equal\n mov rdi, 1\n.equal:\n mov rdi, 0\n mov rax, 60\n syscall\n",
            "",
        )
        .unwrap();

        assert_eq!(trace.relevant_registers, vec!["rax".to_string(), "rdi".to_string()]);
        assert!(!trace.uses_stdin);

        let branches = trace.branches.unwrap();
        let main = branches.iter().find(|branch| branch.id == "main").unwrap();
        assert!(main.timeline.iter().any(|event| event.detail.contains("cmp rax, 42")));
        assert!(main.timeline.last().unwrap().focus.contains("exit code 0"));
        assert!(main.initial_registers.iter().any(|entry| entry.key == "rax" && entry.value == "0x0 / 0"));
        assert!(main.timeline.iter().any(|event| {
            event.registers.iter().any(|entry| entry.key == "rax" && entry.value == "0x2a / 42")
        }));
    }

    #[test]
    fn traces_stdin_roundtrip() {
        let trace = x86_trace(
            "v6",
            "section .bss\n buf: resb 16\nsection .text\nglobal _start\n_start:\n mov rax, 0\n mov rdi, 0\n lea rsi, [buf]\n mov rdx, 16\n syscall\n mov rdx, rax\n mov rax, 1\n mov rdi, 1\n lea rsi, [buf]\n syscall\n mov rax, 60\n mov rdi, 0\n syscall\n",
            "abc\n",
        )
        .unwrap();

        assert!(trace.uses_stdin);

        let branches = trace.branches.unwrap();
        let main = branches.iter().find(|branch| branch.id == "main").unwrap();
        assert_eq!(main.stdout, vec!["abc"]);
    }

    #[test]
    fn traces_fork_as_two_branches() {
        let trace = x86_trace(
            "v7",
            "section .text\nglobal _start\n_start:\n mov rax, 57\n syscall\n cmp rax, 0\n jz .child\n mov rax, 60\n mov rdi, 42\n syscall\n.child:\n mov rax, 60\n mov rdi, 0\n syscall\n",
            "",
        )
        .unwrap();

        let branches = trace.branches.unwrap();
        assert!(branches.iter().any(|branch| branch.id == "parent"));
        assert!(branches.iter().any(|branch| branch.id == "child"));
    }

    #[test]
    fn traces_all_current_stage_samples() {
        let samples = [
            ("v1", include_str!("../../../v1/asm/exit42.asm"), ""),
            ("v1", include_str!("../../../v1/asm/regs.asm"), ""),
            ("v1", include_str!("../../../v1/asm/swap.asm"), ""),
            ("v1", include_str!("../../../v1/asm/arithmetic.asm"), ""),
            ("v1", include_str!("../../../v1/asm/flags.asm"), ""),
            ("v2", include_str!("../../../v2/asm/cmp_equal.asm"), ""),
            ("v2", include_str!("../../../v2/asm/countdown.asm"), ""),
            ("v2", include_str!("../../../v2/asm/max.asm"), ""),
            ("v3", include_str!("../../../v3/asm/store_load.asm"), ""),
            ("v3", include_str!("../../../v3/asm/memory_modify.asm"), ""),
            ("v3", include_str!("../../../v3/asm/array_sum.asm"), ""),
            ("v4", include_str!("../../../v4/asm/push_pop.asm"), ""),
            ("v4", include_str!("../../../v4/asm/call_ret.asm"), ""),
            ("v4", include_str!("../../../v4/asm/nested_calls.asm"), ""),
            ("v5", include_str!("../../../v5/asm/hello.asm"), ""),
            ("v5", include_str!("../../../v5/asm/write_stderr.asm"), ""),
            ("v5", include_str!("../../../v5/asm/multi_syscall.asm"), ""),
            ("v6", include_str!("../../../v6/asm/fd_table.asm"), ""),
            ("v6", include_str!("../../../v6/asm/read_stdin.asm"), "stdin\n"),
            ("v6", include_str!("../../../v6/asm/write_file.asm"), ""),
            ("v7", include_str!("../../../v7/asm/getpid.asm"), ""),
            ("v7", include_str!("../../../v7/asm/fork_basic.asm"), ""),
            ("v7", include_str!("../../../v7/asm/execve_echo.asm"), ""),
            ("v8", include_str!("../../../v8/asm/mmap_write.asm"), ""),
            ("v8", include_str!("../../../v8/asm/mprotect_crash.asm"), ""),
            ("v8", include_str!("../../../v8/asm/two_mappings.asm"), ""),
        ];

        for (stage, source, stdin) in samples {
            let trace = x86_trace(stage, source, stdin)
                .unwrap_or_else(|error| panic!("{stage} sample failed: {error}"));
            assert!(
                !trace.timeline.is_empty() || trace.branches.as_ref().is_some_and(|branches| !branches.is_empty()),
                "{stage} should yield at least one timeline event"
            );
        }
    }
}
