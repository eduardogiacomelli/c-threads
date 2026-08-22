import { rng } from "./utils";

export interface LaneOp {
  run: () => void;
  /** Op cannot be picked until this is true (spawned, joined, …). */
  ready?: () => boolean;
}

export interface Lane {
  ops: LaneOp[];
}

/**
 * Round-robin-free scheduler: at every tick it picks uniformly among the
 * ops that are runnable right now. Each lane still executes its own ops in
 * order - that constraint is what makes the output a *plausible* run rather
 * than noise, and it is the same rule a real scheduler obeys.
 *
 * The seed makes a run reproducible, so "shuffle" is a new schedule and not
 * a new program.
 */
export function runSchedule(lanes: Lane[], seed: number, maxTicks = 400) {
  const next = rng(seed);
  const cursor = lanes.map(() => 0);

  for (let tick = 0; tick < maxTicks; tick++) {
    const runnable: number[] = [];
    lanes.forEach((lane, i) => {
      const op = lane.ops[cursor[i]];
      if (!op) return;
      if (op.ready && !op.ready()) return;
      runnable.push(i);
    });
    if (runnable.length === 0) return;

    const pick = runnable[Math.min(runnable.length - 1, Math.floor(next() * runnable.length))];
    lanes[pick].ops[cursor[pick]].run();
    cursor[pick]++;
  }
}
