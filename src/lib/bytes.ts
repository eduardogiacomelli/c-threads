import type { Slot } from "./types";

export type ByteKind = "value" | "pad" | "unknown";

export interface ByteCell {
  hex: string;
  kind: ByteKind;
  /** Absolute address of this byte, for the tooltip. */
  addr: number;
}

/**
 * Derives the bytes of a slot from its value rather than storing them.
 *
 * Deriving means the bytes can never disagree with the number printed next to
 * them, which is the one thing that would make this view worse than useless.
 * Little-endian, because that is what x86-64 does and it is the detail that
 * surprises everybody the first time they look at memory.
 */
export function slotBytes(slot: Slot): ByteCell[] {
  const out: ByteCell[] = [];
  const raw = slot.value.trim();

  let n: bigint | null = null;
  if (raw === "NULL") n = 0n;
  else if (/^0x[0-9a-fA-F]+$/.test(raw)) n = BigInt(raw);
  else if (/^-?\d+$/.test(raw)) n = BigInt(raw);
  else n = charLiteral(raw);

  for (let i = 0; i < slot.size; i++) {
    if (n === null) {
      out.push({ hex: "??", kind: "unknown", addr: slot.addr + i });
    } else {
      /* two's complement for negatives, then peel one byte at a time */
      const unsigned = BigInt.asUintN(slot.size * 8, n);
      const byte = Number((unsigned >> BigInt(8 * i)) & 0xffn);
      out.push({
        hex: byte.toString(16).padStart(2, "0"),
        kind: "value",
        addr: slot.addr + i,
      });
    }
  }

  for (let i = 0; i < (slot.padAfter ?? 0); i++) {
    out.push({ hex: "··", kind: "pad", addr: slot.addr + slot.size + i });
  }

  return out;
}

const ESCAPES: Record<string, number> = {
  "0": 0,
  n: 10,
  t: 9,
  r: 13,
  "\\": 92,
  "'": 39,
};

/**
 * `'A'` -> 65, `'\0'` -> 0. A char really is just a small integer, and the
 * terminator really is just the number zero - which is the entire reason a C
 * string can be walked off the end of.
 */
export function charLiteral(raw: string): bigint | null {
  const m = /^'(\\.|[^'\\])'$/.exec(raw);
  if (!m) return null;
  const body = m[1];
  if (body.startsWith("\\")) {
    const v = ESCAPES[body[1]];
    return v === undefined ? null : BigInt(v);
  }
  return BigInt(body.charCodeAt(0));
}

/** Alignment of a scalar: its own size, capped at 8 on x86-64. */
export function alignOf(size: number) {
  return Math.min(size, 8);
}

export function roundUp(value: number, align: number) {
  return Math.ceil(value / align) * align;
}
