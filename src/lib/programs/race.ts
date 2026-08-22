import { Machine, lineFinder } from "../machine";
import { runSchedule, type Lane } from "../sched";
import type { Program, Slot } from "../types";

const SRC = `#include <stdio.h>
#include <pthread.h>

int counter = 10;              /* shared: one box for everyone */

void *bump(void *arg)
{
    int reg = counter;         /* LOAD  */
    reg = reg + 1;             /* ADD   */
    counter = reg;             /* STORE */
    return NULL;
}

int main(void)
{
    pthread_t a, b;

    pthread_create(&a, NULL, bump, NULL);
    pthread_create(&b, NULL, bump, NULL);

    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("counter = %d\\n", counter);   /* expected 12 */
    return 0;
}
`;

export const race: Program = {
  id: "race",
  title: "counter++ is three operations",
  blurb: "Two threads, three steps each, and a lost update that depends only on order.",
  origin: "pthreads / passo-12",
  concepts: ["data race", "load-add-store", "shared vs private", "non-atomic"],
  schedulable: true,
  takeaway:
    "If two threads can touch the same box and either one writes, that is a race - however short the window looks in testing. `x++` is three operations, not one.",
  source: () => SRC,
  build(_mode, seed) {
    const at = lineFinder(SRC);
    const m = new Machine();

    const counter = m.global({ name: "counter", type: "int", value: "10" });
    m.pushFrame("main");
    m.snap(
      at("int main"),
      "`counter` is global - one box, shared by every thread. Expected result after two increments: 12.",
    );

    const spawned = [false, false];
    const finished = [false, false];
    const regs: (Slot | undefined)[] = [];
    const names = ["a", "b"];

    const firstCreate = at("pthread_create");
    const createLine = [firstCreate, at("pthread_create", firstCreate)];

    const mainOps: Lane["ops"] = [0, 1].map((k) => ({
      run: () => {
        const t = m.addThread(`thread ${names[k]}`);
        m.setThread(t.id, { state: "ready" });
        spawned[k] = true;
        m.snap(
          createLine[k],
          `Thread ${names[k]} created. It will run its three steps at some point - the scheduler decides when.`,
        );
      },
    }));

    const firstJoin = at("pthread_join");
    const joinLine = [firstJoin, at("pthread_join", firstJoin)];

    for (let k = 0; k < 2; k++) {
      mainOps.push({
        run: () => {
          m.setThread(0, {
            state: "blocked",
            detail: `waiting for thread ${names[k]}`,
          });
          m.snap(
            joinLine[k],
            `main blocks in pthread_join on thread ${names[k]}. Nothing in main runs until that thread is done.`,
            { tone: "warn" },
          );
        },
      });
      mainOps.push({
        ready: () => finished[k],
        run: () => {
          m.setThread(0, { state: "running", detail: undefined });
          m.snap(joinLine[k], `Thread ${names[k]} is finished; join returns.`, {
            tone: "ok",
          });
        },
      });
    }

    mainOps.push({
      ready: () => finished.every(Boolean),
      run: () => {
        const value = Number(counter.value);
        m.read(counter);
        m.print(`counter = ${value}`);
        m.setThread(0, { state: "done" });
        m.snap(
          at("printf("),
          value === 12
            ? "12 this time. Nothing was fixed - the two threads simply never overlapped. The exact same binary can print 11 on the next run."
            : `${value}: one increment vanished. Both threads loaded 10 before either stored, so the second store overwrote the first. No crash, no warning, just a wrong number.`,
          { tone: value === 12 ? "warn" : "error" },
        );
      },
    });

    const laneFor = (k: number): Lane => {
      const tid = k + 1;
      return {
        ops: [
          {
            ready: () => spawned[k],
            run: () => {
              m.setThread(tid, { state: "running" });
              m.pushFrame("bump", tid);
              const v = m.read(counter);
              const reg = m.declare({ name: "reg", type: "int", value: v }, tid);
              regs[k] = reg;
              m.setThread(tid, { detail: `reg = ${v}` });
              m.snap(
                at("int reg = counter"),
                `LOAD - thread ${names[k]} copied ${v} out of the shared box into its own \`reg\`. \`reg\` is a local: each thread has its own, at its own address. Memory is untouched.`,
                { thread: tid },
              );
            },
          },
          {
            ready: () => spawned[k],
            run: () => {
              const reg = regs[k]!;
              const v = String(Number(reg.value) + 1);
              m.write(reg, v);
              m.setThread(tid, { detail: `reg = ${v}` });
              m.snap(
                at("reg = reg + 1"),
                `ADD - thread ${names[k]} now holds ${v} in its register. Still nothing written back: the shared box is unchanged.`,
                { thread: tid },
              );
            },
          },
          {
            ready: () => spawned[k],
            run: () => {
              const reg = regs[k]!;
              const before = counter.value;
              m.read(reg);
              m.write(counter, reg.value);
              const clobbered = Number(before) >= Number(reg.value);
              m.popFrame(tid, false);
              m.setThread(tid, { state: "done", line: null, detail: undefined });
              finished[k] = true;
              m.snap(
                at("counter = reg"),
                clobbered
                  ? `STORE - thread ${names[k]} wrote ${reg.value} over a box that already held ${before}. The other thread's work just disappeared.`
                  : `STORE - thread ${names[k]} wrote ${reg.value} back to memory.`,
                { thread: tid, tone: clobbered ? "error" : "info" },
              );
            },
          },
        ],
      };
    };

    runSchedule([{ ops: mainOps }, laneFor(0), laneFor(1)], seed);
    return m.done();
  },
};
