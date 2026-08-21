"use client";

import { useLayoutEffect, useState, type RefObject } from "react";
import type { Slot, Step } from "@/lib/types";

interface Wire {
  d: string;
  tone: "normal" | "dead";
  key: string;
}

function allSlots(step: Step): Slot[] {
  return [
    ...step.globals,
    ...step.frames.flatMap((f) => f.slots),
    ...step.heap.flatMap((h) => h.slots),
  ];
}

/**
 * Draws the pointer arrows over the memory pane.
 *
 * Positions come from the DOM after layout instead of from a model of the
 * layout — boxes move when frames are pushed and popped, and keeping a
 * parallel geometry model in sync with flexbox is a losing game.
 */
export function Arrows({
  containerRef,
  step,
}: {
  containerRef: RefObject<HTMLDivElement | null>;
  step: Step;
}) {
  const [wires, setWires] = useState<Wire[]>([]);

  useLayoutEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const measure = () => {
      const box = container.getBoundingClientRect();
      const slots = allSlots(step);
      const byId = new Map(slots.map((s) => [s.id, s]));
      const out: Wire[] = [];

      slots.forEach((slot) => {
        if (!slot.points) return;
        /* the wire is stale when what it points AT is gone, which is the
           whole story of a dangling pointer — the pointer itself is fine */
        const stale = slot.dead || byId.get(slot.points)?.dead;
        const from = container.querySelector<HTMLElement>(
          `[data-slot-id="${slot.id}"]`,
        );
        const to = container.querySelector<HTMLElement>(
          `[data-slot-id="${slot.points}"]`,
        );
        if (!from || !to) return;

        const a = from.getBoundingClientRect();
        const b = to.getBoundingClientRect();

        /* leave and enter on facing sides so the wire never crosses the
           address label above a box or the tag below it */
        const rightward = a.left + a.width / 2 < b.left + b.width / 2;
        const x1 = (rightward ? a.right : a.left) - box.left;
        const y1 = a.top + a.height / 2 - box.top;
        const x2 = (rightward ? b.left : b.right) - box.left;
        const y2 = b.top + b.height / 2 - box.top;
        const bow = Math.max(22, Math.abs(x2 - x1) * 0.4) * (rightward ? 1 : -1);

        out.push({
          key: `${slot.id}->${slot.points}`,
          tone: stale ? "dead" : "normal",
          d: `M ${x1} ${y1} C ${x1 + bow} ${y1}, ${x2 - bow} ${y2}, ${x2} ${y2}`,
        });
      });

      setWires(out);
    };

    measure();
    /* re-measure after motion settles and on any resize of the pane */
    const t = setTimeout(measure, 300);
    const ro = new ResizeObserver(measure);
    ro.observe(container);
    return () => {
      clearTimeout(t);
      ro.disconnect();
    };
  }, [containerRef, step]);

  return (
    <svg
      className="pointer-events-none absolute inset-0 h-full w-full overflow-visible"
      aria-hidden
    >
      <defs>
        <marker
          id="arrowhead"
          viewBox="0 0 8 8"
          refX="7"
          refY="4"
          markerWidth="6"
          markerHeight="6"
          orient="auto-start-reverse"
        >
          <path d="M0 0 L8 4 L0 8 z" fill="var(--accent)" />
        </marker>
        <marker
          id="arrowhead-dead"
          viewBox="0 0 8 8"
          refX="7"
          refY="4"
          markerWidth="6"
          markerHeight="6"
          orient="auto-start-reverse"
        >
          <path d="M0 0 L8 4 L0 8 z" fill="var(--danger)" />
        </marker>
      </defs>
      {wires.map((w) => (
        <path
          key={w.key}
          d={w.d}
          fill="none"
          strokeWidth={1.5}
          stroke={w.tone === "dead" ? "var(--danger)" : "var(--accent)"}
          strokeDasharray={w.tone === "dead" ? "5 4" : undefined}
          markerEnd={`url(#${w.tone === "dead" ? "arrowhead-dead" : "arrowhead"})`}
        />
      ))}
    </svg>
  );
}
