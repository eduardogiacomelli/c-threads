# C Machine

Step through C programs and watch the stack, the heap and the threads move.

Built for INE5645 prep: the programs mirror the `passo-NN` files in
`~/c-playground/c-do-zero/` and
`~/vaultin/Vaulters/01_Courses/PPD/01_PPD_Go/07_Pthreads_Do_Zero/`.

```bash
pnpm dev      # http://localhost:3000
pnpm build
pnpm lint
```

## What it does

Six programs, each replayed one machine step at a time:

| # | Program | Shows |
|---|---------|-------|
| 1 | Address and value | `&`, `*`, writing through a pointer vs re-aiming it, NULL |
| 2 | The frame dies, the address stays | stack frame reuse, dangling pointer |
| 3 | One step past the end | array layout, off-by-one, who gets clobbered |
| 4 | A struct on the heap | `malloc`, `->`, use-after-free |
| 5 | One box for all threads, or one each | the `&i` bug, one slot per thread |
| 6 | `counter++` is three operations | load / add / store, lost update |

Every panel is live: hover any identifier in the source to read its current
value, address and size; click a line to jump the timeline there; arrow keys
step, space plays.

Programs 5 and 6 are **simulated, not scripted**. Each thread is a lane of
ops, and a seeded scheduler interleaves the lanes while keeping each lane in
order — the same constraint a real scheduler obeys. `shuffle schedule` picks a
new seed, so the race genuinely prints 11 on some runs and 12 on others, and
the `&i` program genuinely reads a different id depending on when a thread
gets the CPU.

## How it is put together

```
src/lib/types.ts        the machine model: Slot, Frame, HeapBlock, Step
src/lib/machine.ts      mutable draft + snapshot builder, address allocation
src/lib/sched.ts        the thread scheduler
src/lib/csyntax.ts      C tokenizer (identifiers stay addressable)
src/lib/programs/*.ts   one builder per program
src/components/         code pane, memory pane, arrows, controls, panels
src/store/              zustand: which program, which mode, which step
```

There is **no C interpreter**. Each program ships a builder that walks a
known-correct execution and calls `snap()` to freeze a complete picture of
memory. Adding a program means writing one builder, not extending a VM.

Two decisions worth knowing about:

- **Frame addresses come from `(thread, depth)`, never a counter.** That is
  what makes a returned frame and the next call land on the *same* address —
  the entire point of program 2.
- **The tokenizer is hand-written.** Not for colour: identifiers have to stay
  addressable so the code pane can bind the `p` in the source to the live slot
  named `p`. Shiki and Prism hand back opaque markup.

## Adding a program

```ts
// src/lib/programs/my-thing.ts
export const myThing: Program = {
  id: "my-thing",
  title: "…",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();
    m.pushFrame("main");
    const x = m.declare({ name: "x", type: "int", value: "1" });
    m.snap(at("int x"), "what just happened, in one sentence");
    return m.done();
  },
};
```

Register it in `src/lib/programs/index.ts`. Use `at("some code")` rather than
a line number so editing the C source cannot silently desync the trace.

## Stack

Next 16 (App Router, Turbopack) · React 19 · TypeScript · Tailwind 4 ·
motion · zustand · lucide-react.

Theming is a `dark` class on `<html>`, set before first paint by a blocking
snippet in `layout.tsx`, with every colour in the app coming from a CSS
variable. `next-themes` was tried first and removed: it renders its script
inside `<body>`, which React 19 rejects and which broke hydration.
