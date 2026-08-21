import type { Program } from "../types";
import { pointers } from "./pointers";
import { dangling } from "./dangling";
import { arrays } from "./arrays";
import { heap } from "./heap";
import { threadArg } from "./thread-arg";
import { race } from "./race";

/** Order matters: this is the teaching sequence. */
export const PROGRAMS: Program[] = [
  pointers,
  dangling,
  arrays,
  heap,
  threadArg,
  race,
];

export const byId = (id: string) =>
  PROGRAMS.find((p) => p.id === id) ?? PROGRAMS[0];
