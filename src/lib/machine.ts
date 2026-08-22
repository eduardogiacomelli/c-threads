import { alignOf, roundUp } from "./bytes";
import type {
  Frame,
  HeapBlock,
  Slot,
  Step,
  ThreadInfo,
  Tone,
} from "./types";

/** Stack grows down from here; each thread gets its own 8 KiB region. */
const STACK_TOP = 0x7ffc_dd40;
const THREAD_STRIDE = 0x2000;
const FRAME_STRIDE = 0x40;
const HEAP_BASE = 0x55a1_c2f0;

/**
 * Frame addresses come from (thread, depth), never from a running counter.
 * That is what makes frame reuse show up as the *same* address after a
 * return - the whole point of the dangling-pointer program.
 */
function frameAddr(thread: number, depth: number) {
  return STACK_TOP - thread * THREAD_STRIDE - depth * FRAME_STRIDE;
}

export function hex(addr: number) {
  return "0x" + addr.toString(16).padStart(8, "0");
}

let uid = 0;
const nextId = (p: string) => `${p}-${++uid}`;

interface SlotSpec {
  name: string;
  type: string;
  value: string;
  size?: number;
  kind?: Slot["kind"];
  points?: string | null;
  tag?: string;
  tone?: Tone;
}

/**
 * Mutable draft of the machine. Builders mutate it and call `snap()` to
 * freeze a step. Every snapshot is a deep copy, so steps never alias.
 */
export class Machine {
  globals: Slot[] = [];
  frames: Frame[] = [];
  heap: HeapBlock[] = [];
  output: string[] = [];
  threads: ThreadInfo[] = [
    { id: 0, name: "main", state: "running", line: null },
  ];

  private steps: Step[] = [];
  private heapCursor = HEAP_BASE;
  private pendingReads: string[] = [];
  private pendingWrites: string[] = [];

  /* ---------------------------------------------------------- threads */

  addThread(name: string): ThreadInfo {
    const t: ThreadInfo = {
      id: this.threads.length,
      name,
      state: "not-started",
      line: null,
    };
    this.threads.push(t);
    return t;
  }

  thread(id: number) {
    const t = this.threads.find((x) => x.id === id);
    if (!t) throw new Error(`no thread ${id}`);
    return t;
  }

  setThread(id: number, patch: Partial<ThreadInfo>) {
    Object.assign(this.thread(id), patch);
  }

  /* ----------------------------------------------------------- frames */

  pushFrame(fn: string, thread = 0): Frame {
    const depth = this.frames.filter((f) => f.thread === thread && !f.dead)
      .length;
    const f: Frame = {
      id: nextId("frame"),
      fn,
      thread,
      depth,
      slots: [],
    };
    /* A ghost frame at this exact slot is replaced: the new call reuses
       that memory, which is what we want the reader to notice. */
    const ghostAt = this.frames.findIndex(
      (g) => g.dead && g.thread === thread && g.depth === depth,
    );
    if (ghostAt >= 0) this.frames.splice(ghostAt, 1, f);
    else this.frames.push(f);
    return f;
  }

  /** `ghost` keeps the frame on screen, dimmed, with its bytes intact. */
  popFrame(thread = 0, ghost = false) {
    const live = this.frames.filter((f) => f.thread === thread && !f.dead);
    const f = live[live.length - 1];
    if (!f) return;
    if (ghost) {
      f.dead = true;
      f.slots.forEach((s) => {
        s.dead = true;
        s.tag = "frame released";
      });
    } else {
      this.frames = this.frames.filter((x) => x.id !== f.id);
    }
  }

  private frameOf(thread: number) {
    const live = this.frames.filter((f) => f.thread === thread && !f.dead);
    const f = live[live.length - 1];
    if (!f) throw new Error(`thread ${thread} has no live frame`);
    return f;
  }

  /* ------------------------------------------------------------ slots */

  declare(spec: SlotSpec, thread = 0): Slot {
    const f = this.frameOf(thread);
    const size = spec.size ?? 4;
    const packed = f.slots.reduce((n, x) => n + x.size + (x.padAfter ?? 0), 0);
    const used = roundUp(packed, alignOf(size));
    if (used > packed && f.slots.length > 0) {
      const prev = f.slots[f.slots.length - 1];
      prev.padAfter = (prev.padAfter ?? 0) + (used - packed);
    }
    const s: Slot = {
      id: nextId("slot"),
      kind: spec.kind ?? "scalar",
      size: spec.size ?? 4,
      addr: frameAddr(thread, f.depth) + used,
      points: spec.points ?? null,
      name: spec.name,
      type: spec.type,
      value: spec.value,
      tag: spec.tag,
      tone: spec.tone,
    };
    f.slots.push(s);
    this.pendingWrites.push(s.id);
    return s;
  }

