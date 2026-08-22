import type { Program } from "../types";
import { pointers } from "./pointers";
import { dangling } from "./dangling";
import { arrays } from "./arrays";
import { strings } from "./strings";
import { voidSwap } from "./void-swap";
import { reinterpret } from "./reinterpret";
import { heap } from "./heap";
import { threadArg } from "./thread-arg";
import { recursion } from "./recursion";
import { race } from "./race";
import { atomics } from "./atomics";
import { workSplit } from "./work-split";

/** Order matters: this is the teaching sequence. */
export const PROGRAMS: Program[] = [
  pointers,
  arrays,
  strings,
  voidSwap,
  reinterpret,
  dangling,
  recursion,
  heap,
  threadArg,
  race,
  atomics,
  workSplit,
];

export const byId = (id: string) =>
  PROGRAMS.find((p) => p.id === id) ?? PROGRAMS[0];
