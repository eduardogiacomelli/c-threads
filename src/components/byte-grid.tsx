"use client";

import { slotBytes } from "@/lib/bytes";
import { hex } from "@/lib/machine";
import type { Slot } from "@/lib/types";
import { cn } from "@/lib/utils";

/**
 * The raw bytes behind one slot, lowest address first.
 *
 * Reading left to right you are walking up in memory, which is why a 25 looks
 * like `19 00 00 00` and not `00 00 00 19`. Padding cells are the bytes the
 * ABI inserts for alignment: real, addressable, and holding nothing.
 */
export function ByteGrid({ slot, dim }: { slot: Slot; dim?: boolean }) {
  const cells = slotBytes(slot);

  return (
    <div className="flex flex-wrap gap-[2px]" aria-label={`bytes of ${slot.name}`}>
      {cells.map((cell, i) => (
        <span
          key={i}
          title={`${hex(cell.addr)}${cell.kind === "pad" ? " · padding" : ""}`}
          className={cn(
            "min-w-[1.55rem] rounded-[2px] border px-[3px] py-[1px] text-center",
            "font-mono text-[10px] leading-tight tabular-nums",
            cell.kind === "pad" &&
              "border-dashed border-[var(--line)] text-[var(--faint)]",
            cell.kind === "unknown" &&
              "border-[var(--line)] bg-[var(--sunken)] text-[var(--faint)]",
            cell.kind === "value" &&
              (cell.hex === "00"
                ? "border-[var(--line-soft)] bg-[var(--sunken)] text-[var(--faint)]"
                : "border-[var(--line)] bg-[var(--panel)] text-[var(--text)]"),
            dim && "opacity-60",
          )}
        >
          {cell.hex}
        </span>
      ))}
    </div>
  );
}
