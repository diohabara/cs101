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

// Phase 1: C とシステムプログラミング基礎 (v9-v17)
import v9Book from "../../../v9/book.md?raw";
import v9PtrDeref from "../../../v9/src/ptr_deref.c?raw";
import v9VolatileIo from "../../../v9/src/volatile_io.c?raw";
import v9Bitops from "../../../v9/src/bitops.c?raw";
import v10Book from "../../../v10/book.md?raw";
import v10BumpAlloc from "../../../v10/src/bump_alloc.c?raw";
import v10FreelistAlloc from "../../../v10/src/freelist_alloc.c?raw";
import v10AllocH from "../../../v10/src/alloc.h?raw";
import v11Book from "../../../v11/book.md?raw";
import v11Pcb from "../../../v11/src/pcb.c?raw";
import v11ForkExec from "../../../v11/src/fork_exec.c?raw";
import v11ProcessH from "../../../v11/src/process.h?raw";
import v12Book from "../../../v12/book.md?raw";
import v12SchedRr from "../../../v12/src/sched_rr.c?raw";
import v12SchedPrio from "../../../v12/src/sched_prio.c?raw";
import v12SchedH from "../../../v12/src/sched.h?raw";
import v13Book from "../../../v13/book.md?raw";
import v13SchedEdf from "../../../v13/src/sched_edf.c?raw";
import v13PrioInversion from "../../../v13/src/prio_inversion.c?raw";
import v13RtSchedH from "../../../v13/src/rt_sched.h?raw";
import v14Book from "../../../v14/book.md?raw";
import v14Spinlock from "../../../v14/src/spinlock.c?raw";
import v14MutexCond from "../../../v14/src/mutex_cond.c?raw";
import v14SyncH from "../../../v14/src/sync.h?raw";
import v15Book from "../../../v15/book.md?raw";
import v15MmioSim from "../../../v15/src/mmio_sim.c?raw";
import v15MmioLed from "../../../v15/src/mmio_led.c?raw";
import v15MmioH from "../../../v15/src/mmio.h?raw";
import v16Book from "../../../v16/book.md?raw";
import v16Ringbuf from "../../../v16/src/ringbuf.c?raw";
import v16DmaSim from "../../../v16/src/dma_sim.c?raw";
import v16DmaH from "../../../v16/src/dma.h?raw";
import v17Book from "../../../v17/book.md?raw";
import v17Minifs from "../../../v17/src/minifs.c?raw";
import v17Minishell from "../../../v17/src/minishell.c?raw";
import v17FsH from "../../../v17/src/fs.h?raw";

// Phase 2: ベアメタルカーネル (v18-v26)
import v18Book from "../../../v18/book.md?raw";
import v18Boot from "../../../v18/src/boot.c?raw";
import v18Uart from "../../../v18/src/uart.c?raw";
import v18UartH from "../../../v18/src/uart.h?raw";
import v18Linker from "../../../v18/src/linker.ld?raw";
import v19Book from "../../../v19/book.md?raw";
import v19Gdt from "../../../v19/src/gdt.c?raw";
import v19GdtH from "../../../v19/src/gdt.h?raw";
import v20Book from "../../../v20/book.md?raw";
import v20Idt from "../../../v20/src/idt.c?raw";
import v20IdtH from "../../../v20/src/idt.h?raw";
import v21Book from "../../../v21/book.md?raw";
import v21Timer from "../../../v21/src/timer.c?raw";
import v21TimerH from "../../../v21/src/timer.h?raw";
import v22Book from "../../../v22/book.md?raw";
import v22Pmm from "../../../v22/src/pmm.c?raw";
import v22PmmH from "../../../v22/src/pmm.h?raw";
import v23Book from "../../../v23/book.md?raw";
import v23Paging from "../../../v23/src/paging.c?raw";
import v23PagingH from "../../../v23/src/paging.h?raw";
import v24Book from "../../../v24/book.md?raw";
import v24Usermode from "../../../v24/src/usermode.c?raw";
import v24UsermodeH from "../../../v24/src/usermode.h?raw";
import v24SyscallHandler from "../../../v24/src/syscall_handler.c?raw";
import v25Book from "../../../v25/book.md?raw";
import v25DmaHw from "../../../v25/src/dma_hw.c?raw";
import v25DmaHwH from "../../../v25/src/dma_hw.h?raw";
import v26Book from "../../../v26/book.md?raw";
import v26Kernel from "../../../v26/src/kernel.c?raw";
import v26KernelH from "../../../v26/src/kernel.h?raw";
import v26Task from "../../../v26/src/task.c?raw";

