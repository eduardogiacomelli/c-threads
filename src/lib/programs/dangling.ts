import { Machine, hex, lineFinder } from "../machine";
import type { Program } from "../types";

const SRC = `#include <stdio.h>

int *saved;                    /* a global pointer */

void make_number(void)
{
    int number = 42;
    saved = &number;           /* the bug: address of a dying box */
}

void other_function(void)
{
    int other = 777;
    printf("other = %d\\n", other);
}

int main(void)
{
    make_number();
    printf("*saved = %d\\n", *saved);

    other_function();
    printf("*saved = %d\\n", *saved);

    return 0;
}
`;

export const dangling: Program = {
  id: "dangling",
  title: "The frame dies, the address stays",
  blurb: "Why a pointer to a local survives as a number and stops being valid as a box.",
  origin: "c-do-zero / passo-14",
  concepts: ["stack frame", "lifetime", "dangling pointer", "stack-use-after-return"],
  takeaway:
    "Never let an address outlive its box. If a result must survive the return it belongs on the heap or in storage the caller owns - and say which in a comment, because the compiler will not.",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();

    const saved = m.global({
      name: "saved",
      type: "int *",
      value: "NULL",
      size: 8,
      kind: "pointer",
    });
    m.pushFrame("main");
    m.snap(at("int main"), "`saved` is global: one box for the whole program, and it outlives every function.");

    m.snap(at("make_number();"), "main calls make_number.");

    m.pushFrame("make_number");
    const number = m.declare({ name: "number", type: "int", value: "42" });
    m.snap(
      at("int number"),
      `A new frame was pushed and \`number\` was born at ${hex(number.addr)}. This frame exists only while the call does.`,
    );

    m.aim(saved, number);
    m.snap(
      at("saved = &number"),
      "The address is copied into the global. Perfectly legal right now - and gcc already warns: storing the address of local variable 'number' in 'saved'.",
      { tone: "warn" },
    );

    m.popFrame(0, true);
    m.snap(
      at("}", at("saved = &number")),
      `make_number returned. The frame is abandoned - but nothing was erased: ${hex(number.addr)} still contains 42, and \`saved\` still holds that number.`,
      { tone: "warn" },
    );

    m.read(saved);
    m.print("*saved = 42");
    m.snap(
      at('printf("*saved'),
      "Reads 42. THIS IS THE DANGEROUS PART: the bug looks like working code. Nothing has overwritten the bytes yet.",
      { tone: "warn" },
    );

    m.snap(at("other_function();"), "main calls a completely unrelated function.");

    m.pushFrame("other_function");
    const other = m.declare({ name: "other", type: "int", value: "777" });
    m.snap(
      at("int other"),
      `Look at the address: ${hex(other.addr)}. The new frame got the same slot the old one gave up, and \`other\` is sitting exactly where \`number\` was.`,
      { tone: "error" },
    );

    m.read(other);
    m.print("other = 777");
    m.snap(at('printf("other'), "other_function does its own thing, unaware any of this is happening.");

    m.popFrame(0, false);
    m.snap(at("other_function();"), "It returns.");

    m.read(saved);
    m.print("*saved = 777");
    m.snap(
      at('printf("*saved', at("other_function();")),
      "*saved now reads 777. AddressSanitizer reports stack-use-after-return and names the frame the address belonged to. Without sanitizers the program prints the wrong number and exits 0.",
      { tone: "error" },
    );

    return m.done();
  },
};
