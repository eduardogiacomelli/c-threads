import { Machine, lineFinder } from "../machine";
import type { Program } from "../types";

const SRC = `#include <stdio.h>

int main(void)
{
    int age   = 25;        /* a box that holds 25 */
    int other = 99;        /* a second box        */

    int *p = &age;         /* p holds the ADDRESS of age */

    printf("*p = %d\\n", *p);

    *p = 30;               /* write THROUGH the pointer  */
    printf("age = %d\\n", age);

    p = &other;            /* re-aim the pointer itself  */
    printf("*p = %d\\n", *p);

    p = NULL;
    printf("%d\\n", *p);    /* reading through NULL       */

    return 0;
}
`;

export const pointers: Program = {
  id: "pointers",
  title: "Address and value",
  blurb: "& and *, and the difference between writing through a pointer and re-aiming it.",
  origin: "c-do-zero / passo-05",
  concepts: ["&", "*", "NULL", "dereference"],
  takeaway:
    "`*p` reaches through the arrow; `p =` moves the arrow. Initialise every pointer, and check for NULL before dereferencing one you did not just set.",
  source: () => SRC,
  build() {
    const at = lineFinder(SRC);
    const m = new Machine();

    m.pushFrame("main");
    m.snap(at("int main"), "main starts. Its frame is the box factory: every local declared below lives here.");

    const age = m.declare({ name: "age", type: "int", value: "25" });
    m.snap(
      at("int age"),
      "`age` is 4 bytes at a fixed address. The name is for you and the compiler; the machine only knows the address.",
    );

    const other = m.declare({ name: "other", type: "int", value: "99" });
    m.snap(at("int other"), "A second box, 4 bytes further along.");

    const p = m.declare({
      name: "p",
      type: "int *",
      value: "",
      size: 8,
      kind: "pointer",
    });
    m.aim(p, age);
    m.snap(
      at("int *p"),
      "`p` is a box like any other - 8 bytes, because every address is 8 bytes on x86-64. What it happens to hold is the address of `age`.",
    );

    m.read(p);
    m.read(age);
    m.print("*p = 25");
    m.snap(
      at('printf("*p'),
      "`*p` means: go to the address stored in p, read an int there. Two lookups, not one.",
    );

    m.read(p);
    m.write(age, "30");
    m.snap(
      at("*p = 30"),
      "`*p = 30` followed the arrow and wrote at the far end. The arrow itself did not move - `age` changed.",
      { tone: "ok" },
    );

    m.read(age);
    m.print("age = 30");
    m.snap(at('printf("age'), "Reading `age` directly gives the same 30. One box, two ways in.");

    m.aim(p, other);
    m.snap(
      at("p = &other"),
      "`p = &other` changed the pointer itself. Nothing was written to `age` or `other` - only the arrow moved. Same asterisk-free line, opposite effect from the one above.",
      { tone: "warn" },
    );

    m.read(p);
    m.read(other);
    m.print("*p = 99");
    m.snap(at('printf("*p', at("p = &other")), "Now `*p` lands in `other`.");

    m.aim(p, null);
    m.snap(at("p = NULL"), "NULL is the agreed-on 'points at nothing' value: address zero.", {
      tone: "warn",
    });

    m.read(p);
    m.snap(
      at('printf("%d'),
      "Dereferencing NULL. The sanitizer stops the program here: runtime error: load of null pointer of type 'int'. Without it you would get a bare Segmentation fault.",
      { tone: "error" },
    );

    return m.done();
  },
};
