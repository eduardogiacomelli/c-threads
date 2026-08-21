import { Machine, hex, lineFinder } from "../machine";
import { runSchedule, type Lane } from "../sched";
import type { HeapBlock, Program, Slot } from "../types";

const N = 3;
const DATA = [10, 20, 30, 40, 50, 60];
const TOTAL = DATA.reduce((a, b) => a + b, 0);

const SRC = `#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 3

int data[6] = {10, 20, 30, 40, 50, 60};   /* shared, but READ ONLY */

typedef struct {
    int  id;
    int  from, to;
    long sum;                  /* this thread's output slot */
} Task;

void *partial_sum(void *arg)
{
    Task *t = (Task *) arg;
    long sum = 0;

    for (int i = t->from; i < t->to; i++)
        sum += data[i];        /* reading shared data needs no lock */

    t->sum = sum;              /* nobody else writes here */
    return NULL;
}

int main(void)
{
    pthread_t th[N];
    Task *tasks[N];

    for (int k = 0; k < N; k++) {
        tasks[k] = malloc(sizeof(Task));
        *tasks[k] = (Task){ k, k * 2, k * 2 + 2, 0 };
        pthread_create(&th[k], NULL, partial_sum, tasks[k]);
    }

    long total = 0;
    for (int k = 0; k < N; k++) {
        pthread_join(th[k], NULL);
        total += tasks[k]->sum;    /* combined AFTER the join */
        free(tasks[k]);
    }

    printf("total = %ld\\n", total);
    return 0;
}
`;

