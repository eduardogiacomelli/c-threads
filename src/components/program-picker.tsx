"use client";

import { PROGRAMS } from "@/lib/programs";
import { useMachine } from "@/store/machine-store";
import { cn } from "@/lib/utils";

export function ProgramPicker() {
  const { programId, select } = useMachine();

  return (
    <nav className="grid gap-1" aria-label="Programs">
      {PROGRAMS.map((p, i) => {
        const active = p.id === programId;
        return (
          <button
            key={p.id}
            type="button"
            onClick={() => select(p.id)}
            aria-current={active}
            className={cn(
              "group grid gap-0.5 rounded border px-2.5 py-2 text-left transition",
              active
                ? "border-[var(--accent)] bg-[var(--accent-soft)]"
                : "border-transparent hover:border-[var(--line)] hover:bg-[var(--panel)]",
            )}
          >
            <span className="flex items-baseline gap-2">
              <span className="font-mono text-[10px] tabular-nums text-[var(--faint)]">
                {String(i + 1).padStart(2, "0")}
              </span>
              <span
                className={cn(
                  "text-[13px] font-medium",
                  active ? "text-[var(--accent)]" : "text-[var(--text)]",
                )}
              >
                {p.title}
              </span>
            </span>
            <span className="pl-6 text-[11px] leading-snug text-[var(--muted)]">
              {p.blurb}
            </span>
            <span className="pl-6 font-mono text-[10px] text-[var(--faint)]">
              {p.origin}
            </span>
          </button>
        );
      })}
    </nav>
  );
}
