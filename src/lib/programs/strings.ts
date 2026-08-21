import { Machine, lineFinder } from "../machine";
import type { Program, Slot } from "../types";

const TEXT = "Ana Giacomelli";

const src = (mode: string) => `#include <stdio.h>
#include <string.h>

int main(void)
{
    char name[8] = "Ana";       /* 8 bytes reserved, 4 of them used */
    char tail[8] = "Silva";     /* the neighbour — keep an eye on it */

    size_t len = strlen(name);  /* walks to the '\\0'; not a stored field */
    printf("\\"%s\\": %zu chars in %zu bytes\\n", name, len, sizeof(name));

${
  mode === "unsafe"
    ? `    strcpy(name, "${TEXT}");
`
    : `    int wanted = snprintf(name, sizeof(name), "%s", "${TEXT}");
    if (wanted >= (int) sizeof(name))
        fprintf(stderr, "warning: name truncated\\n");
`
}
    printf("name = \\"%s\\"\\n", name);
    printf("tail = \\"%s\\"\\n", tail);
    return 0;
}
`;

/** How printf("%s") reads a run of bytes: forward until it finds a zero. */
function readCString(cells: Slot[], start: number) {
  let out = "";
  for (let i = start; i < cells.length; i++) {
    const ch = cells[i].value;
    if (ch === "'\\0'") return { text: out, terminated: true, at: i };
    out += ch.length === 3 ? ch[1] : ch.slice(1, -1);
  }
  return { text: out, terminated: false, at: -1 };
}

const lit = (c: string) => (c === "\0" ? "'\\0'" : `'${c}'`);

