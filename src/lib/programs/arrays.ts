import { Machine, lineFinder } from "../machine";
import type { Program } from "../types";

const src = (mode: string) => `#include <stdio.h>

#define N 5

int main(void)
{
    int before = 111;
    int v[N] = {0, 0, 0, 0, 0};
    int after = 999;

    for (int i = 0; i ${mode === "overflow" ? "<=" : "<"} N; i++)
        v[i] = 7;

    printf("before=%d after=%d\\n", before, after);
    return 0;
}
`;

export const arrays: Program = {
  id: "arrays",
  title: "One step past the end",
  blurb: "An array is a run of boxes and nothing guards the far end of it.",
  origin: "c-do-zero / passo-08 e 09",
  concepts: ["array layout", "off-by-one", "stack-buffer-overflow"],
  takeaway:
    "An array never carries its own length, so a function taking one takes a size too. Derive the bound with `sizeof(v)/sizeof(v[0])` in the scope that declared it — never a literal in two places.",
  modes: [
    { id: "overflow", label: "i <= N", danger: true, hint: "the off-by-one" },
    { id: "fixed", label: "i < N", hint: "the fix" },
  ],
  source: src,
  build(mode) {
    const SRC = src(mode);
    const at = lineFinder(SRC);
    const m = new Machine();
    const limit = mode === "overflow" ? 5 : 4;

    m.pushFrame("main");
    const before = m.declare({ name: "before", type: "int", value: "111" });
    m.snap(at("int before"), "A neighbour, declared first. Remember it: nothing in the loop mentions it.");

    const cells = m.declareArray("v", "int", ["0", "0", "0", "0", "0"]);
    m.snap(
      at("int v[N]"),
      "Five boxes, contiguous, 4 bytes apart. `v` does not store its own length — the 5 lives only in your source.",
    );

    const after = m.declare({ name: "after", type: "int", value: "999" });
    m.snap(at("int after"), "Another neighbour, right after the array. The compiler decides this layout, not you.");

    const i = m.declare({ name: "i", type: "int", value: "0" });
    m.snap(at("for ("), "The loop counter is a box too.");

    for (let k = 0; k <= limit; k++) {
      m.write(i, String(k));
      m.read(i);

      if (k < 5) {
        m.write(cells[k], "7");
        m.snap(
          at("v[i] = 7"),
          `v[${k}] = 7. The address is computed, not searched: start + ${k} x 4 bytes. That is why it is fast, and why nobody checks it.`,
        );
      } else {
        /* the write lands on whatever the compiler put after the array */
        m.write(after, "7", { tone: "error", tag: "clobbered by v[5]" });
        m.snap(
          at("v[i] = 7"),
          "v[5] does not exist. The address start + 5 x 4 is computed anyway and the write lands on the next thing in the frame. AddressSanitizer: stack-buffer-overflow, WRITE of size 4, 0 bytes after the 20-byte region.",
          { tone: "error" },
        );
      }
    }

    m.read(before);
    m.read(after);
    m.print(
      mode === "overflow" ? "before=111 after=7" : "before=111 after=999",
    );
    m.snap(
      at("printf("),
      mode === "overflow"
        ? "`after` is 7 and no line of this program ever named it. This is why the bug is expensive: it does not fail where it is written."
        : "With `i < N` the loop stops at 4 and both neighbours are untouched. Same code, one character different.",
      { tone: mode === "overflow" ? "error" : "ok" },
    );

    return m.done();
  },
};