// Phase 3: ハイパーバイザー (v27-v31)
import v27Book from "../../../v27/book.md?raw";
import v27VmxDetect from "../../../v27/src/vmx_detect.c?raw";
import v27VmxH from "../../../v27/src/vmx.h?raw";
import v28Book from "../../../v28/book.md?raw";
import v28Vmcs from "../../../v28/src/vmcs.c?raw";
import v28VmcsH from "../../../v28/src/vmcs.h?raw";
import v29Book from "../../../v29/book.md?raw";
import v29Guest from "../../../v29/src/guest.c?raw";
import v29GuestH from "../../../v29/src/guest.h?raw";
import v30Book from "../../../v30/book.md?raw";
import v30VmexitHandler from "../../../v30/src/vmexit_handler.c?raw";
import v30VmexitH from "../../../v30/src/vmexit.h?raw";
import v31Book from "../../../v31/book.md?raw";
import v31Ept from "../../../v31/src/ept.c?raw";
import v31EptH from "../../../v31/src/ept.h?raw";
import v31Hypervisor from "../../../v31/src/hypervisor.c?raw";

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
  c: "c",
  h: "c",
  ld: "c",
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
  // Phase 1: C とシステムプログラミング基礎
  {
    id: "v9",
    title: "\u{1F6A7} asm to C bridge",
    topic: "os",
    book: v9Book,
    files: {
      "src/ptr_deref.c": v9PtrDeref,
      "src/volatile_io.c": v9VolatileIo,
      "src/bitops.c": v9Bitops,
    },
  },
  {
    id: "v10",
    title: "\u{1F6A7} memory allocator",
    topic: "os",
    book: v10Book,
    files: {
      "src/bump_alloc.c": v10BumpAlloc,
      "src/freelist_alloc.c": v10FreelistAlloc,
      "src/alloc.h": v10AllocH,
    },
  },
  {
    id: "v11",
    title: "\u{1F6A7} process abstraction",
    topic: "os",
    book: v11Book,
    files: {
      "src/pcb.c": v11Pcb,
      "src/fork_exec.c": v11ForkExec,
      "src/process.h": v11ProcessH,
    },
  },
  {
    id: "v12",
    title: "\u{1F6A7} scheduler",
    topic: "os",
    book: v12Book,
    files: {
      "src/sched_rr.c": v12SchedRr,
      "src/sched_prio.c": v12SchedPrio,
      "src/sched.h": v12SchedH,
    },
  },
  {
    id: "v13",
    title: "\u{1F6A7} realtime scheduling",
    topic: "os",
    book: v13Book,
    files: {
      "src/sched_edf.c": v13SchedEdf,
      "src/prio_inversion.c": v13PrioInversion,
      "src/rt_sched.h": v13RtSchedH,
    },
  },
  {
    id: "v14",
    title: "\u{1F6A7} threads and sync",
    topic: "os",
    book: v14Book,
    files: {
      "src/spinlock.c": v14Spinlock,
      "src/mutex_cond.c": v14MutexCond,
      "src/sync.h": v14SyncH,
    },
  },
  {
    id: "v15",
    title: "\u{1F6A7} MMIO and devices",
    topic: "os",
    book: v15Book,
    files: {
      "src/mmio_sim.c": v15MmioSim,
      "src/mmio_led.c": v15MmioLed,
      "src/mmio.h": v15MmioH,
    },
  },
  {
    id: "v16",
    title: "\u{1F6A7} DMA and ring buffer",
    topic: "os",
    book: v16Book,
    files: {
      "src/ringbuf.c": v16Ringbuf,
      "src/dma_sim.c": v16DmaSim,
      "src/dma.h": v16DmaH,
    },
  },
  {
    id: "v17",
    title: "\u{1F6A7} filesystem and shell",
    topic: "os",
    book: v17Book,
    files: {
      "src/minifs.c": v17Minifs,
      "src/minishell.c": v17Minishell,
      "src/fs.h": v17FsH,
    },
  },
  // Phase 2: ベアメタルカーネル
  {
    id: "v18",
    title: "\u{1F6A7} boot and serial I/O",
    topic: "os",
    book: v18Book,
    files: {
      "src/boot.c": v18Boot,
      "src/uart.c": v18Uart,
      "src/uart.h": v18UartH,
      "src/linker.ld": v18Linker,
    },
  },
  {
    id: "v19",
    title: "\u{1F6A7} GDT and segmentation",
    topic: "os",
    book: v19Book,
    files: {
      "src/gdt.c": v19Gdt,
      "src/gdt.h": v19GdtH,
    },
  },
  {
    id: "v20",
    title: "\u{1F6A7} IDT and interrupts",
    topic: "os",
    book: v20Book,
    files: {
      "src/idt.c": v20Idt,
      "src/idt.h": v20IdtH,
    },
  },
  {
    id: "v21",
    title: "\u{1F6A7} timer and preemption",
    topic: "os",
    book: v21Book,
    files: {
      "src/timer.c": v21Timer,
      "src/timer.h": v21TimerH,
    },
  },
  {
    id: "v22",
    title: "\u{1F6A7} physical memory allocator",
    topic: "os",
    book: v22Book,
    files: {
      "src/pmm.c": v22Pmm,
      "src/pmm.h": v22PmmH,
    },
  },
  {
    id: "v23",
    title: "\u{1F6A7} page tables",
    topic: "os",
    book: v23Book,
    files: {
      "src/paging.c": v23Paging,
      "src/paging.h": v23PagingH,
    },
  },
  {
    id: "v24",
    title: "\u{1F6A7} usermode and syscall",
    topic: "os",
    book: v24Book,
    files: {
      "src/usermode.c": v24Usermode,
      "src/usermode.h": v24UsermodeH,
      "src/syscall_handler.c": v24SyscallHandler,
    },
  },
  {
    id: "v25",
    title: "\u{1F6A7} DMA controller",
    topic: "os",
    book: v25Book,
    files: {
      "src/dma_hw.c": v25DmaHw,
      "src/dma_hw.h": v25DmaHwH,
    },
  },
  {
    id: "v26",
    title: "\u{1F6A7} mini OS integration",
    topic: "os",
    book: v26Book,
    files: {
      "src/kernel.c": v26Kernel,
      "src/kernel.h": v26KernelH,
      "src/task.c": v26Task,
    },
  },
  // Phase 3: ハイパーバイザー
  {
    id: "v27",
    title: "\u{1F6A7} VT-x and VMX",
    topic: "os",
    book: v27Book,
    files: {
      "src/vmx_detect.c": v27VmxDetect,
      "src/vmx.h": v27VmxH,
    },
  },
  {
    id: "v28",
    title: "\u{1F6A7} VMCS configuration",
    topic: "os",
    book: v28Book,
    files: {
      "src/vmcs.c": v28Vmcs,
      "src/vmcs.h": v28VmcsH,
    },
  },
  {
    id: "v29",
    title: "\u{1F6A7} guest execution",
    topic: "os",
    book: v29Book,
    files: {
      "src/guest.c": v29Guest,
      "src/guest.h": v29GuestH,
    },
  },
  {
    id: "v30",
    title: "\u{1F6A7} VM exit handling",
    topic: "os",
    book: v30Book,
    files: {
      "src/vmexit_handler.c": v30VmexitHandler,
      "src/vmexit.h": v30VmexitH,
    },
  },
  {
    id: "v31",
    title: "\u{1F6A7} security isolation (EPT)",
    topic: "os",
    book: v31Book,
    files: {
      "src/ept.c": v31Ept,
      "src/ept.h": v31EptH,
      "src/hypervisor.c": v31Hypervisor,
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
