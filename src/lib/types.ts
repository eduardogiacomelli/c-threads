/**
 * The machine model.
 *
 * There is no C interpreter here, and that is on purpose: every program ships
 * a builder that replays a known-correct execution as a list of immutable
 * snapshots. Adding a program means writing a builder, not extending a VM.
 */

export type Tone = "info" | "warn" | "error" | "ok";

export type SlotKind = "scalar" | "pointer" | "cell" | "field";

/** One named box in memory. The unit everything else is made of. */
export interface Slot {
  /** Stable across steps — arrows and highlights are keyed on this. */
  id: string;
  name: string;
  /** C type as written in the source, e.g. `int *`, `char[6]`. */
  type: string;
  addr: number;
  size: number;
  /** Rendered value. Pointers carry the target address here. */
  value: string;
  kind: SlotKind;
  /** Slot id this pointer refers to. Drives the arrow layer. */
  points?: string | null;
  /** Storage is gone (popped frame, freed block) but still drawn. */
  dead?: boolean;
  /** Short label under the box. */
  tag?: string;
  /** Alignment padding that follows this slot, in bytes. Only the byte view
   *  shows it, but it is what makes sizeof(struct) bigger than its fields. */
  padAfter?: number;
  tone?: Tone;
}

export interface Frame {
  id: string;
  fn: string;
  thread: number;
  depth: number;
  slots: Slot[];
  /** Popped, drawn as a ghost so frame reuse is visible. */
  dead?: boolean;
}

export interface HeapBlock {
  id: string;
  label: string;
  addr: number;
  size: number;
  slots: Slot[];
  freed?: boolean;
}

export type ThreadState =
  | "not-started"
  | "running"
  | "ready"
  | "blocked"
  | "done";

export interface ThreadInfo {
  id: number;
  name: string;
  state: ThreadState;
  line: number | null;
  /** Per-thread scratch shown next to the thread chip, e.g. a register. */
  detail?: string;
}

/** One frame of the animation: a complete picture of the machine. */
export interface Step {
  line: number;
  thread: number;
  threads: ThreadInfo[];
  globals: Slot[];
  frames: Frame[];
  heap: HeapBlock[];
  output: string[];
  note: string;
  tone: Tone;
  /** Slot ids touched this step. */
  reads: string[];
  writes: string[];
}

export interface ProgramMode {
  id: string;
  label: string;
  danger?: boolean;
  hint?: string;
}

export interface Program {
  id: string;
  title: string;
  /** One line, shown in the picker. */
  blurb: string;
  /** Which passo file this mirrors. */
  origin: string;
  concepts: string[];
  /** A mode can change the code itself, so the source depends on it. */
  source: (mode: string) => string;
  modes?: ProgramMode[];
  /** Threads can be interleaved differently; seed drives the schedule. */
  schedulable?: boolean;
  build: (mode: string, seed: number) => Step[];
}
