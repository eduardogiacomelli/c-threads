import { Machine, lineFinder } from "../machine";
import { runSchedule, type Lane } from "../sched";
import type { Program, Slot } from "../types";

const THREADS = 2;
const PER = 2;
const EXPECTED = THREADS * PER;

const src = (mode: string) => `#include <stdio.h>
#include <pthread.h>
${mode === "atomic" ? "#include <stdatomic.h>\n" : ""}
${mode === "atomic" ? "_Atomic int" : "volatile int"} counter = 0;

void *bump(void *arg)
{
    (void) arg;
    for (int i = 0; i < ${PER}; i++)
${
  mode === "atomic"
    ? `        counter++;             /* one indivisible lock addq */`
    : `        counter++;             /* three separate operations */`
}
    return NULL;
}

int main(void)
{
    pthread_t a, b;

    pthread_create(&a, NULL, bump, NULL);
    pthread_create(&b, NULL, bump, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("counter = %d\\n", counter);   /* expected ${EXPECTED} */
    return 0;
}
`;

export const atomics: Program = {
  id: "atomics",
  title: "_Atomic closes the window",
  blurb: "The same increment as one indivisible step instead of three.",
  origin: "c-do-zero / passo-37 e 38",
  concepts: ["_Atomic", "lock addq", "read-modify-write", "no window"],
  takeaway:
    "volatile forces the memory access; _Atomic makes it indivisible. Only the second one prevents a lost update, and neither is as fast as giving every thread its own accumulator.",
  schedulable: true,
  modes: [
    { id: "plain", label: "volatile int", danger: true, hint: "three operations" },
    { id: "atomic", label: "_Atomic int", hint: "one operation" },
  ],
  source: src,
  build(mode, seed) {
    const SRC = src(mode);
    const at = lineFinder(SRC);
    const m = new Machine();
    const atomic = mode === "atomic";
    const names = ["a", "b"];

    const counter = m.global({
      name: "counter",
      type: atomic ? "_Atomic int" : "volatile int",
      value: "0",
    });
    m.pushFrame("main");
    m.snap(
      at("int main"),
      atomic
        ? `\`counter\` is _Atomic. Every ++ on it compiles to a single \`lock addq\`: one instruction the CPU refuses to interleave. Expected result after ${EXPECTED} increments: ${EXPECTED}.`
        : `\`counter\` is volatile, which forces every access to hit memory and prevents nothing else. Each ++ is still load, add, store. Expected: ${EXPECTED}.`,
      { tone: atomic ? "ok" : "warn" },
    );

    const spawned = [false, false];
    const finished = [false, false];
    const regs: (Slot | undefined)[] = [];

    const firstCreate = at("pthread_create");
    const createLine = [firstCreate, at("pthread_create", firstCreate)];

    const mainOps: Lane["ops"] = [0, 1].map((k) => ({
      run: () => {
        const t = m.addThread(`thread ${names[k]}`);
        m.setThread(t.id, { state: "ready" });
        spawned[k] = true;
        m.snap(createLine[k], `Thread ${names[k]} created.`);
      },
    }));

    const firstJoin = at("pthread_join");
    const joinLine = [firstJoin, at("pthread_join", firstJoin)];

    for (let k = 0; k < THREADS; k++) {
      mainOps.push({
        run: () => {
          m.setThread(0, { state: "blocked", detail: `waiting for ${names[k]}` });
          m.snap(joinLine[k], `main blocks until thread ${names[k]} finishes.`, {
            tone: "warn",
          });
        },
      });
      mainOps.push({
        ready: () => finished[k],
        run: () => {
          m.setThread(0, { state: "running", detail: undefined });
          m.snap(joinLine[k], `Thread ${names[k]} is done; join returns.`, {
            tone: "ok",
          });
        },
      });
    }

    mainOps.push({
      ready: () => finished.every(Boolean),
      run: () => {
        const got = Number(counter.value);
        const lost = EXPECTED - got;
        m.read(counter);
        m.print(`counter = ${got}`);
        m.setThread(0, { state: "done", line: null });
        m.snap(
          at("printf("),
          atomic
            ? `${got}. Shuffle the schedule as often as you like: an _Atomic increment has no half-finished state for another thread to land in, so there is no interleaving that can lose one.`
            : lost === 0
              ? `${got} this time, which is correct by luck. Nothing was fixed; the two threads simply never overlapped. Shuffle the schedule and watch it break.`
              : `${got}. ${lost} increment${lost > 1 ? "s" : ""} vanished. Two threads loaded the same value before either stored, and the second store wrote over the first.`,
          { tone: atomic ? "ok" : lost === 0 ? "warn" : "error" },
        );
      },
    });

    /* one lane per worker */
    const laneFor = (k: number): Lane => {
      const tid = k + 1;
      const ops: Lane["ops"] = [];

      for (let i = 0; i < PER; i++) {
        if (atomic) {
          /* A single op, because there is a single instruction. The thread
             has no register slot at all: nothing is ever half done. */
          ops.push({
            ready: () => spawned[k],
            run: () => {
              m.setThread(tid, { state: "running" });
              if (i === 0) m.pushFrame("bump", tid);
              const before = Number(counter.value);
              m.read(counter);
              m.write(counter, String(before + 1));
              m.snap(
                at("counter++"),
                `Thread ${names[k]}: lock addq. Read, add and write happen as ONE step that no other core can interrupt, so counter goes ${before} to ${before + 1} with no window in between.`,
                { thread: tid, tone: "ok" },
              );
            },
          });
        } else {
          ops.push({
            ready: () => spawned[k],
            run: () => {
              m.setThread(tid, { state: "running" });
              if (i === 0) m.pushFrame("bump", tid);
              const v = m.read(counter);
              const reg =
                regs[k] ??
                m.declare({ name: "reg", type: "int", value: v }, tid);
              m.write(reg, v);
              regs[k] = reg;
              m.setThread(tid, { detail: `reg = ${v}` });
              m.snap(
                at("counter++"),
                `LOAD. Thread ${names[k]} copied ${v} into a register of its own. Memory is untouched, and this is the window: whatever happens next, this thread still believes counter is ${v}.`,
                { thread: tid },
              );
            },
          });
          ops.push({
            ready: () => spawned[k],
            run: () => {
              const reg = regs[k]!;
              const next = String(Number(reg.value) + 1);
              m.write(reg, next);
              m.setThread(tid, { detail: `reg = ${next}` });
              m.snap(
                at("counter++"),
                `ADD. Thread ${names[k]} now holds ${next}. Still nothing written back.`,
                { thread: tid },
              );
            },
          });
          ops.push({
            ready: () => spawned[k],
            run: () => {
              const reg = regs[k]!;
              const before = counter.value;
              const clobbered = Number(before) >= Number(reg.value);
              m.read(reg);
              m.write(counter, reg.value);
              m.snap(
                at("counter++"),
                clobbered
                  ? `STORE. Thread ${names[k]} writes ${reg.value} over a box that already held ${before}. The other thread's increment just disappeared.`
                  : `STORE. Thread ${names[k]} writes ${reg.value} back to memory.`,
                { thread: tid, tone: clobbered ? "error" : "info" },
              );
            },
          });
        }
      }

      ops.push({
        ready: () => spawned[k],
        run: () => {
          m.popFrame(tid, true);
          m.setThread(tid, { state: "done", line: null, detail: undefined });
          finished[k] = true;
          m.snap(at("return NULL"), `Thread ${names[k]} exits.`, { thread: tid });
        },
      });

      return { ops };
    };

    runSchedule([{ ops: mainOps }, laneFor(0), laneFor(1)], seed);
    return m.done();
  },
};
