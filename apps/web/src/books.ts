import v1Book from "../../../v1/book.md?raw";
import v1Exit42 from "../../../v1/asm/exit42.asm?raw";
import v1Regs from "../../../v1/asm/regs.asm?raw";
import v1Swap from "../../../v1/asm/swap.asm?raw";
import v1Arithmetic from "../../../v1/asm/arithmetic.asm?raw";
import v1Flags from "../../../v1/asm/flags.asm?raw";
import v2Book from "../../../v2/book.md?raw";
import v2CmpEqual from "../../../v2/asm/cmp_equal.asm?raw";
import v2Countdown from "../../../v2/asm/countdown.asm?raw";
import v2Max from "../../../v2/asm/max.asm?raw";
import v3Book from "../../../v3/book.md?raw";
import v3StoreLoad from "../../../v3/asm/store_load.asm?raw";
import v3ArraySum from "../../../v3/asm/array_sum.asm?raw";
import v3MemoryModify from "../../../v3/asm/memory_modify.asm?raw";
import v4Book from "../../../v4/book.md?raw";
import v4PushPop from "../../../v4/asm/push_pop.asm?raw";
import v4CallRet from "../../../v4/asm/call_ret.asm?raw";
import v4NestedCalls from "../../../v4/asm/nested_calls.asm?raw";
import v5Book from "../../../v5/book.md?raw";
import v5Hello from "../../../v5/asm/hello.asm?raw";
import v5WriteStderr from "../../../v5/asm/write_stderr.asm?raw";
import v5MultiSyscall from "../../../v5/asm/multi_syscall.asm?raw";
import v6Book from "../../../v6/book.md?raw";
import v6ReadStdin from "../../../v6/asm/read_stdin.asm?raw";
import v6WriteFile from "../../../v6/asm/write_file.asm?raw";
import v6FdTable from "../../../v6/asm/fd_table.asm?raw";
import v7Book from "../../../v7/book.md?raw";
import v7Getpid from "../../../v7/asm/getpid.asm?raw";
import v7ForkBasic from "../../../v7/asm/fork_basic.asm?raw";
import v7ExecveEcho from "../../../v7/asm/execve_echo.asm?raw";
import v8Book from "../../../v8/book.md?raw";
import v8MmapWrite from "../../../v8/asm/mmap_write.asm?raw";
import v8MprotectCrash from "../../../v8/asm/mprotect_crash.asm?raw";
import v8TwoMappings from "../../../v8/asm/two_mappings.asm?raw";
import { stageQuizzes, type StageQuiz } from "./quiz";

export type BookStage = {
  id: string;
  title: string;
  topic: "cpu" | "os" | "compiler";
  markdown: string;
  quiz?: StageQuiz;
};

type StageSource = Omit<BookStage, "markdown"> & {
  book: string;
  files: Record<string, string>;
};

const codeExtensions: Record<string, string> = {
  asm: "asm",
  md: "markdown",
  rs: "rust",
  toml: "toml",
};

function languageFor(path: string) {
  const extension = path.split(".").pop() ?? "";
  return codeExtensions[extension] ?? "";
}

function expandBook(source: StageSource) {
  return source.book.replace(/\{\{(include|code):([^}]+)\}\}/g, (_, kind: string, rawPath: string) => {
    const path = rawPath.trim();
    const content = source.files[path];

    if (!content) {
      return `\n> missing include: ${path}\n`;
    }

    if (kind === "include") {
      return `\n${content.trim()}\n`;
    }

    return `\n#### \`${path}\`\n\n\`\`\`${languageFor(path)}\n${content.trimEnd()}\n\`\`\`\n`;
  });
}

const stageSources: StageSource[] = [
  {
    id: "v1",
    title: "CPU state model",
    topic: "cpu",
    book: v1Book,
    files: {
      "asm/exit42.asm": v1Exit42,
      "asm/regs.asm": v1Regs,
      "asm/swap.asm": v1Swap,
      "asm/arithmetic.asm": v1Arithmetic,
      "asm/flags.asm": v1Flags,
    },
  },
  {
    id: "v2",
    title: "cmp and branch",
    topic: "cpu",
    book: v2Book,
    files: {
      "asm/cmp_equal.asm": v2CmpEqual,
      "asm/countdown.asm": v2Countdown,
      "asm/max.asm": v2Max,
    },
  },
  {
    id: "v3",
    title: "memory and load/store",
    topic: "cpu",
    book: v3Book,
    files: {
      "asm/store_load.asm": v3StoreLoad,
      "asm/array_sum.asm": v3ArraySum,
      "asm/memory_modify.asm": v3MemoryModify,
    },
  },
  {
    id: "v4",
    title: "stack and call/ret",
    topic: "cpu",
    book: v4Book,
    files: {
      "asm/push_pop.asm": v4PushPop,
      "asm/call_ret.asm": v4CallRet,
      "asm/nested_calls.asm": v4NestedCalls,
    },
  },
  {
    id: "v5",
    title: "syscalls and output",
    topic: "cpu",
    book: v5Book,
    files: {
      "asm/hello.asm": v5Hello,
      "asm/write_stderr.asm": v5WriteStderr,
      "asm/multi_syscall.asm": v5MultiSyscall,
    },
  },
  {
    id: "v6",
    title: "fd and I/O",
    topic: "cpu",
    book: v6Book,
    files: {
      "asm/read_stdin.asm": v6ReadStdin,
      "asm/write_file.asm": v6WriteFile,
      "asm/fd_table.asm": v6FdTable,
    },
  },
  {
    id: "v7",
    title: "process model",
    topic: "cpu",
    book: v7Book,
    files: {
      "asm/getpid.asm": v7Getpid,
      "asm/fork_basic.asm": v7ForkBasic,
      "asm/execve_echo.asm": v7ExecveEcho,
    },
  },
  {
    id: "v8",
    title: "virtual memory",
    topic: "cpu",
    book: v8Book,
    files: {
      "asm/mmap_write.asm": v8MmapWrite,
      "asm/mprotect_crash.asm": v8MprotectCrash,
      "asm/two_mappings.asm": v8TwoMappings,
    },
  },
];

export const books: BookStage[] = stageSources.map((stage) => {
  const markdown = expandBook(stage);

  return {
    id: stage.id,
    title: stage.title,
    topic: stage.topic,
    markdown,
    quiz: stageQuizzes[stage.id],
  };
});