  global(spec: SlotSpec): Slot {
    const used = roundUp(
      this.globals.reduce((n, s) => n + s.size, 0),
      alignOf(spec.size ?? 4),
    );
    const s: Slot = {
      id: nextId("glob"),
      kind: spec.kind ?? "scalar",
      size: spec.size ?? 4,
      addr: 0x5591_4020 + used,
      points: spec.points ?? null,
      name: spec.name,
      type: spec.type,
      value: spec.value,
      tag: spec.tag,
      tone: spec.tone,
    };
    this.globals.push(s);
    return s;
  }

  /** Contiguous cells of one array, addressed like the real thing. */
  declareArray(
    name: string,
    type: string,
    values: string[],
    opts: { elemSize?: number; thread?: number } = {},
  ): Slot[] {
    const elem = opts.elemSize ?? 4;
    const thread = opts.thread ?? 0;
    const f = this.frameOf(thread);
    const base =
      frameAddr(thread, f.depth) +
      roundUp(
        f.slots.reduce((n, s) => n + s.size + (s.padAfter ?? 0), 0),
        alignOf(elem),
      );
    return values.map((v, i) => {
      const s: Slot = {
        id: nextId("slot"),
        name: `${name}[${i}]`,
        type,
        value: v,
        kind: "cell",
        size: elem,
        addr: base + i * elem,
        points: null,
      };
      f.slots.push(s);
      return s;
    });
  }

  /**
   * Lays the struct out the way the ABI does: every field starts at a
   * multiple of its own alignment, and the total is rounded up to the widest
   * member. That is why sizeof(struct) is usually more than the sum of the
   * fields, and the byte view is where you can actually see it.
   */
  malloc(label: string, specs: SlotSpec[]): HeapBlock {
    const block: HeapBlock = {
      id: nextId("heap"),
      label,
      addr: this.heapCursor,
      size: 0,
      slots: [],
    };

    let off = 0;
    let widest = 1;
    for (const spec of specs) {
      const size = spec.size ?? 4;
      const align = alignOf(size);
      widest = Math.max(widest, align);
      const at = roundUp(off, align);
      if (at > off && block.slots.length > 0) {
        const prev = block.slots[block.slots.length - 1];
        prev.padAfter = (prev.padAfter ?? 0) + (at - off);
      }
      block.slots.push({
        id: nextId("slot"),
        kind: spec.kind ?? "field",
        size,
        addr: block.addr + at,
        points: spec.points ?? null,
        name: spec.name,
        type: spec.type,
        value: spec.value,
        tag: spec.tag,
      });
      off = at + size;
    }

    block.size = roundUp(off, widest);
    if (block.size > off && block.slots.length > 0) {
      const last = block.slots[block.slots.length - 1];
      last.padAfter = (last.padAfter ?? 0) + (block.size - off);
    }

    this.heapCursor += roundUp(block.size + 16, 16);
    this.heap.push(block);
    this.pendingWrites.push(...block.slots.map((s) => s.id));
    return block;
  }

  free(block: HeapBlock) {
    block.freed = true;
    block.slots.forEach((s) => {
      s.dead = true;
      s.tag = "liberado";
    });
  }

  /* ------------------------------------------------------------ edits */

  write(slot: Slot, value: string, extra: Partial<Slot> = {}) {
    slot.value = value;
    Object.assign(slot, extra);
    this.pendingWrites.push(slot.id);
    return slot;
  }

  /** Records a read so the step can highlight it, and returns the value. */
  read(slot: Slot) {
    this.pendingReads.push(slot.id);
    return slot.value;
  }

  aim(ptr: Slot, target: Slot | null) {
    ptr.points = target ? target.id : null;
    ptr.value = target ? hex(target.addr) : "NULL";
    this.pendingWrites.push(ptr.id);
  }

  print(line: string) {
    this.output.push(line);
  }

  /* ------------------------------------------------------------- snap */

  snap(
    line: number,
    note: string,
    opts: { tone?: Tone; thread?: number } = {},
  ) {
    const thread = opts.thread ?? 0;
    this.setThread(thread, { line });
    this.steps.push({
      line,
      thread,
      note,
      tone: opts.tone ?? "info",
      threads: this.threads.map((t) => ({ ...t })),
      globals: this.globals.map(cloneSlot),
      frames: this.frames.map((f) => ({ ...f, slots: f.slots.map(cloneSlot) })),
      heap: this.heap.map((h) => ({ ...h, slots: h.slots.map(cloneSlot) })),
      output: [...this.output],
      reads: [...this.pendingReads],
      writes: [...this.pendingWrites],
    });
    this.pendingReads = [];
    this.pendingWrites = [];
  }

  done(): Step[] {
    return this.steps;
  }
}

const cloneSlot = (s: Slot): Slot => ({ ...s });

/**
 * Line lookup by content. Builders say `at(src, "*p = 30")` instead of
 * hard-coding 37, so editing the C source can't silently desync the trace.
 */
export function lineFinder(source: string) {
  const lines = source.split("\n");
  return (needle: string, from = 0): number => {
    const i = lines.findIndex((l, k) => k >= from && l.includes(needle));
    if (i < 0) throw new Error(`line not found: ${needle}`);
    return i + 1;
  };
}
