# C Machine

Step through C programs and watch the stack, the heap and the threads move.

Built for INE5645 prep. The programs mirror the `passo-NN` tutorial in
[`c/`](c/), which is the same material as runnable C.

```bash
pnpm dev      # http://localhost:3000
pnpm build
pnpm lint
```

## What it does

Eight programs, each replayed one machine step at a time:

| # | Program | Shows |
|---|---------|-------|
| 1 | Address and value | `&`, `*`, writing through a pointer vs re-aiming it, NULL |
| 2 | One step past the end | array layout, off-by-one, who gets clobbered |
| 3 | A string is a char array with one rule | the `'\0'`, strlen walking, strcpy vs snprintf |
| 4 | The frame dies, the address stays | stack frame reuse, dangling pointer |
| 5 | A struct on the heap | `malloc`, `->`, use-after-free |
| 6 | One box for all threads, or one each | the `&i` bug, one slot per thread |
| 7 | `counter++` is three operations | load / add / store, lost update |
| 8 | Split the work, combine after the join | private output slots, why this needs no mutex |

Every panel is live: hover any identifier in the source to read its current
value, address and size; click a line to jump the timeline there; arrow keys
step, space plays.

**Byte view** (the `bytes` button) drops every box down to its raw bytes,
lowest address first. It is where little-endian stops being a word — 25 is
`19 00 00 00` — and where struct padding becomes visible: `Args { int; int *; }`
is 16 bytes, not 12, because the pointer has to start on an 8-byte boundary.
The layout engine applies real ABI alignment rules, so the sizes it prints are
the ones `sizeof` gives you.

Every program carries a one-sentence **rule of thumb** in the sidebar — the
practice to take away, not the mechanism.

**Blocking joins** are modelled, not narrated. `pthread_join` sets main to
`blocked` and its next op is gated on the joined thread actually finishing, so
the scheduler cannot pick main until then. You watch the CPU go to the workers
while main waits.

Programs 6, 7 and 8 are **simulated, not scripted**. Each thread is a lane of
ops, and a seeded scheduler interleaves the lanes while keeping each lane in
order — the same constraint a real scheduler obeys. `shuffle schedule` picks a
new seed, so the race genuinely prints 11 on some runs and 12 on others, and
the `&i` program genuinely reads a different id depending on when a thread
gets the CPU.

Program 8 is the control case: shuffle it as much as you like and it prints
210 every time, because no two threads write the same box. Race and no-race
run through the same scheduler, which is the only way the contrast means
anything.

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

## The C tutorial

[`c/`](c/) holds 19 numbered steps, one idea each, compiled with ASan and
UBSan so a bad pointer reports a file and a line rather than a bare segfault.

```bash
cd c && make 05      # build and run one step
make limpar          # remove the binaries
```

Roughly half are wrong on purpose and the following step fixes them, so
`make 09`, `make 12` and `make 14` are *supposed* to abort under the
sanitizer. Start at [`c/00 - COMECE AQUI.md`](c/00%20-%20COMECE%20AQUI.md);
[`c/memoria.md`](c/memoria.md) is the stack/heap diagram the pointer steps
refer back to, and `c/inspetor.html` opens straight from the filesystem.

Written in Portuguese, unlike the app.

## Stack

Next 16 (App Router, Turbopack) · React 19 · TypeScript · Tailwind 4 ·
motion · zustand · lucide-react.

Theming is a `dark` class on `<html>`, set before first paint by a blocking
snippet in `layout.tsx`, with every colour in the app coming from a CSS
variable. `next-themes` was tried first and removed: it renders its script
inside `<body>`, which React 19 rejects and which broke hydration.
