import { Machine, lineFinder } from "../machine";
import type { Program } from "../types";

/** Exact IEEE 754 bits of a float, so the app and the C agree. */
function floatBits(value: number): { bytes: number[]; u32: number } {
  const view = new DataView(new ArrayBuffer(4));
  view.setFloat32(0, value, true);
  return {
    bytes: Array.from({ length: 4 }, (_, i) => view.getUint8(i)),
    u32: view.getUint32(0, true),
  };
}

const SRC = `#include <stdio.h>
#include <stdint.h>

union Word {
    int32_t  as_signed;
    uint32_t as_unsigned;
    float    as_float;
    uint8_t  as_bytes[4];
};

int main(void)
{
    union Word w;

    w.as_signed = -1;
    printf("signed   %d\\n",  w.as_signed);
    printf("unsigned %u\\n",  w.as_unsigned);

    w.as_float = 1.0f;
    printf("float    %f\\n",  w.as_float);
    printf("as int   %u\\n",  w.as_unsigned);

    return 0;
}
`;

export const reinterpret: Program = {
  id: "reinterpret",
  title: "One set of bytes, four readings",
  blurb: "A union proves the type is only an instruction for how to read memory.",
  origin: "c-do-zero / passo-23 e 25",
  concepts: ["union", "type punning", "signed vs unsigned", "endianness"],
  takeaway:
    "A type is not stored anywhere. It is an instruction to the compiler about how many bytes to read and how to interpret them, which is why the same four bytes can be -1 and 4294967295 at the same time.",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();

    m.pushFrame("main");
    m.snap(
      at("int main"),
      "Turn the byte view on for this one. Everything below is the same four bytes, looked at differently.",
    );

    /* Every member of a union starts at the same address. The machine model
       has no union type, so they are declared as four slots that are then
       given one shared address: which is exactly what a union is. */
    const s = m.declare({ name: "w.as_signed", type: "int32_t", value: "?", size: 4 });
    const u = m.declare({ name: "w.as_unsigned", type: "uint32_t", value: "?", size: 4 });
    const f = m.declare({ name: "w.as_float", type: "float", value: "?", size: 4 });
    const base = s.addr;
    [u, f].forEach((slot) => (slot.addr = base));

    m.snap(
      at("union Word"),
      `sizeof(union Word) is 4, not 16: the members are laid on top of each other, not side by side. All three show the same address, ${"0x" + base.toString(16)}, because there is only one set of bytes here.`,
    );

    /* -1 in two's complement is all ones */
    m.write(s, "-1");
    m.write(u, "4294967295");
    m.write(f, "-nan");
    f.tag = "these bits are not a valid float";
    m.snap(
      at("w.as_signed = -1"),
      "Writing -1 sets all 32 bits to 1. Look at the byte grid: ff ff ff ff, and every member shows it, because every member IS it.",
    );

    m.read(s);
    m.print("signed   -1");
    m.snap(
      at('printf("signed'),
      "Read as int32_t, the top bit means negative and two's complement makes the value -1.",
    );

    m.read(u);
    m.print("unsigned 4294967295");
    m.snap(
      at('printf("unsigned'),
      "Read as uint32_t, the same top bit is just another power of two, and the value is 4294967295. Nothing changed in memory between these two lines. This is exactly the conversion that makes `i < n` false when i is -1 and n is unsigned.",
      { tone: "warn" },
    );

    /* now a float, computed for real */
    const { bytes, u32 } = floatBits(1.0);
    m.write(f, "1.000000", { tag: undefined });
    m.write(u, String(u32));
    m.write(s, String(u32 | 0));
    m.snap(
      at("w.as_float = 1.0f"),
      `Writing 1.0f into the same four bytes. The grid now reads ${bytes
        .map((b) => b.toString(16).padStart(2, "0"))
        .join(" ")}, which is IEEE 754 for one: sign 0, exponent 127, mantissa 0.`,
    );

    m.read(u);
    m.print("float    1.000000");
    m.print(`as int   ${u32}`);
    m.snap(
      at('printf("as int'),
      `Read those same bits as an integer and you get ${u32}. Nothing was converted: no rounding, no cast, no arithmetic. 1.0f simply IS the bit pattern 0x${u32.toString(16)}, and asking for it as an integer just reads it differently.`,
      { tone: "ok" },
    );

    m.snap(
      at("return 0"),
      "The honest way to do this deliberately is memcpy, which is always correct and costs nothing. Reading a union member you did not write is also fine in C. The one to avoid is *(uint32_t *)&f, which breaks strict aliasing and usually only misbehaves once you turn on -O2.",
    );

    return m.done();
  },
};
