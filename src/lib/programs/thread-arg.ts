import { Machine, lineFinder } from "../machine";
import { runSchedule, type Lane } from "../sched";
import type { Program, Slot } from "../types";

const N = 3;

const src = (mode: string) => `#include <stdio.h>
#include <pthread.h>

#define N ${N}

void *worker(void *arg)
{
    int id = *(int *) arg;
    printf("thread %d\\n", id);
    return NULL;
}

int main(void)
{
    pthread_t t[N];
${mode === "shared" ? "" : "    int ids[N];\n"}
    for (int i = 0; i < N; i++) {
${mode === "shared" ? "" : "        ids[i] = i;\n"}        pthread_create(&t[i], NULL, worker, ${mode === "shared" ? "&i" : "&ids[i]"});
    }

    for (int i = 0; i < N; i++)
        pthread_join(t[i], NULL);

    return 0;
}
`;

export const threadArg: Program = {
  id: "thread-arg",
  title: "One box for all threads, or one each",
  blurb: "The &i bug, simulated: the value a thread reads depends on when it runs.",
  origin: "pthreads / passo-03 e 04",
  concepts: ["pthread_create", "void *", "one slot per thread", "scheduling"],
  schedulable: true,
  modes: [
    { id: "shared", label: "&i", danger: true, hint: "one box, shared by all" },
    { id: "perthread", label: "&ids[i]", hint: "one box per thread" },
  ],
  source: src,
  build(mode, seed) {
    const SRC = src(mode);
    const at = lineFinder(SRC);
    const shared = mode === "shared";
    const m = new Machine();

    m.pushFrame("main");
    m.snap(at("int main"), "main starts. Threads have not been created yet — there is one line of execution.");

    let ids: Slot[] = [];
    if (!shared) {
      ids = m.declareArray("ids", "int", ["?", "?", "?"]);
      m.snap(at("int ids[N]"), "Three separate boxes, at three different addresses. That is the entire fix.");
    }

    const i = m.declare({ name: "i", type: "int", value: "0" });
    m.snap(
      at("for (int i"),
      shared
        ? "The loop counter. There is exactly one `i`, and its address is about to be handed to every thread."
        : "The loop counter. It stays private to main this time.",
    );

    const spawned = [false, false, false];
    const finished = [false, false, false];
    const argSlot: Slot[] = [];

    /* main's lane: set i, (set ids[i]), create — then the joins */
    const mainOps: Lane["ops"] = [];
    for (let k = 0; k < N; k++) {
      mainOps.push({
        run: () => {
          m.write(i, String(k));
          m.snap(at("for (int i"), `Loop turn ${k}: i = ${k}.`);
        },
      });
      if (!shared) {
        mainOps.push({
          run: () => {
            m.read(i);
            m.write(ids[k], String(k));
            m.snap(at("ids[i] = i"), `ids[${k}] = ${k}. Written once, never touched again.`);
          },
        });
      }
      mainOps.push({
        run: () => {
          const t = m.addThread(`thread ${k}`);
          m.setThread(t.id, { state: "ready" });
          argSlot[k] = shared ? i : ids[k];
          spawned[k] = true;
          m.read(argSlot[k]);
          m.snap(
            at("pthread_create"),
            shared
              ? `Thread ${k} created, holding the address of \`i\`. Same address as thread ${k - 1 < 0 ? "…" : k - 1}. It has NOT read anything yet — that happens whenever the scheduler lets it run.`
              : `Thread ${k} created, holding the address of ids[${k}]. Nobody else will ever write there.`,
            { tone: shared ? "warn" : "ok" },
          );
        },
      });
    }

    mainOps.push({
      run: () => {
        m.write(i, String(N));
        m.snap(
          at("for (int i"),
          shared
            ? `The loop increments i one last time to ${N}, then the test fails and it exits. Any thread that has not dereferenced yet will now read ${N} — an id that does not exist.`
            : "The loop ends. Nothing it wrote is going to change again.",
          { tone: shared ? "error" : "info" },
        );
      },
    });

    /* One join per thread, in two ops: main blocks, and only becomes
       runnable again once that thread has finished. While it is blocked the
       scheduler physically cannot pick it, which is what makes the wait
       visible instead of merely implied. */
    for (let k = 0; k < N; k++) {
      mainOps.push({
        run: () => {
          m.setThread(0, { state: "blocked", detail: `waiting for thread ${k}` });
          m.snap(
            at("pthread_join"),
            finished[k]
              ? `main calls pthread_join(t[${k}]). That thread already finished, so this returns straight away — join waits for a thread, it does not make one run.`
              : `main calls pthread_join(t[${k}]) and blocks. It cannot execute another line until thread ${k} is done; watch the scheduler hand the CPU to the workers.`,
            { tone: "warn" },
          );
        },
      });
      mainOps.push({
        ready: () => finished[k],
        run: () => {
          m.setThread(0, { state: "running", detail: undefined });
          m.snap(
            at("pthread_join"),
            `Thread ${k} finished, so join returned and main is runnable again.`,
            { tone: "ok" },
          );
        },
      });
    }

    mainOps.push({
      ready: () => finished.every(Boolean),
      run: () => {
        m.setThread(0, { state: "done", line: null });
        m.snap(
          at("return 0"),
          shared
            ? `Three arrows, one box. Every thread dereferenced the same address and got whatever \`i\` happened to hold at that instant. Shuffle the schedule: the ids change, and a thread that runs late reads ${N}.`
            : "Three arrows, three boxes, no overlap. Every thread found its own id whatever order they ran in — the result stopped depending on timing.",
          { tone: shared ? "error" : "ok" },
        );
      },
    });

    /* one lane per thread: read the argument, then print */
    const threadLanes: Lane[] = [];
    for (let k = 0; k < N; k++) {
      threadLanes.push({
        ops: [
          {
            ready: () => spawned[k],
            run: () => {
              const tid = k + 1;
              m.setThread(tid, { state: "running" });
              m.pushFrame("worker", tid);
              const arg = argSlot[k];
              /* `void *arg` is a parameter, which means it is a real box on
                 this thread's own stack holding a copy of the address. It is
                 what the arrows point from. */
              const argBox = m.declare(
                { name: "arg", type: "void *", value: "", size: 8, kind: "pointer" },
                tid,
              );
              m.aim(argBox, arg);
              const read = m.read(arg);
              const idSlot = m.declare(
                { name: "id", type: "int", value: read },
                tid,
              );
              const wrong = shared && read !== String(k);
              if (wrong) idSlot.tone = "error";
              m.snap(
                at("int id = *(int *)"),
                shared
                  ? `Thread ${k} dereferences the shared box and gets ${read}.${wrong ? ` It was created when i was ${k} — but i has moved on.` : " It happened to run before main changed i."}`
                  : `Thread ${k} dereferences its own box and gets ${read}.`,
                { thread: tid, tone: wrong ? "error" : "ok" },
              );
            },
          },
          {
            ready: () => spawned[k],
            run: () => {
              const tid = k + 1;
              const frame = m.frames.find((f) => f.thread === tid && !f.dead);
              const idSlot = frame?.slots.find((s) => s.name === "id");
              const val = idSlot ? m.read(idSlot) : "?";
              m.print(`thread ${val}`);
              m.popFrame(tid, true);
              m.setThread(tid, { state: "done", line: null });
              finished[k] = true;
              m.snap(at("printf("), `Thread ${k} prints and exits.`, { thread: tid });
            },
          },
        ],
      });
    }

    runSchedule([{ ops: mainOps }, ...threadLanes], seed);
    return m.done();
  },
};
