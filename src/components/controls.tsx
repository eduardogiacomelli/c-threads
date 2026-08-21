"use client";

import { useEffect } from "react";
import {
  ChevronLeft,
  ChevronRight,
  Dices,
  Hash,
  Moon,
  Pause,
  Play,
  RotateCcw,
  Sun,
} from "lucide-react";
import { useMachine } from "@/store/machine-store";
import { byId } from "@/lib/programs";
import { cn } from "@/lib/utils";

function Btn({
  onClick,
  children,
  title,
  active,
  danger,
  disabled,
}: {
  onClick: () => void;
  children: React.ReactNode;
  title: string;
  active?: boolean;
  danger?: boolean;
  disabled?: boolean;
}) {
  return (
    <button
      type="button"
      title={title}
      aria-label={title}
      aria-pressed={active}
      disabled={disabled}
      onClick={onClick}
      className={cn(
        "inline-flex items-center gap-1.5 rounded border px-2.5 py-1.5 text-xs font-medium transition",
        "border-[var(--line)] bg-[var(--panel)] text-[var(--text)]",
        "hover:border-[var(--accent)] hover:text-[var(--accent)]",
        "disabled:cursor-not-allowed disabled:opacity-40 disabled:hover:border-[var(--line)] disabled:hover:text-[var(--text)]",
        active &&
          (danger
            ? "border-[var(--danger)] bg-[var(--danger-soft)] text-[var(--danger)] hover:text-[var(--danger)]"
            : "border-[var(--accent)] bg-[var(--accent-soft)] text-[var(--accent)]"),
      )}
    >
      {children}
    </button>
  );
}

function toggleTheme() {
  const dark = document.documentElement.classList.toggle("dark");
  try {
    localStorage.setItem("cm-theme", dark ? "dark" : "light");
  } catch {
    /* private mode: the toggle still works for this session */
  }
}

export function Controls() {
  const s = useMachine();
  const program = byId(s.programId);

  /* autoplay */
  useEffect(() => {
    if (!s.playing) return;
    const id = setInterval(s.next, s.speed);
    return () => clearInterval(id);
  }, [s.playing, s.speed, s.next]);

  /* keyboard transport */
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const el = document.activeElement;
      if (el instanceof HTMLInputElement || el instanceof HTMLTextAreaElement) return;
      if (e.key === "ArrowRight") { e.preventDefault(); s.next(); }
      else if (e.key === "ArrowLeft") { e.preventDefault(); s.prev(); }
      else if (e.key === " ") { e.preventDefault(); s.togglePlay(); }
      else if (e.key === "r") s.reset();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [s]);

  const last = s.steps.length - 1;

  return (
    <div className="flex flex-wrap items-center gap-2 rounded-lg border border-[var(--line)] bg-[var(--sunken)] px-3 py-2">
      <Btn onClick={s.reset} title="Restart (r)">
        <RotateCcw size={13} />
      </Btn>
      <Btn onClick={s.prev} title="Previous step (left arrow)" disabled={s.index === 0}>
        <ChevronLeft size={13} />
      </Btn>
      <Btn onClick={s.togglePlay} title="Play / pause (space)" active={s.playing}>
        {s.playing ? <Pause size={13} /> : <Play size={13} />}
        {s.playing ? "pause" : "play"}
      </Btn>
      <Btn onClick={s.next} title="Next step (right arrow)" disabled={s.index >= last}>
        <ChevronRight size={13} />
      </Btn>

      <input
        type="range"
        min={0}
        max={Math.max(last, 0)}
        value={s.index}
        onChange={(e) => s.go(Number(e.target.value))}
        aria-label="Timeline"
        className="mx-1 h-1 min-w-[8rem] flex-1 accent-[var(--accent)]"
      />
      <span className="min-w-[4.5rem] font-mono text-[11px] tabular-nums text-[var(--muted)]">
        {s.index + 1} / {s.steps.length}
      </span>

      <div className="mx-1 h-5 w-px bg-[var(--line)]" />

      <label className="flex items-center gap-1.5 text-[10px] font-semibold uppercase tracking-wider text-[var(--faint)]">
        speed
        <input
          type="range"
          min={200}
          max={2000}
          step={100}
          value={2200 - s.speed}
          onChange={(e) => s.setSpeed(2200 - Number(e.target.value))}
          aria-label="Playback speed"
          className="h-1 w-16 accent-[var(--accent)]"
        />
      </label>

      {program.modes && (
        <div className="flex items-center gap-1">
          {program.modes.map((m) => (
            <Btn
              key={m.id}
              onClick={() => s.setMode(m.id)}
              title={m.hint ?? m.label}
              active={s.mode === m.id}
              danger={m.danger}
            >
              <span className="font-mono">{m.label}</span>
            </Btn>
          ))}
        </div>
      )}

      {program.schedulable && (
        <Btn onClick={s.shuffle} title="Re-run with a different thread schedule">
          <Dices size={13} />
          shuffle schedule
        </Btn>
      )}

      <Btn
        onClick={s.toggleAddresses}
        title="Show addresses"
        active={s.showAddresses}
      >
        <Hash size={13} />
      </Btn>

      {/* icon swap is pure CSS so there is no theme state to hydrate */}
      <Btn onClick={toggleTheme} title="Toggle theme">
        <Sun size={13} className="hidden dark:block" />
        <Moon size={13} className="block dark:hidden" />
      </Btn>
    </div>
  );
}
