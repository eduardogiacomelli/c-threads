"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import { TOKEN_CLASS, tokenizeSource, type Token } from "@/lib/csyntax";
import { hex } from "@/lib/machine";
import type { Slot, Step } from "@/lib/types";
import { cn } from "@/lib/utils";
import { threadColor } from "./memory-pane";

/**
 * Resolves an identifier in the source to a live box.
 *
 * Innermost frame of the executing thread first, then that thread's other
 * frames, then statics - which is roughly C's own lookup order, and means
 * hovering `reg` while thread b is running shows thread b's register.
 */
function resolve(step: Step, name: string): Slot | undefined {
  const mine = step.frames.filter((f) => f.thread === step.thread && !f.dead);
  for (let i = mine.length - 1; i >= 0; i--) {
    const hit = mine[i].slots.find((s) => s.name === name);
    if (hit) return hit;
  }
  const anyFrame = step.frames.flatMap((f) => f.slots).find((s) => s.name === name);
  if (anyFrame) return anyFrame;
  const global = step.globals.find((s) => s.name === name);
  if (global) return global;
  return step.heap.flatMap((h) => h.slots).find((s) => s.name === name);
}

function Tip({ slot }: { slot: Slot }) {
  return (
    <span
      role="tooltip"
      className="pointer-events-none absolute bottom-full left-0 z-20 mb-1 w-max max-w-xs
                 rounded border border-[var(--line)] bg-[var(--panel)] px-2 py-1.5
                 font-mono text-[11px] leading-relaxed shadow-lg"
    >
      <span className="text-[var(--muted)]">{slot.type} </span>
      <span className="text-[var(--text)]">{slot.name}</span>
      <br />
      <span className="text-[var(--faint)]">value </span>
      <span className="text-[var(--accent)]">{slot.value}</span>
      <br />
      <span className="text-[var(--faint)]">addr&nbsp; </span>
      <span className="tabular-nums">{hex(slot.addr)}</span>
      <br />
      <span className="text-[var(--faint)]">size&nbsp; {slot.size} bytes</span>
      {slot.dead && (
        <>
          <br />
          <span className="text-[var(--danger)]">storage is gone</span>
        </>
      )}
    </span>
  );
}

function CodeToken({ token, step }: { token: Token; step: Step }) {
  const [open, setOpen] = useState(false);
  const slot = token.kind === "ident" ? resolve(step, token.text) : undefined;

  if (!slot) {
    return <span className={TOKEN_CLASS[token.kind]}>{token.text}</span>;
  }

  const live = step.reads.includes(slot.id) || step.writes.includes(slot.id);

  return (
    <span
      className={cn(
        "relative cursor-help rounded-sm underline decoration-dotted decoration-1 underline-offset-2",
        TOKEN_CLASS[token.kind],
        live && "bg-[var(--accent-soft)]",
      )}
      onMouseEnter={() => setOpen(true)}
      onMouseLeave={() => setOpen(false)}
      onFocus={() => setOpen(true)}
      onBlur={() => setOpen(false)}
      tabIndex={0}
    >
      {token.text}
      {open && <Tip slot={slot} />}
    </span>
  );
}

export function CodePane({
  source,
  step,
  stepLines,
  onJump,
}: {
  source: string;
  step: Step;
  /** step index -> line, so clicking a line can jump the timeline */
  stepLines: number[];
  onJump: (index: number) => void;
}) {
  const lines = useMemo(() => tokenizeSource(source), [source]);
  const activeRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    activeRef.current?.scrollIntoView({ block: "center", behavior: "smooth" });
  }, [step.line]);

  /* every line another thread is currently sitting on, for the gutter marks */
  const parked = new Map<number, number[]>();
  step.threads.forEach((t) => {
    if (t.line == null || t.state === "done" || t.id === step.thread) return;
    parked.set(t.line, [...(parked.get(t.line) ?? []), t.id]);
  });

  return (
    <div className="h-full overflow-auto rounded-lg border border-[var(--line)] bg-[var(--panel)] py-2 font-mono text-[13px] leading-6">
      {lines.map((tokens, i) => {
        const lineNo = i + 1;
        const active = lineNo === step.line;
        const target = stepLines.indexOf(lineNo);
        const others = parked.get(lineNo);

        return (
          <div
            key={lineNo}
            ref={active ? activeRef : undefined}
            onClick={() => target >= 0 && onJump(target)}
            className={cn(
              "group relative flex gap-3 px-3",
              target >= 0 && "cursor-pointer hover:bg-[var(--sunken)]",
              active && "bg-[var(--accent-soft)]",
            )}
          >
            {active && (
              <span
                className="absolute left-0 top-0 h-full w-[3px]"
                style={{ background: threadColor(step.thread) }}
              />
            )}

            <span className="w-6 shrink-0 select-none text-right text-[11px] tabular-nums text-[var(--faint)]">
              {lineNo}
            </span>

            <span className="flex w-4 shrink-0 items-center gap-0.5">
              {others?.map((id) => (
                <span
                  key={id}
                  title={`${step.threads[id]?.name} is here`}
                  className="h-1.5 w-1.5 rounded-full"
                  style={{ background: threadColor(id) }}
                />
              ))}
            </span>

            <code className="whitespace-pre">
              {tokens.map((t, k) => (
                <CodeToken key={k} token={t} step={step} />
              ))}
            </code>
          </div>
        );
      })}
    </div>
  );
}
