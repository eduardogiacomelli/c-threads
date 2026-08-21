import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

/** Deterministic PRNG so a schedule seed always replays the same run. */
export function rng(seed: number) {
  let s = seed >>> 0 || 1;
  return () => {
    s ^= s << 13;
    s ^= s >>> 17;
    s ^= s << 5;
    s >>>= 0;
    return s / 0xffffffff;
  };
}

/**
 * Interleaves per-thread op lists into one order, preserving each thread's
 * internal sequence. That constraint is the whole model of a race: threads
 * run in order internally, in any order relative to each other.
 */
export function interleave<T>(lanes: T[][], seed: number): { lane: number; op: T }[] {
  const next = rng(seed);
  const cursors = lanes.map(() => 0);
  const out: { lane: number; op: T }[] = [];
  for (;;) {
    const open = cursors
      .map((c, i) => (c < lanes[i].length ? i : -1))
      .filter((i) => i >= 0);
    if (open.length === 0) break;
    const lane = open[Math.floor(next() * open.length) % open.length];
    out.push({ lane, op: lanes[lane][cursors[lane]] });
    cursors[lane]++;
  }
  return out;
}
