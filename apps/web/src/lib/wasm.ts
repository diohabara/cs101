import type { ScenarioId, TraceResult } from "./types";

type WasmModule = {
  run_scenario: (scenario: string, source: string) => string;
  run_x86_trace: (stageId: string, source: string, stdin: string) => string;
};

let cachedModule: WasmModule | null = null;

async function loadModule() {
  if (cachedModule) {
    return cachedModule;
  }

  const module = (await import("../generated/wasm/cs101_engine.js")) as unknown as WasmModule;
  cachedModule = module;
  return module;
}

export async function runScenario(scenario: ScenarioId, source: string): Promise<TraceResult> {
  const module = await loadModule();
  const output = module.run_scenario(scenario, source);
  return JSON.parse(output) as TraceResult;
}

export async function runX86Trace(
  stageId: string,
  source: string,
  stdin = ""
): Promise<TraceResult> {
  const module = await loadModule();
  const output = module.run_x86_trace(stageId, source, stdin);
  return JSON.parse(output) as TraceResult;
}
