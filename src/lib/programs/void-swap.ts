import { Machine, lineFinder } from "../machine";
import type { Program, Slot } from "../types";

/**
 * The two doubles are corrupted for real, not described.
 *
 * A DataView gives the exact IEEE 754 bytes, the low four are exchanged, and
 * the result is read back as a double. The numbers the byte view shows are
 * therefore the same ones the C program prints, down to the last digit.
 */
function ieeeBytes(value: number): number[] {
  const view = new DataView(new ArrayBuffer(8));
  view.setFloat64(0, value, true); /* little-endian, like x86-64 */
  return Array.from({ length: 8 }, (_, i) => view.getUint8(i));
}

function fromBytes(bytes: number[]): number {
  const view = new DataView(new ArrayBuffer(8));
  bytes.forEach((b, i) => view.setUint8(i, b));
  return view.getFloat64(0, true);
}

/** Rendered the way the C program prints it, %.12f. */
const fmt = (v: number) => v.toFixed(12);

const A = 1.5;
const B = 3.14159;

const src = (mode: string) => `#include <stdio.h>
#include <string.h>

void swap(void *a, void *b${mode === "sized" ? ", size_t size" : ""})
{
${
  mode === "sized"
    ? `    /* unsigned char is the raw-byte type: exactly 1 byte, no sign,
     * and legal to alias any object with. */
    unsigned char *pa = (unsigned char *) a;
    unsigned char *pb = (unsigned char *) b;

    for (size_t i = 0; i < size; i++) {
        unsigned char t = pa[i];
        pa[i] = pb[i];
        pb[i] = t;
    }`
    : `    /* THE BUG: void * carries no size, so this guesses "int". */
    int *pa = (int *) a;
    int *pb = (int *) b;

    int t = *pa;
    *pa = *pb;
    *pb = t;`
}
}

int main(void)
{
    double p = ${A};
    double q = ${B};

    swap(&p, &q${mode === "sized" ? ", sizeof p" : ""});

    printf("p=%.12f q=%.12f\\n", p, q);
    return 0;
}
`;

export const voidSwap: Program = {
  id: "void-swap",
  title: "A generic swap that guesses the size",
  blurb: "void * forgets the type, and the type is where the size was.",
  origin: "c-do-zero / passo-21 e 22",
  concepts: ["void *", "size_t", "unsigned char", "silent corruption"],
  takeaway:
    "A void * is an address with the type erased, and erasing the type erases the size. Any generic function that moves data has to be told how many bytes, which is why every one in the standard library takes a size.",
  modes: [
    { id: "guess", label: "cast to int *", danger: true, hint: "moves 4 bytes" },
    { id: "sized", label: "+ size_t size", hint: "moves all of them" },
  ],
  source: src,
  build(mode) {
    const SRC = src(mode);
    const at = lineFinder(SRC);
    const m = new Machine();
    const sized = mode === "sized";
    const moved = sized ? 8 : 4;

    m.pushFrame("main");

    /* Each double is eight one-byte cells, so the swap can be watched byte
       by byte. The value shown on each cell is the byte itself; the running
       interpretation is in the narration. */
    const aBytes = ieeeBytes(A);
    const bBytes = ieeeBytes(B);

    const p = m.declareArray(
      "p",
      "double",
      aBytes.map((b) => String(b)),
      { elemSize: 1 },
    );
    m.snap(
      at("double p"),
      `Turn the byte view on. \`p\` is ${A}, and these are its eight IEEE 754 bytes, lowest address first. The low four are the least significant bits of the mantissa; the high four carry the exponent and the sign.`,
    );

    const q = m.declareArray(
      "q",
      "double",
      bBytes.map((b) => String(b)),
      { elemSize: 1 },
    );
    m.snap(at("double q"), `\`q\` is ${B}. Same eight bytes, different contents.`);

    m.snap(
      at("swap(&p"),
      sized
        ? "swap is told the size, so it can move every byte. Watch all eight."
        : "swap receives two addresses and nothing else. Inside, it decides the type is int, and an int is four bytes. It will move four.",
      { tone: sized ? "ok" : "warn" },
    );

    /* the swap, one byte at a time */
    const live: Slot[] = [];
    for (let i = 0; i < moved; i++) {
      const pv = p[i].value;
      const qv = q[i].value;
      m.read(p[i]);
      m.read(q[i]);
      m.write(p[i], qv);
      m.write(q[i], pv);
      live.push(p[i], q[i]);
      m.snap(
        at(sized ? "pa[i] = pb[i]" : "*pa = *pb"),
        `byte ${i}: ${pv} and ${qv} exchanged.${
          !sized && i === moved - 1
            ? " That was the fourth and last byte an int covers. Bytes 4 to 7 of both doubles are never touched."
            : ""
        }`,
        { tone: !sized && i === moved - 1 ? "warn" : "info" },
      );
    }

    /* what the eight bytes now mean, computed rather than asserted */
    const finalA = aBytes.slice();
    const finalB = bBytes.slice();
    for (let i = 0; i < moved; i++) {
      const t = finalA[i];
      finalA[i] = finalB[i];
      finalB[i] = t;
    }
    const pv = fromBytes(finalA);
    const qv = fromBytes(finalB);

    if (!sized) {
      for (let i = 0; i < 4; i++) {
        p[i].tone = "error";
        q[i].tone = "error";
      }
      p[0].tag = "half of q's mantissa";
      q[0].tag = "half of p's mantissa";
    }

    m.print(`p=${fmt(pv)} q=${fmt(qv)}`);
    m.snap(
      at("printf("),
      sized
        ? `All eight bytes moved, so p is ${fmt(pv)} and q is ${fmt(qv)}: a real swap. The same function works for an int, a char or a whole struct, because it never pretends to know what the bytes mean.`
        : `Read the result: p is ${fmt(pv)} and q is ${fmt(qv)}. Neither swapped nor unchanged, just slightly wrong, which is the worst possible failure: it looks like a rounding error and gets blamed on floating point instead of on the swap.`,
      { tone: sized ? "ok" : "error" },
    );

    return m.done();
  },
};
