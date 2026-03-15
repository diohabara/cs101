import { startTransition, useEffect, useMemo, useState } from "react";
import hljs from "highlight.js/lib/core";
import x86asm from "highlight.js/lib/languages/x86asm";
import type { KeyValue, TraceBranch, TraceResult } from "../lib/types";
import { runX86Trace } from "../lib/wasm";

hljs.registerLanguage("x86asm", x86asm);
hljs.registerAliases(["asm", "nasm"], { languageName: "x86asm" });

type RunnableAsmBlockProps = {
  initialSource: string;
  stageId: string;
};

function linesFrom(source: string) {
  return source.replace(/\n$/, "").split("\n");
}

function branchFor(trace: TraceResult | null, branchId: string | null): TraceBranch | null {
  if (!trace?.branches?.length) {
    return null;
  }

  return trace.branches.find((entry) => entry.id === branchId) ?? trace.branches[0] ?? null;
}

function highlighted(line: string) {
  if (!line) {
    return " ";
  }

  return hljs.highlight(line, { language: "x86asm", ignoreIllegals: true }).value;
}

function registerGroups(entries: KeyValue[], relevantRegisters: string[], changedKeys: Set<string>) {
  const relevantSet = new Set(relevantRegisters);
  const meta = entries.filter((entry) =>
    entry.key === "rip(before)" || entry.key === "rip(after)" || entry.key === "rflags"
  );
  const flags = entries.filter((entry) =>
    entry.key === "ZF" || entry.key === "SF" || entry.key === "CF" || entry.key === "OF"
  );
  const registers = entries.filter((entry) => relevantSet.has(entry.key));
  const changed = registers.filter((entry) => changedKeys.has(entry.key));

  return { meta, flags, registers, changed };
}

type MemoryEffect = {
  op: string;
  size: string;
  addr: string;
  value: string;
  direction: "read" | "write";
};

function parseMemoryEffect(item: string): MemoryEffect | null {
  // "store qword [0x7ffefff8] <- 0x5"
  const storeMatch = item.match(/^store\s+(\w+)\s+\[([^\]]+)]\s*<-\s*(.+)$/);
  if (storeMatch) {
    return { op: "STORE", size: storeMatch[1], addr: storeMatch[2], value: storeMatch[3], direction: "write" };
  }

  // "load qword [0x7ffefff8] -> 0x5"
  const loadMatch = item.match(/^load\s+(\w+)\s+\[([^\]]+)]\s*->\s*(.+)$/);
  if (loadMatch) {
    return { op: "LOAD", size: loadMatch[1], addr: loadMatch[2], value: loadMatch[3], direction: "read" };
  }

  // "lea -> rax = 0x00100000"
  const leaMatch = item.match(/^lea\s*->\s*(.+)$/);
  if (leaMatch) {
    return { op: "LEA", size: "", addr: "", value: leaMatch[1], direction: "read" };
  }

  // "read bytes [0x00100000] len=10"
  const readBytesMatch = item.match(/^read\s+bytes\s+\[([^\]]+)]\s*len=(\d+)$/);
  if (readBytesMatch) {
    return { op: "READ", size: `${readBytesMatch[2]}B`, addr: readBytesMatch[1], value: "", direction: "read" };
  }

  // "read c-string [0x00100000]"
  const readCstrMatch = item.match(/^read\s+c-string\s+\[([^\]]+)]$/);
  if (readCstrMatch) {
    return { op: "READ", size: "c-str", addr: readCstrMatch[1], value: "", direction: "read" };
  }

  return null;
}

