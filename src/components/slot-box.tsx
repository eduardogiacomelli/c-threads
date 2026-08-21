"use client";

import { motion } from "motion/react";
import { ByteGrid } from "./byte-grid";
import { hex } from "@/lib/machine";
import type { Slot } from "@/lib/types";
import { cn } from "@/lib/utils";

const TONE_RING: Record<string, string> = {
  error: "border-[var(--danger)] bg-[var(--danger-soft)]",
  warn: "border-[var(--warn)] bg-[var(--warn-soft)]",
  ok: "border-[var(--ok)] bg-[var(--ok-soft)]",
};

export function SlotBox({
  slot,
  read,
  written,
  showAddress,
  showBytes,
  accent,
}: {
  slot: Slot;
  read?: boolean;
  written?: boolean;
  showAddress: boolean;
  showBytes?: boolean;
  /** Thread colour, when the slot belongs to a thread's frame. */
  accent?: string;
}) {
  const tone = slot.tone ? TONE_RING[slot.tone] : undefined;

  return (
    <div className="grid gap-0.5" data-slot-anchor={slot.id}>
      {showAddress && (
        <span className="font-mono text-[10px] leading-none tabular-nums text-[var(--faint)]">
          {hex(slot.addr)}
        </span>
      )}

      <motion.div
        data-slot-id={slot.id}
        layout
        animate={
          written
            ? { scale: [1, 1.06, 1] }
            : read
              ? { scale: [1, 1.02, 1] }
              : { scale: 1 }
        }
        transition={{ duration: 0.28 }}
        className={cn(
          "flex items-baseline justify-between gap-3 rounded border px-2 py-1",
          "bg-[var(--panel)] font-mono text-sm tabular-nums",
          slot.dead
            ? "border-dashed border-[var(--line)] text-[var(--faint)] opacity-70"
            : "border-[var(--line)]",
          written && !slot.dead && "border-[var(--accent)] bg-[var(--accent-soft)]",
          read && !written && !slot.dead && "border-[var(--accent)]",
          tone,
        )}
        style={accent && !slot.dead ? { borderLeftColor: accent, borderLeftWidth: 3 } : undefined}
      >
        <span className="flex items-baseline gap-1.5">
          <span className="text-[var(--muted)]">{slot.name}</span>
          <span className="text-[10px] text-[var(--faint)]">{slot.type}</span>
          {showBytes && (slot.padAfter ?? 0) > 0 && (
            <span className="text-[10px] text-[var(--faint)]">
              +{slot.padAfter} pad
            </span>
          )}
        </span>
        <span
          className={cn(
            "font-medium",
            slot.kind === "pointer" && "text-[11px] text-[var(--accent)]",
          )}
        >
          {slot.value}
        </span>
      </motion.div>

      {showBytes && <ByteGrid slot={slot} dim={slot.dead} />}

      {slot.tag && (
        <span
          className={cn(
            "font-mono text-[10px] leading-tight",
            slot.tone === "error" ? "text-[var(--danger)]" : "text-[var(--faint)]",
          )}
        >
          {slot.tag}
        </span>
      )}
    </div>
  );
}
