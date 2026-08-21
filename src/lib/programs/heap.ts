import { Machine, hex, lineFinder } from "../machine";
import type { Program } from "../types";

const SRC = `#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int  id;
    int *data;
} Args;

int main(void)
{
    int data[3] = {10, 20, 30};

    Args *a = malloc(sizeof(Args));
    if (a == NULL) return 1;

    a->id   = 7;
    a->data = data;

    printf("id=%d first=%d\\n", a->id, a->data[0]);

    free(a);

    printf("%d\\n", a->id);      /* use after free */
    return 0;
}
`;

export const heap: Program = {
  id: "heap",
  title: "A struct on the heap",
  blurb: "malloc, -> versus ., and what free actually invalidates.",
  origin: "c-do-zero / passo-15 e 16",
  concepts: ["malloc", "free", "struct", "->", "use-after-free"],
  takeaway:
    "One `free` per `malloc`, `NULL` the pointer straight after, and write down who frees it. Test `malloc` against NULL even though it \"never\" fails.",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();

    m.pushFrame("main");
    const data = m.declareArray("data", "int", ["10", "20", "30"]);
    m.snap(at("int data[3]"), "Three ints on the stack of main. These live until main returns.");

    const block = m.malloc("malloc(sizeof(Args))", [
      { name: "id", type: "int", value: "?", size: 4, kind: "field", tag: "uninitialised" },
      { name: "data", type: "int *", value: "?", size: 8, kind: "pointer", tag: "uninitialised" },
    ]);
    const a = m.declare({
      name: "a",
      type: "Args *",
      value: hex(block.addr),
      size: 8,
      kind: "pointer",
    });
    a.points = block.slots[0].id;
    m.snap(
      at("Args *a = malloc"),
      "malloc carved 12 bytes out of the heap and handed back the address. `a` is on the stack; what it points at is not. That block belongs to nobody's frame — which is exactly why it can outlive one.",
    );

    m.snap(
      at("if (a == NULL)"),
      "Always test the return. On Linux malloc almost never fails, and 'almost never' with a pointer is a crash at address zero.",
    );

    m.read(a);
    m.write(block.slots[0], "7", { tag: undefined });
    m.snap(
      at("a->id"),
      "`a->id = 7` is shorthand for `(*a).id`: follow the pointer, then pick the field. The arrow exists because the parenthesised form is unbearable to type.",
    );

    m.read(a);
    m.aim(block.slots[1], data[0]);
    block.slots[1].tag = undefined;
    m.snap(
      at("a->data = data"),
      "A struct field can itself be a pointer. `data` decayed to the address of its first element — the struct now reaches back into main's stack.",
      { tone: "warn" },
    );

    m.read(a);
    m.read(block.slots[0]);
    m.read(block.slots[1]);
    m.read(data[0]);
    m.print("id=7 first=10");
    m.snap(
      at("printf("),
      "Three hops to print one number: a -> the block -> the data pointer -> the array cell. Every arrow on screen is one of them.",
    );

    m.free(block);
    m.snap(
      at("free(a)"),
      "free returns the block. `a` still holds the same number — the pointer was not changed, and C will not change it for you. This is why the habit is `free(a); a = NULL;`.",
      { tone: "warn" },
    );

    m.read(a);
    m.read(block.slots[0]);
    m.snap(
      at("use after free"),
      "Reading through `a` after free. AddressSanitizer: heap-use-after-free, and it prints three stacks — where it was allocated, where it was freed, and here.",
      { tone: "error" },
    );

    return m.done();
  },
};