export const strings: Program = {
  id: "strings",
  title: "A string is a char array with one rule",
  blurb: "The '\\0' terminator, strlen walking to find it, and what a copy does without it.",
  origin: "c-do-zero / passo-11, 12 e 13",
  concepts: ["char[]", "'\\0' terminator", "strlen is O(n)", "buffer overflow", "snprintf"],
  modes: [
    { id: "unsafe", label: "strcpy", danger: true, hint: "no idea how big the destination is" },
    { id: "safe", label: "snprintf", hint: "told how big the destination is" },
  ],
  takeaway:
    "Budget N+1 bytes for N characters, and never call a write function that cannot be told the destination size. `snprintf(dst, sizeof dst, ...)` over `strcpy`; check the return for truncation.",
  source: src,
  build(mode) {
    const SRC = src(mode);
    const at = lineFinder(SRC);
    const m = new Machine();
    const unsafe = mode === "unsafe";

    m.pushFrame("main");
    m.snap(at("int main"), "Turn the byte view on for this one — it is the whole point of the program.");

    /* char name[8] = "Ana"; — the rest of the array is zero-filled */
    const name = m.declareArray(
      "name",
      "char",
      ["'A'", "'n'", "'a'", "'\\0'", "'\\0'", "'\\0'", "'\\0'", "'\\0'"],
      { elemSize: 1 },
    );
    name[3].tag = "terminator";
    m.snap(
      at("char name[8]"),
      'Eight boxes of one byte each. "Ana" is three characters, so it needs FOUR bytes: the text plus the \'\\0\' that marks the end. An initialiser shorter than the array zero-fills the rest.',
    );

    const tail = m.declareArray(
      "tail",
      "char",
      ["'S'", "'i'", "'l'", "'v'", "'a'", "'\\0'", "'\\0'", "'\\0'"],
      { elemSize: 1 },
    );
    tail[5].tag = "terminator";
    m.snap(
      at("char tail[8]"),
      "A second array, immediately after the first. The compiler chose to put it there; it is under no obligation to, which is why the same bug can wreck something different on another build.",
    );

    /* strlen: an actual loop, one step per byte examined */
    m.snap(
      at("size_t len"),
      "strlen(name) is a function call that loops. There is no stored length anywhere — watch it look for the zero.",
    );

    let count = 0;
    for (let i = 0; i < name.length; i++) {
      m.read(name[i]);
      const isNul = name[i].value === "'\\0'";
      if (!isNul) count++;
      m.snap(
        at("size_t len"),
        isNul
          ? `name[${i}] is '\\0'. Stop. Return ${count}. Four bytes were examined to answer a question about three characters — that is why calling strlen inside a loop condition is a classic performance bug.`
          : `name[${i}] is ${name[i].value}, not zero. Keep walking. Count so far: ${count}.`,
        { tone: isNul ? "ok" : "info" },
      );
      if (isNul) break;
    }

    const len = m.declare({ name: "len", type: "size_t", value: String(count), size: 8 });
    m.read(len);
    m.print(`"Ana": 3 chars in 8 bytes`);
    m.snap(
      at("%zu chars"),
      "strlen counts characters; sizeof counts the box. They answer different questions and beginners reach for the wrong one constantly.",
    );

    /* the copy, byte by byte */
    const region = [...name, ...tail];
    const bytes = [...TEXT.split(""), "\0"];
    const capacity = name.length;

    if (unsafe) {
      m.snap(
        at("strcpy(name"),
        `strcpy receives two addresses and nothing else. It cannot know \`name\` is 8 bytes — there is no length to pass and no way to ask. It will write all ${bytes.length} bytes of "${TEXT}" plus the terminator, starting here.`,
        { tone: "warn" },
      );

      for (let i = 0; i < bytes.length; i++) {
        const cell = region[i];
        const inside = i < capacity;
        m.write(cell, lit(bytes[i]));
        if (!inside) {
          cell.tone = "error";
          cell.tag = "not yours";
        } else {
          cell.tag = i === capacity - 1 ? "last byte that fits" : undefined;
        }
        m.snap(
          at("strcpy(name"),
          inside
            ? `Byte ${i + 1} of ${bytes.length}: ${lit(bytes[i])} into name[${i}]. Still inside the array.`
            : `Byte ${i + 1}: ${lit(bytes[i])} written ${i - capacity} bytes past the end of name — into tail[${i - capacity}]. Nothing stopped it. AddressSanitizer would abort here with stack-buffer-overflow.`,
          { tone: inside ? "info" : "error" },
        );
      }
    } else {
      m.snap(
        at("snprintf(name"),
        "snprintf is told the size of the destination, so it can stop. It writes at most 7 characters plus a terminator, and it ALWAYS terminates — which strncpy, the function that looks like the obvious choice, does not guarantee.",
        { tone: "ok" },
      );

      for (let i = 0; i < capacity; i++) {
        const last = i === capacity - 1;
        const ch = last ? "\0" : TEXT[i];
        m.write(region[i], lit(ch));
        region[i].tag = last ? "terminator, always written" : undefined;
        m.snap(
          at("snprintf(name"),
          last
            ? `Byte 8 is the '\\0'. snprintf spent the last byte of the buffer on the terminator instead of on a character — that is the trade, and it is the right one.`
            : `Byte ${i + 1} of 8: ${lit(ch)} into name[${i}].`,
          { tone: last ? "ok" : "info" },
        );
      }

      const wanted = m.declare({ name: "wanted", type: "int", value: String(TEXT.length) });
      m.snap(
        at("int wanted"),
        `snprintf returns ${TEXT.length}: the length it WANTED to write, not the length it wrote. That is the truncation detector, and it is the only reason you can tell this went wrong at all.`,
      );
      m.read(wanted);
      m.snap(
        at("wanted >="),
        `${TEXT.length} >= 8, so the text did not fit. Truncated data is still wrong data — but you know about it, which is the entire difference from the strcpy version.`,
        { tone: "warn" },
      );
      m.print("warning: name truncated");
    }

    /* what %s actually prints now */
    const shown = readCString(region, 0);
    const tailShown = readCString(region, capacity);
    m.print(`name = "${shown.text}"`);
    m.snap(
      at('printf("name'),
      unsafe
        ? `printf("%s", name) walks from name[0] until it meets a zero — and the zero is now at tail[${shown.at - capacity}], in the neighbour's memory. So it prints the whole of "${shown.text}" and the buffer looks like it worked. It did not: 7 bytes of that string are not in \`name\`.`
        : `printf prints "${shown.text}". Seven characters and the terminator, exactly what fits.`,
      { tone: unsafe ? "error" : "ok" },
    );

    m.print(`tail = "${tailShown.text}"`);
    m.snap(
      at('printf("tail'),
      unsafe
        ? `And \`tail\` is "${tailShown.text}". It used to be "Silva". No line in this program mentions \`tail\` after it was initialised.`
        : `\`tail\` is still "Silva" — untouched, because snprintf knew where to stop.`,
      { tone: unsafe ? "error" : "ok" },
    );

    return m.done();
  },
};
