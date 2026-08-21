"use client";

import { useMemo } from "react";
import { Cpu } from "lucide-react";
import { CodePane } from "@/components/code-pane";
import { Controls } from "@/components/controls";
import { MemoryPane } from "@/components/memory-pane";
import { ConsoleOut, Narration, ThreadRail } from "@/components/panels";
import { ProgramPicker } from "@/components/program-picker";
import { byId } from "@/lib/programs";
import { useMachine } from "@/store/machine-store";

export default function Page() {
  const { programId, mode, seed, steps, index, source, showAddresses, showBytes, go } =
    useMachine();
  const program = byId(programId);
  const step = steps[Math.min(index, steps.length - 1)];
  const stepLines = useMemo(() => steps.map((s) => s.line), [steps]);

  return (
    <div className="flex h-screen flex-col gap-3 p-3 lg:p-4">
      <header className="flex flex-wrap items-baseline justify-between gap-x-4 gap-y-1">
        <div className="flex items-baseline gap-2.5">
          <Cpu size={17} className="translate-y-0.5 text-[var(--accent)]" />
          <h1 className="text-base font-semibold tracking-tight">C Machine</h1>
          <p className="text-[12px] text-[var(--muted)]">
            step through the program and watch the boxes move
          </p>
        </div>
        <div className="flex flex-wrap gap-1.5">
          {program.concepts.map((c) => (
            <span
              key={c}
              className="rounded border border-[var(--line)] px-1.5 py-0.5 font-mono text-[10px] text-[var(--muted)]"
            >
              {c}
            </span>
          ))}
        </div>
      </header>

      <div className="grid min-h-0 flex-1 grid-cols-1 gap-3 lg:grid-cols-[15rem_minmax(0,1fr)] xl:grid-cols-[16rem_minmax(0,1fr)]">
        <aside className="hidden min-h-0 flex-col gap-3 overflow-auto lg:flex">
          <ProgramPicker />
          <div className="mt-auto rounded-lg border border-[var(--line)] bg-[var(--sunken)] p-2.5 text-[11px] leading-relaxed text-[var(--muted)]">
            <p className="mb-1 font-semibold text-[var(--text)]">Try this</p>
            <p>
              Hover any identifier in the code to read its current value,
              address and size. Click a line to jump the timeline there.
              Arrow keys step, space plays.
            </p>
          </div>
        </aside>

        <main className="grid min-h-0 grid-rows-[minmax(0,1fr)_auto] gap-3">
          <div className="grid min-h-0 grid-cols-1 gap-3 xl:grid-cols-[minmax(0,26rem)_minmax(0,1fr)]">
            <div className="min-h-0">
              <CodePane
                source={source}
                step={step}
                stepLines={stepLines}
                onJump={go}
              />
            </div>
            <div className="grid min-h-0 grid-rows-[minmax(0,1fr)_auto] gap-3">
              <MemoryPane
                key={`${programId}-${mode}-${seed}`}
                step={step}
                showAddresses={showAddresses}
                showBytes={showBytes}
              />
              <div className="grid gap-3 md:grid-cols-[minmax(0,1fr)_minmax(0,16rem)]">
                <div className="grid gap-2">
                  <ThreadRail step={step} />
                  <Narration step={step} />
                </div>
                <ConsoleOut step={step} />
              </div>
            </div>
          </div>

          <Controls />
        </main>
      </div>
    </div>
  );
}