export const workSplit: Program = {
  id: "work-split",
  title: "Split the work, combine after the join",
  blurb: "The pattern that needs no mutex: private output slots, merged by main.",
  origin: "pthreads / Ex. 5, 6, 7 e os desafios",
  concepts: ["one struct per thread", "private output", "join then combine", "no lock needed"],
  schedulable: true,
  source: () => SRC,
  build(_mode, seed) {
    const at = lineFinder(SRC);
    const m = new Machine();

    const cells: Slot[] = DATA.map((v, i) =>
      m.global({ name: `data[${i}]`, type: "int", value: String(v) }),
    );
    m.pushFrame("main");
    m.snap(
      at("int main"),
      `\`data\` is shared by every thread — and that is fine, because nobody writes to it. Concurrent reads of memory that never changes need no protection at all.`,
    );

    const spawned = [false, false, false];
    const finished = [false, false, false];
    const blocks: HeapBlock[] = [];
    let total: Slot | undefined;

    const mainOps: Lane["ops"] = [];

    for (let k = 0; k < N; k++) {
      mainOps.push({
        run: () => {
          const from = k * 2;
          const to = k * 2 + 2;
          const block = m.malloc(`tasks[${k}]`, [
            { name: "id", type: "int", value: String(k), size: 4 },
            { name: "from", type: "int", value: String(from), size: 4 },
            { name: "to", type: "int", value: String(to), size: 4 },
            { name: "sum", type: "long", value: "0", size: 8 },
          ]);
          blocks[k] = block;
          const t = m.addThread(`thread ${k}`);
          m.setThread(t.id, { state: "ready" });
          spawned[k] = true;
          m.snap(
            at("pthread_create"),
            `Thread ${k} gets its own malloc'd Task at ${hex(block.addr)}: which slice to read (${from}..${to - 1}) and where to put the answer. One block per thread — the passo-16 pattern.`,
            { tone: "ok" },
          );
        },
      });
    }

    mainOps.push({
      run: () => {
        total = m.declare({ name: "total", type: "long", value: "0", size: 8 });
        m.snap(
          at("long total = 0"),
          "main's accumulator. It stays on main's stack and no thread can see it — which is exactly why no lock is needed here either.",
        );
      },
    });

    for (let k = 0; k < N; k++) {
      mainOps.push({
        run: () => {
          m.setThread(0, { state: "blocked", detail: `waiting for thread ${k}` });
          m.snap(
            at("pthread_join"),
            `main blocks on thread ${k}. The join is also what makes that thread's write visible here: everything it did happens-before this call returns, which is why no lock is needed to read the result.`,
            { tone: "warn" },
          );
        },
      });
      mainOps.push({
        ready: () => finished[k],
        run: () => {
          m.setThread(0, { state: "running", detail: undefined });
          const sumSlot = blocks[k].slots[3];
          const before = Number(total!.value);
          const add = Number(m.read(sumSlot));
          m.write(total!, String(before + add));
          m.snap(
            at("total += tasks[k]->sum"),
            `join returned for thread ${k}, so its work is finished and its result is visible. total = ${before} + ${add} = ${before + add}. The addition happens on main's thread, one at a time.`,
            { tone: "ok" },
          );
        },
      });
      mainOps.push({
        run: () => {
          m.free(blocks[k]);
          m.snap(at("free(tasks[k])"), `Task ${k} is done with; free it. Allocated by main, freed by main.`);
        },
      });
    }

    mainOps.push({
      run: () => {
        m.read(total!);
        m.print(`total = ${TOTAL}`);
        m.setThread(0, { state: "done", line: null });
        m.snap(
          at("printf("),
          `${TOTAL}. Shuffle the schedule as many times as you like — this program prints ${TOTAL} on every possible interleaving, because no two threads ever write to the same box. That is the whole trick, and it is cheaper than a mutex.`,
          { tone: "ok" },
        );
      },
    });

    /* one lane per worker: take the argument, sum a slice, publish */
    const lanes: Lane[] = [];
    for (let k = 0; k < N; k++) {
      const tid = k + 1;
      lanes.push({
        ops: [
          {
            ready: () => spawned[k],
            run: () => {
              m.setThread(tid, { state: "running" });
              m.pushFrame("partial_sum", tid);
              const t = m.declare(
                { name: "t", type: "Task *", value: "", size: 8, kind: "pointer" },
                tid,
              );
              t.points = blocks[k].slots[0].id;
              t.value = hex(blocks[k].addr);
              m.declare({ name: "sum", type: "long", value: "0", size: 8 }, tid);
              m.snap(
                at("Task *t = (Task *) arg"),
                `Thread ${k} casts the void * back to Task * and now reaches its own block. Follow the arrow: no two threads point at the same one.`,
                { thread: tid },
              );
            },
          },
          {
            ready: () => spawned[k],
            run: () => {
              const frame = m.frames.find((f) => f.thread === tid && !f.dead)!;
              const sum = frame.slots.find((s) => s.name === "sum")!;
              const from = k * 2;
              const slice = [cells[from], cells[from + 1]];
              slice.forEach((c) => m.read(c));
              m.read(blocks[k].slots[1]);
              m.read(blocks[k].slots[2]);
              const value = DATA[from] + DATA[from + 1];
              m.write(sum, String(value));
              m.setThread(tid, { detail: `sum = ${value}` });
              m.snap(
                at("sum += data[i]"),
                `Thread ${k} reads data[${from}] and data[${from + 1}] — shared memory, read-only — and accumulates ${value} into its own local \`sum\`.`,
                { thread: tid },
              );
            },
          },
          {
            ready: () => spawned[k],
            run: () => {
              const frame = m.frames.find((f) => f.thread === tid && !f.dead)!;
              const sum = frame.slots.find((s) => s.name === "sum")!;
              m.read(sum);
              m.write(blocks[k].slots[3], sum.value);
              /* ghost rather than remove: keeping every `t` on screen is what
                 makes "three arrows, three blocks, no overlap" visible at the
                 end instead of only during the few steps threads overlap */
              m.popFrame(tid, true);
              m.setThread(tid, { state: "done", line: null, detail: undefined });
              finished[k] = true;
              m.snap(
                at("t->sum = sum"),
                `Thread ${k} publishes ${sum.value} into its own block. This is the only write it makes to shared memory, and it is the only thread that ever touches that slot — so there is nothing to race with.`,
                { thread: tid, tone: "ok" },
              );
            },
          },
        ],
      });
    }

    runSchedule([{ ops: mainOps }, ...lanes], seed);
    return m.done();
  },
};
