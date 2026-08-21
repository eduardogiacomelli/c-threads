"use client";

import { create } from "zustand";
import { PROGRAMS, byId } from "@/lib/programs";
import type { Step } from "@/lib/types";

interface MachineStore {
  programId: string;
  mode: string;
  seed: number;
  index: number;
  playing: boolean;
  speed: number;
  showAddresses: boolean;
  showBytes: boolean;

  steps: Step[];
  source: string;

  select: (id: string) => void;
  setMode: (mode: string) => void;
  shuffle: () => void;
  go: (index: number) => void;
  next: () => void;
  prev: () => void;
  reset: () => void;
  togglePlay: () => void;
  setSpeed: (s: number) => void;
  toggleAddresses: () => void;
  toggleBytes: () => void;
}

function rebuild(programId: string, mode: string, seed: number) {
  const program = byId(programId);
  return {
    steps: program.build(mode, seed),
    source: program.source(mode),
  };
}

const FIRST = PROGRAMS[0];
const initialMode = FIRST.modes?.[0]?.id ?? "default";

export const useMachine = create<MachineStore>((set, get) => ({
  programId: FIRST.id,
  mode: initialMode,
  seed: 7,
  index: 0,
  playing: false,
  speed: 900,
  showAddresses: true,
  showBytes: false,
  ...rebuild(FIRST.id, initialMode, 7),

  select: (id) => {
    const program = byId(id);
    const mode = program.modes?.[0]?.id ?? "default";
    const { seed } = get();
    set({ programId: id, mode, index: 0, playing: false, ...rebuild(id, mode, seed) });
  },

  setMode: (mode) => {
    const { programId, seed } = get();
    set({ mode, index: 0, playing: false, ...rebuild(programId, mode, seed) });
  },

  shuffle: () => {
    const { programId, mode } = get();
    const seed = 1 + Math.floor(Math.random() * 99999);
    set({ seed, index: 0, playing: false, ...rebuild(programId, mode, seed) });
  },

  go: (index) => {
    const { steps } = get();
    set({ index: Math.max(0, Math.min(steps.length - 1, index)) });
  },

  next: () => {
    const { index, steps } = get();
    if (index >= steps.length - 1) set({ playing: false });
    else set({ index: index + 1 });
  },

  prev: () => set((s) => ({ index: Math.max(0, s.index - 1), playing: false })),
  reset: () => set({ index: 0, playing: false }),
  togglePlay: () =>
    set((s) => ({
      playing: !s.playing,
      index: !s.playing && s.index >= s.steps.length - 1 ? 0 : s.index,
    })),
  setSpeed: (speed) => set({ speed }),
  toggleAddresses: () => set((s) => ({ showAddresses: !s.showAddresses })),
  toggleBytes: () => set((s) => ({ showBytes: !s.showBytes })),
}));
