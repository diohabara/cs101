export type SectionKind = "read" | "write" | "watch" | "break";

export type ScenarioId = "riscv" | "cpu" | "os" | "virt" | "compiler";

export interface SectionCopy {
  beginner: string;
  practitioner: string;
  theory: string;
}

export interface ChapterSection {
  id: string;
  kind: SectionKind;
  title: string;
  copy: SectionCopy;
  anchorEventId?: string;
}

export interface Chapter {
  id: string;
  title: string;
  subtitle: string;
  scenario: ScenarioId;
  starterCode: string;
  observationChecklist: string[];
  sections: ChapterSection[];
}

export interface KeyValue {
  key: string;
  value: string;
}

export interface InstructionState {
  line: number;
  rip_before: string;
  rip_after: string;
  source: string;
}

export interface TraceEvent {
  id: string;
  label: string;
  detail: string;
  focus: string;
  registers: KeyValue[];
  memory: string[];
  types: string[];
  heap: string[];
  instruction?: InstructionState;
  register_diff?: KeyValue[];
  memory_diff?: KeyValue[];
  stack_snapshot?: KeyValue[];
  branch_id?: string;
}

export interface TraceBranch {
  id: string;
  label: string;
  summary: string;
  stdout: string[];
  stderr: string[];
  initial_registers?: KeyValue[];
  timeline: TraceEvent[];
}

export interface TraceResult {
  summary: string;
  diagnostics: string[];
  observations: string[];
  timeline: TraceEvent[];
  relevant_registers?: string[];
  uses_stdin?: boolean;
  active_branch?: string;
  branches?: TraceBranch[];
}