export function RunnableAsmBlock({ initialSource, stageId }: RunnableAsmBlockProps) {
  const [stdin, setStdin] = useState("");
  const [trace, setTrace] = useState<TraceResult | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [selectedBranchId, setSelectedBranchId] = useState<string | null>(null);
  const [cursor, setCursor] = useState(0);
  const [isRunning, setIsRunning] = useState(false);

  const activeBranch = useMemo(
    () => branchFor(trace, selectedBranchId ?? trace?.active_branch ?? null),
    [selectedBranchId, trace]
  );
  const timeline = activeBranch?.timeline ?? trace?.timeline ?? [];
  const currentEvent = cursor > 0 ? (timeline[cursor - 1] ?? null) : null;
  const nextEvent = timeline[cursor] ?? null;
  const sourceLines = useMemo(() => linesFrom(initialSource), [initialSource]);
  const highlightedLines = useMemo(
    () => sourceLines.map((line) => highlighted(line)),
    [sourceLines]
  );
  const executedLine = currentEvent?.instruction?.line ?? null;
  const ripLine = nextEvent?.instruction?.line ?? null;
  const changedKeys = useMemo(
    () => new Set((currentEvent?.register_diff ?? []).map((entry) => entry.key)),
    [currentEvent]
  );
  const changedMemoryKeys = useMemo(
    () => new Set((currentEvent?.memory_diff ?? []).map((entry) => entry.key)),
    [currentEvent]
  );
  const prevEvent = cursor > 1 ? (timeline[cursor - 2] ?? null) : null;
  const currentStack = cursor === 0 ? [] : (currentEvent?.stack_snapshot ?? []);
  const prevStackKeys = useMemo(
    () => new Map((prevEvent?.stack_snapshot ?? []).map((entry) => [entry.key, entry.value])),
    [prevEvent]
  );
  const changedStackKeys = useMemo(() => {
    const keys = new Set<string>();
    for (const entry of currentStack) {
      const prev = prevStackKeys.get(entry.key);
      if (prev === undefined || prev !== entry.value) {
        keys.add(entry.key);
      }
    }
    return keys;
  }, [currentStack, prevStackKeys]);
  const removedStackEntries = useMemo(() => {
    if (cursor <= 1) return [];
    const currentKeys = new Set(currentStack.map((entry) => entry.key));
    return (prevEvent?.stack_snapshot ?? []).filter((entry) => !currentKeys.has(entry.key));
  }, [cursor, currentStack, prevEvent]);
  const snapshotEntries = cursor === 0
    ? (activeBranch?.initial_registers ?? [])
    : (currentEvent?.registers ?? []);
  const { meta, flags, registers, changed } = useMemo(
    () => registerGroups(snapshotEntries, trace?.relevant_registers ?? [], changedKeys),
    [changedKeys, snapshotEntries, trace?.relevant_registers]
  );
  const showStdinCard = trace?.uses_stdin ?? false;
  const hasMultipleBranches = (trace?.branches?.length ?? 0) > 1;
  const sideEffects = [
    {
      key: "stdout",
      title: "stdout",
      content: activeBranch?.stdout.join("\n") ?? "",
      kind: "pre" as const,
    },
    {
      key: "stderr",
      title: "stderr",
      content: activeBranch?.stderr.join("\n") ?? "",
      kind: "pre" as const,
    },
    {
      key: "memory",
      title: "memory",
      content: currentEvent?.memory ?? [],
      kind: "list" as const,
    },
    {
      key: "diagnostics",
      title: "diagnostics",
      content: trace?.diagnostics ?? [],
      kind: "list" as const,
    },
  ].filter((entry) =>
    Array.isArray(entry.content) ? entry.content.length > 0 : entry.content.trim().length > 0
  );

  useEffect(() => {
    let isCancelled = false;
    const timeoutId = window.setTimeout(() => {
      setIsRunning(true);
      setError(null);

      void runX86Trace(stageId, initialSource, stdin)
        .then((result) => {
          if (isCancelled) {
            return;
          }

          startTransition(() => {
            setTrace(result);
            setSelectedBranchId(result.active_branch ?? result.branches?.[0]?.id ?? null);
            setCursor(0);
          });
        })
        .catch((runError) => {
          if (isCancelled) {
            return;
          }

          setTrace(null);
          setError(runError instanceof Error ? runError.message : "trace の生成に失敗しました。");
        })
        .finally(() => {
          if (!isCancelled) {
            setIsRunning(false);
          }
        });
    }, 160);

    return () => {
      isCancelled = true;
      window.clearTimeout(timeoutId);
    };
  }, [initialSource, stageId, stdin]);

  function handleReset() {
    const defaultBranch = trace?.active_branch ?? trace?.branches?.[0]?.id ?? null;
    setSelectedBranchId(defaultBranch);
    setCursor(0);
    setStdin("");
    setError(null);
  }

  function handleBranchChange(branchId: string) {
    setSelectedBranchId(branchId);
    setCursor(0);
  }

  const headline = (() => {
    if (isRunning) {
      return "_start から再計算中...";
    }

    if (cursor === 0) {
      return nextEvent?.instruction
        ? `次に ${nextEvent.instruction.source} を実行`
        : "未実行";
    }

    return currentEvent?.focus ?? trace?.summary ?? "状態が表示されます。";
  })();

  return (
    <section className="asm-runner">
      <div className="asm-runner__body">
          <div className="asm-runner__grid">
            <div className="asm-runner__main">
              <div className="asm-runner__toolbar">
                <div className="asm-runner__actions">
                  <button
                    aria-label="Prev"
                    className="asm-runner__icon-button"
                    disabled={timeline.length === 0 || cursor === 0}
                    onClick={() => setCursor((current) => Math.max(current - 1, 0))}
                    title="Prev"
                    type="button"
                  >
                    ◀
                  </button>
                  <button
                    aria-label="Next"
                    className="asm-runner__icon-button"
                    disabled={timeline.length === 0 || cursor >= timeline.length}
                    onClick={() => setCursor((current) => Math.min(current + 1, timeline.length))}
                    title="Next"
                    type="button"
                  >
                    ▶
                  </button>
                  <button
                    aria-label="Reset"
                    className="asm-runner__icon-button"
                    title="Reset"
                    type="button"
                    onClick={handleReset}
                  >
                    ↺
                  </button>
                  {timeline.length > 0 ? <span className="asm-runner__chip">{cursor} / {timeline.length} step</span> : null}
                  {hasMultipleBranches && activeBranch ? (
                    <span className="asm-runner__chip">branch: {activeBranch.label}</span>
                  ) : null}
                </div>

                <div className="asm-runner__status">
                  <span>{headline}</span>
                </div>
              </div>

              {showStdinCard ? (
                <div className="asm-runner__card">
                  <div className="asm-runner__card-header">
                    <div>
                      <h4>stdin</h4>
                      <p>`read` 系サンプルの入力です。変更すると `_start` から再計算します。</p>
                    </div>
                  </div>

                  <label className="asm-runner__field">
                    <span>stdin</span>
                    <textarea
                      className="asm-runner__stdin"
                      onChange={(event) => setStdin(event.target.value)}
                      placeholder="read 系サンプル用の入力"
                      spellCheck={false}
                      value={stdin}
                    />
                  </label>
                </div>
              ) : null}

              <div className="asm-runner__card">
                <div className="asm-runner__card-header">
                  <div>
                    <h4>Code</h4>
                    <p>青緑は実行済み、濃いバッジは次に RIP が指す命令です。</p>
                  </div>
                </div>

                <div className="asm-runner__code">
                  {sourceLines.map((line, index) => {
                    const lineNumber = index + 1;
                    const isExecuted = executedLine === lineNumber;
                    const isRip = ripLine === lineNumber;
                    const className = isRip
                      ? "asm-runner__code-line rip"
                      : isExecuted
                        ? "asm-runner__code-line executed"
                        : "asm-runner__code-line";

                    return (
                      <div key={`${lineNumber}:${line}`} className={className}>
                        <span className="asm-runner__line-no">{lineNumber}</span>
                        <span
                          className="asm-runner__line-text hljs language-asm"
                          dangerouslySetInnerHTML={{ __html: highlightedLines[index] }}
                        />
                        <span className="asm-runner__line-badges">
                          {isExecuted ? <span className="asm-runner__executed-badge">Executed</span> : null}
                          {isRip ? <span className="asm-runner__rip-badge">RIP</span> : null}
                        </span>
                      </div>
                    );
                  })}
                </div>
              </div>
            </div>

            <div className="asm-runner__sidebar">
              <div className="asm-runner__card">
                <div className="asm-runner__card-header">
                  <div>
                    <h4>State</h4>
                    <p>
                      {currentEvent
                        ? "この step で変化したレジスタは強調表示されます。"
                        : "この ASM で関わるレジスタだけを表示します。"}
                    </p>
                  </div>
                </div>

                <div className="asm-runner__status asm-runner__status--stack">
                  <span>{timeline.length === 0 ? "未実行" : `${cursor} / ${timeline.length} step`}</span>
                  {hasMultipleBranches && activeBranch ? <span>branch: <code>{activeBranch.label}</code></span> : null}
                  {currentEvent?.instruction ? (
                    <span>
                      executed <code>{currentEvent.instruction.rip_before}</code>
                    </span>
                  ) : null}
                  {nextEvent?.instruction ? (
                    <span>
                      next RIP <code>{nextEvent.instruction.rip_before}</code>
                    </span>
                  ) : timeline.length > 0 ? (
                    <span>end of program</span>
                  ) : null}
                </div>

                {flags.length > 0 ? (
                  <div className="asm-runner__flags">
                    {flags.map((entry) => (
                      <div
                        key={`${entry.key}:${entry.value}`}
                        className={entry.value === "1" ? "asm-runner__flag is-on" : "asm-runner__flag"}
                      >
                        <span>{entry.key}</span>
                        <code>{entry.value}</code>
                      </div>
                    ))}
                  </div>
                ) : null}

                {meta.length > 0 ? (
                  <div className="asm-runner__registers asm-runner__registers--meta">
                    {meta.map((entry) => (
                      <div key={`${entry.key}:${entry.value}`} className="asm-runner__register asm-runner__register--meta">
                        <span>{entry.key}</span>
                        <code>{entry.value}</code>
                      </div>
                    ))}
                  </div>
                ) : null}

                {registers.length > 0 ? (
                  <div className="asm-runner__registers">
                    {registers.map((entry) => (
                      <div
                        key={`${entry.key}:${entry.value}`}
                        className={changedKeys.has(entry.key) ? "asm-runner__register changed" : "asm-runner__register"}
                      >
                        <span>{entry.key}</span>
                        <code>{entry.value}</code>
                      </div>
                    ))}
                  </div>
                ) : (
                  <p className="asm-runner__empty">このステージで追跡するレジスタはありません。</p>
                )}

                {changed.length > 0 ? (
                  <p className="asm-runner__empty">
                    changed: {changed.map((entry) => `${entry.key}=${entry.value}`).join(", ")}
                  </p>
                ) : null}

                {currentStack.length > 0 || removedStackEntries.length > 0 ? (
                  <>
                    <h4 style={{ margin: "12px 0 8px" }}>Stack</h4>
                    {currentStack.length === 0 && removedStackEntries.length > 0 ? (
                      <p className="asm-runner__empty">Stack is empty</p>
                    ) : null}
                    <div className="asm-runner__registers">
                      {currentStack.map((entry) => (
                        <div
                          key={`${entry.key}:${entry.value}`}
                          className={changedStackKeys.has(entry.key) ? "asm-runner__register changed" : "asm-runner__register"}
                        >
                          <span>{entry.key}</span>
                          <code>{entry.value}</code>
                        </div>
                      ))}
                      {removedStackEntries.map((entry) => (
                        <div
                          key={`removed:${entry.key}:${entry.value}`}
                          className="asm-runner__register removed"
                        >
                          <span>{entry.key}</span>
                          <code>{entry.value}</code>
                        </div>
                      ))}
                    </div>
                  </>
                ) : null}
              </div>

              {hasMultipleBranches ? (
                <div className="asm-runner__branches asm-runner__branches--stack">
                  {trace?.branches?.map((branch) => (
                    <button
                      key={branch.id}
                      className={branch.id === activeBranch?.id ? "asm-runner__branch active" : "asm-runner__branch"}
                      onClick={() => handleBranchChange(branch.id)}
                      type="button"
                    >
                      <strong>{branch.label}</strong>
                      <span>{branch.summary}</span>
                    </button>
                  ))}
                </div>
              ) : null}

              {sideEffects.length > 0 ? (
                <div className="asm-runner__card">
                  <div className="asm-runner__card-header">
                    <div>
                      <h4>Side Effects</h4>
                      <p>出力とメモリ変化だけを表示します。</p>
                    </div>
                  </div>

                  <div className="asm-runner__side-effects">
                    {sideEffects.map((entry) => (
                      <div key={entry.key}>
                        <strong>{entry.title}</strong>
                        {entry.kind === "pre" ? (
                          <pre>{entry.content}</pre>
                        ) : entry.key === "memory" ? (
                          <div className="asm-runner__registers">
                            {entry.content.map((item) => {
                              const effect = parseMemoryEffect(item);
                              if (!effect) {
                                return <div key={item} className="asm-runner__register"><code>{item}</code></div>;
                              }
                              const isChanged = effect.direction === "write" && changedMemoryKeys.size > 0 &&
                                [...changedMemoryKeys].some((addr) => item.includes(addr));
                              return (
                                <div key={item} className={isChanged ? "asm-runner__mem-effect changed" : "asm-runner__mem-effect"}>
                                  <span className={effect.direction === "write" ? "asm-runner__mem-badge asm-runner__mem-badge--write" : "asm-runner__mem-badge asm-runner__mem-badge--read"}>
                                    {effect.op}{effect.size ? ` ${effect.size}` : ""}
                                  </span>
                                  {effect.addr ? <code>{effect.addr}</code> : null}
                                  {effect.value ? <code>{effect.value}</code> : null}
                                </div>
                              );
                            })}
                          </div>
                        ) : (
                          <ul>
                            {entry.content.map((item) => (
                              <li key={item}>{item}</li>
                            ))}
                          </ul>
                        )}
                      </div>
                    ))}
                  </div>
                </div>
              ) : null}

              {error ? <div className="asm-runner__error">{error}</div> : null}
            </div>
          </div>
      </div>
    </section>
  );
}
