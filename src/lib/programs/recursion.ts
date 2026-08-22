import { Machine, hex, lineFinder } from "../machine";
import type { Program } from "../types";

const DEPTH = 4;

const SRC = `#include <stdio.h>

static long factorial(int n)
{
    if (n <= 1)
        return 1;                  /* base case: no more frames */

    long rest = factorial(n - 1);  /* a whole new frame, right here */
    return n * rest;
}

int main(void)
{
    printf("factorial(${DEPTH}) = %ld\\n", factorial(${DEPTH}));
    return 0;
}
`;

export const recursion: Program = {
  id: "recursion",
  title: "Recursion is frames, all the way down",
  blurb: "Every call gets its own n and its own rest, alive at the same time.",
  origin: "c-do-zero / passo-27",
  concepts: ["stack frame", "call depth", "one copy per call", "unwinding"],
  takeaway:
    "Each call has its own locals, and they all exist at once. Depth costs memory: stack size divided by frame size is your real recursion limit, and C will not warn you before it runs out.",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();

    m.pushFrame("main");
    m.snap(
      at("int main"),
      `main calls factorial(${DEPTH}). Watch the stack column: one card per live call, and none of them go away until the innermost one returns.`,
    );

    /* going down: one frame per call */
    for (let n = DEPTH; n >= 1; n--) {
      m.pushFrame("factorial", 0);
      const slot = m.declare({ name: "n", type: "int", value: String(n) });

      if (n > 1) {
        m.snap(
          at("long rest = factorial"),
          `factorial(${n}) pushes a frame. Its \`n\` lives at ${hex(slot.addr)}, and it is a different box from every other \`n\` on screen. The call to factorial(${n - 1}) has to finish before this frame can compute anything.`,
        );
      } else {
        m.read(slot);
        m.snap(
          at("return 1"),
          `factorial(1) hits the base case and returns 1 without recursing. ${DEPTH} frames are alive at this instant, each holding its own n. A base case is not decoration: without one, this column grows until the stack runs out.`,
          { tone: "ok" },
        );
      }
    }

    /* coming back up: each frame finishes its own multiplication */
    let value = 1;
    for (let n = 2; n <= DEPTH; n++) {
      m.popFrame(0, false); /* the callee's frame goes away */

      const frames = m.frames.filter((f) => f.thread === 0 && !f.dead);
      const here = frames[frames.length - 1];
      const nSlot = here.slots.find((x) => x.name === "n");

      const rest = m.declare({ name: "rest", type: "long", value: String(value), size: 8 });
      m.read(rest);
      if (nSlot) m.read(nSlot);
      value = value * n;

      m.snap(
        at("return n * rest"),
        `The callee returned ${rest.value}. This frame's own \`n\` is still ${n}, exactly as it left it, so it computes ${n} * ${rest.value} = ${value} and returns. Each frame kept its own copy the whole time.`,
      );
    }

    m.popFrame(0, false);
    m.print(`factorial(${DEPTH}) = ${value}`);
    m.snap(
      at("printf("),
      `Back in main with ${value}. The stack is a stack: ${DEPTH} pushes on the way in, ${DEPTH} pops on the way out, and the deepest point is what the memory actually cost.`,
      { tone: "ok" },
    );

    m.snap(
      at("return 0"),
      "Depth is the price. Roughly 8 MiB of stack divided by the frame size gives your real limit, so small frames recurse hundreds of thousands deep and a frame with a 16 KiB buffer manages about 500. Python raises RecursionError at 1000; C just runs out and dies, and every thread has its own stack to run out of.",
      { tone: "warn" },
    );

    return m.done();
  },
};
