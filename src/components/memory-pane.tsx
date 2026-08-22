"use client";

import { useRef } from "react";
import { motion } from "motion/react";
import { Layers, Boxes, Globe } from "lucide-react";
import { Arrows } from "./arrows";
import { SlotBox } from "./slot-box";
import { hex } from "@/lib/machine";
import type { Frame, Step } from "@/lib/types";
import { cn } from "@/lib/utils";

const THREAD_COLORS = [
  "var(--t0)",
  "var(--t1)",
  "var(--t2)",
  "var(--t3)",
  "var(--t4)",
  "var(--t5)",
];

export const threadColor = (id: number) => THREAD_COLORS[id % THREAD_COLORS.length];

function PanelTitle({
  icon: Icon,
  children,
  color,
}: {
  icon: React.ComponentType<{ size?: number; className?: string }>;
  children: React.ReactNode;
  color?: string;
}) {
  return (
    <div
      className="flex items-center gap-1.5 text-[10px] font-semibold uppercase tracking-[0.12em]"
      style={{ color: color ?? "var(--faint)" }}
    >
      <Icon size={12} />
      {children}
    </div>
  );
}

function FrameCard({
  frame,
  step,
  showAddresses,
  showBytes,
}: {
  frame: Frame;
  step: Step;
  showAddresses: boolean;
  showBytes: boolean;
}) {
  const accent = threadColor(frame.thread);
  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: -6 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.2 }}
      className={cn(
        "grid gap-1.5 rounded border p-2",
        frame.dead
          ? "border-dashed border-[var(--line)] opacity-60"
          : "border-[var(--line)] bg-[var(--panel)]",
      )}
    >
      <div className="flex items-baseline justify-between gap-2">
        <span
          className="font-mono text-[11px] font-medium"
          style={{ color: frame.dead ? "var(--faint)" : accent }}
        >
          {frame.fn}()
        </span>
        <span className="font-mono text-[10px] text-[var(--faint)]">
          {frame.dead ? "returned" : `depth ${frame.depth}`}
        </span>
      </div>

      {frame.slots.length === 0 ? (
        <span className="px-1 font-mono text-[11px] text-[var(--faint)]">
          no locals yet
        </span>
      ) : (
        frame.slots.map((slot) => (
          <SlotBox
            key={slot.id}
            slot={slot}
            read={step.reads.includes(slot.id)}
            written={step.writes.includes(slot.id)}
            showAddress={showAddresses}
            showBytes={showBytes}
          />
        ))
      )}
    </motion.div>
  );
}

export function MemoryPane({
  step,
  showAddresses,
  showBytes,
}: {
  step: Step;
  showAddresses: boolean;
  showBytes: boolean;
}) {
  const ref = useRef<HTMLDivElement>(null);

  const threads = step.threads.filter(
    (t) => t.id === 0 || step.frames.some((f) => f.thread === t.id) || t.state !== "done",
  );

  return (
    <div
      ref={ref}
      className="gridpaper relative h-full overflow-auto rounded-lg border border-[var(--line)] bg-[var(--sunken)] p-4"
    >
      <Arrows containerRef={ref} step={step} />

      {showBytes && (
        /* without this line the reversed bytes read as a rendering bug */
        <div className="relative mb-3 flex flex-wrap items-center gap-x-3 gap-y-1 rounded border border-[var(--line)] bg-[var(--panel)] px-2.5 py-1.5 font-mono text-[10px] text-[var(--muted)]">
          <span className="font-sans font-semibold uppercase tracking-[0.12em] text-[var(--faint)]">
            byte view
          </span>
          <span>lowest address first - x86-64 is little-endian, so 25 is 19 00 00 00</span>
          <span className="flex items-center gap-1">
            <span className="rounded-[2px] border border-dashed border-[var(--line)] px-1 text-[var(--faint)]">
              ··
            </span>
            alignment padding: real bytes, holding nothing
          </span>
        </div>
      )}

      <div className="relative flex min-w-max items-start gap-5">
        {/* statics ------------------------------------------------- */}
        {step.globals.length > 0 && (
          <section className="grid min-w-[13rem] gap-1.5">
            <PanelTitle icon={Globe}>static · shared by all threads</PanelTitle>
            <div className="grid gap-1.5 rounded border border-[var(--line)] bg-[var(--panel)] p-2">
              {step.globals.map((slot) => (
                <SlotBox
                  key={slot.id}
                  slot={slot}
                  read={step.reads.includes(slot.id)}
                  written={step.writes.includes(slot.id)}
                  showAddress={showAddresses}
                  showBytes={showBytes}
                />
              ))}
            </div>
          </section>
        )}

        {/* one stack per thread ------------------------------------ */}
        {threads.map((thread) => {
          const frames = step.frames.filter((f) => f.thread === thread.id);
          if (frames.length === 0 && thread.state === "not-started") return null;
          return (
            <section key={thread.id} className="grid min-w-[14rem] gap-1.5">
              <PanelTitle icon={Layers} color={threadColor(thread.id)}>
                stack · {thread.name}
                {thread.state === "blocked" && (
                  <span className="ml-1 font-mono text-[9px] normal-case tracking-normal text-[var(--warn)]">
                    blocked{thread.detail ? ` · ${thread.detail}` : ""}
                  </span>
                )}
              </PanelTitle>
              <div className="grid gap-1.5">
                {/* no AnimatePresence: a popped frame is either kept as an
                    explicit ghost or genuinely gone, and an exit animation
                    that stalls leaves stale boxes on screen */}
                {frames.map((frame) => (
                  <FrameCard
                    key={frame.id}
                    frame={frame}
                    step={step}
                    showAddresses={showAddresses}
                    showBytes={showBytes}
                  />
                ))}
                {frames.length === 0 && (
                  <div className="rounded border border-dashed border-[var(--line)] p-2 font-mono text-[11px] text-[var(--faint)]">
                    {thread.state === "done" ? "finished" : "waiting to run"}
                  </div>
                )}
              </div>
            </section>
          );
        })}

        {/* heap ----------------------------------------------------- */}
        {step.heap.length > 0 && (
          <section className="grid min-w-[14rem] gap-1.5">
            <PanelTitle icon={Boxes}>heap · yours until free</PanelTitle>
            <div className="grid gap-1.5">
              {step.heap.map((block) => (
                <div
                  key={block.id}
                  className={cn(
                    "grid gap-1.5 rounded border p-2",
                    block.freed
                      ? "border-dashed border-[var(--danger)] opacity-70"
                      : "border-[var(--line)] bg-[var(--panel)]",
                  )}
                >
                  <div className="flex items-baseline justify-between gap-2">
                    <span className="font-mono text-[11px] text-[var(--muted)]">
                      {block.label}
                    </span>
                    <span className="font-mono text-[10px] text-[var(--faint)]">
                      {block.freed ? "freed" : `${block.size} B @ ${hex(block.addr)}`}
                    </span>
                  </div>
                  {block.slots.map((slot) => (
                    <SlotBox
                      key={slot.id}
                      slot={slot}
                      read={step.reads.includes(slot.id)}
                      written={step.writes.includes(slot.id)}
                      showAddress={showAddresses}
                      showBytes={showBytes}
                    />
                  ))}
                </div>
              ))}
            </div>
          </section>
        )}
      </div>
    </div>
  );
}
