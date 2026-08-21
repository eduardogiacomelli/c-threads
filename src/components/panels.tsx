"use client";

import { AnimatePresence, motion } from "motion/react";
import { AlertTriangle, CheckCircle2, Info, Terminal, XCircle } from "lucide-react";
import type { Step, ThreadInfo } from "@/lib/types";
import { cn } from "@/lib/utils";
import { threadColor } from "./memory-pane";

const TONE = {
  info: { icon: Info, cls: "border-[var(--accent)] bg-[var(--accent-soft)]", fg: "var(--accent)" },
  ok: { icon: CheckCircle2, cls: "border-[var(--ok)] bg-[var(--ok-soft)]", fg: "var(--ok)" },
  warn: { icon: AlertTriangle, cls: "border-[var(--warn)] bg-[var(--warn-soft)]", fg: "var(--warn)" },
  error: { icon: XCircle, cls: "border-[var(--danger)] bg-[var(--danger-soft)]", fg: "var(--danger)" },
} as const;

export function Narration({ step }: { step: Step }) {
  const tone = TONE[step.tone];
  const Icon = tone.icon;
  return (
    <div
      className={cn(
        "flex min-h-[4.5rem] items-start gap-2.5 rounded-lg border-l-2 border-y border-r px-3 py-2.5",
        "border-y-[var(--line)] border-r-[var(--line)]",
        tone.cls,
      )}
    >
      <Icon size={15} style={{ color: tone.fg }} className="mt-0.5 shrink-0" />
      <AnimatePresence mode="wait">
        <motion.p
          key={step.note}
          initial={{ opacity: 0, y: 3 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.16 }}
          className="text-[13px] leading-relaxed text-[var(--text)]"
        >
          {step.note}
        </motion.p>
      </AnimatePresence>
    </div>
  );
}

const STATE_LABEL: Record<ThreadInfo["state"], string> = {
  "not-started": "not started",
  running: "running",
  ready: "ready",
  blocked: "blocked",
  done: "done",
};

export function ThreadRail({ step }: { step: Step }) {
  return (
    <div className="flex flex-wrap gap-1.5">
      {step.threads.map((t) => {
        const active = t.id === step.thread;
        const color = threadColor(t.id);
        return (
          <div
            key={t.id}
            className={cn(
              "flex items-center gap-2 rounded border px-2 py-1",
              active
                ? "border-[var(--line)] bg-[var(--panel)]"
                : "border-[var(--line-soft)] bg-transparent opacity-70",
              t.state === "blocked" &&
                "border-[var(--warn)] bg-[var(--warn-soft)] opacity-100",
            )}
            style={active ? { borderLeftColor: color, borderLeftWidth: 3 } : undefined}
          >
            {/* a blocked thread pulses: it is the one state where nothing
                changes on screen for several steps, so it needs to look like
                waiting rather than like a bug */}
            <span
              className={cn(
                "h-1.5 w-1.5 rounded-full",
                t.state === "done" && "opacity-30",
                t.state === "blocked" && "animate-ping-slow",
              )}
              style={{ background: color }}
            />
            <span className="font-mono text-[11px] text-[var(--text)]">{t.name}</span>
            <span
              className={cn(
                "font-mono text-[10px]",
                t.state === "blocked"
                  ? "text-[var(--warn)]"
                  : "text-[var(--faint)]",
              )}
            >
              {STATE_LABEL[t.state]}
              {t.line != null && t.state !== "done" ? ` · line ${t.line}` : ""}
              {t.detail ? ` · ${t.detail}` : ""}
            </span>
          </div>
        );
      })}
    </div>
  );
}

export function ConsoleOut({ step }: { step: Step }) {
  return (
    <div className="flex h-full min-h-[5rem] flex-col overflow-hidden rounded-lg border border-[var(--line)] bg-[var(--sunken)]">
      <div className="flex items-center gap-1.5 border-b border-[var(--line)] px-2.5 py-1.5 text-[10px] font-semibold uppercase tracking-[0.12em] text-[var(--faint)]">
        <Terminal size={11} />
        stdout
      </div>
      <div className="flex-1 overflow-auto p-2.5 font-mono text-[12px] leading-relaxed">
        {step.output.length === 0 ? (
          <span className="text-[var(--faint)]">nothing printed yet</span>
        ) : (
          step.output.map((line, i) => (
            <div key={i} className="text-[var(--text)]">
              {line}
            </div>
          ))
        )}
      </div>
    </div>
  );
}
