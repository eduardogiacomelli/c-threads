/* ============================================================================
 * STEP 25 — a union: one piece of memory, several ways to read it.
 *
 * A struct puts its members side by side. A union puts them ON TOP of each
 * other: every member starts at the same address, and the union is as big as
 * its largest member. Writing one member changes all the others, because
 * there is only one set of bytes.
 *
 * This is the cleanest way to see that a type is nothing but a reading
 * instruction (passo-05).
 *
 *     Ctrl+Shift+B      (or: make 25)
 * ========================================================================= */

#include <stdio.h>
#include <string.h>
#include <stdint.h>     /* uint32_t, uint8_t — exact-width types */

/* Four bytes, looked at three ways. */
union Word {
    uint32_t as_int;
    float    as_float;
    uint8_t  as_bytes[4];
};

int main(void)
{
    union Word w;

    printf("sizeof(union Word) = %zu  <- the largest member, not the sum\n",
           sizeof(union Word));
    printf("every member starts at the same address:\n");
    printf("  &w.as_int   = %p\n", (void *) &w.as_int);
    printf("  &w.as_float = %p\n", (void *) &w.as_float);
    printf("  &w.as_bytes = %p\n\n", (void *) w.as_bytes);

    /* Write as an integer, read as bytes. */
    w.as_int = 25;
    printf("w.as_int = 25\n  bytes: ");
    for (int i = 0; i < 4; i++) printf("%02x ", w.as_bytes[i]);
    printf("  <- little-endian: the 19 comes first\n\n");

    /* Now write a float into the SAME bytes. */
    w.as_float = 1.0f;
    printf("w.as_float = 1.0f\n  bytes: ");
    for (int i = 0; i < 4; i++) printf("%02x ", w.as_bytes[i]);
    printf("\n  as_int now reads %u\n", w.as_int);
    printf("  Nothing was converted. 1.0f IS the bit pattern 0x3f800000,\n");
    printf("  and reading those bits as an integer gives 1065353216.\n\n");

    /* An endianness check that costs nothing. */
    w.as_int = 1;
    printf("endianness: first byte of the integer 1 is %u -> %s-endian\n",
           w.as_bytes[0], w.as_bytes[0] == 1 ? "little" : "big");
    printf("(x86-64 and ARM are little-endian; network byte order is big)\n\n");

    /* ----------------------------------------------------------------- */
    /* THE HONEST WAY TO REINTERPRET BYTES.
     *
     * Reading a union member you did not write is well defined in C (unlike
     * C++), but the portable, always-correct, never-argued-about way is
     * memcpy. The compiler recognises it and emits the same instructions —
     * you pay nothing for the clarity. */
    float  f = 3.14159f;
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    printf("memcpy: 3.14159f is 0x%08x\n", bits);

    float back;
    memcpy(&back, &bits, sizeof(back));
    printf("        and back again: %.5f\n\n", back);

    /* THE WAY THAT IS ACTUALLY WRONG, and which looks the most natural:
     *
     *     uint32_t bad = *(uint32_t *) &f;
     *
     * That is a strict-aliasing violation. The compiler is allowed to assume
     * a float* and a uint32_t* never point at the same object, so with
     * optimisation on it may reorder or discard the write. It usually
     * "works" at -O0 and breaks at -O2, which is the worst way to find out.
     * Use memcpy or a union; never the pointer cast. */

    /* ----------------------------------------------------------------- */
    /* WHERE UNIONS EARN THEIR KEEP: a tagged value.
     *
     * The union saves space; the tag says which member is live. Keeping the
     * two together in a struct is the whole pattern — a union on its own
     * cannot tell you what it is currently holding. */
    struct Value {
        enum { IS_INT, IS_DOUBLE, IS_TEXT } kind;
        union {
            int    i;
            double d;
            char   s[16];
        } data;
    };

    struct Value values[3];
    values[0].kind = IS_INT;    values[0].data.i = 42;
    values[1].kind = IS_DOUBLE; values[1].data.d = 2.5;
    values[2].kind = IS_TEXT;   snprintf(values[2].data.s,
                                         sizeof(values[2].data.s), "hello");

    printf("tagged union, sizeof = %zu bytes each:\n", sizeof(struct Value));
    for (int i = 0; i < 3; i++) {
        printf("  ");
        switch (values[i].kind) {
        case IS_INT:    printf("int    %d\n", values[i].data.i); break;
        case IS_DOUBLE: printf("double %.2f\n", values[i].data.d); break;
        case IS_TEXT:   printf("text   \"%s\"\n", values[i].data.s); break;
        }
    }

    return 0;
}

/* ============================================================================
 * STRUCT vs UNION, IN ONE PICTURE
 *
 *   struct { int a; int b; }        union { int a; int b; }
 *
 *   0x1000 [ a ]                    0x1000 [ a ]
 *   0x1004 [ b ]                    0x1000 [ b ]   same address
 *   sizeof = 8                      sizeof = 4
 *
 * READING A MEMBER YOU DID NOT WRITE
 *
 *   allowed in C (this is called type punning, and it is why the endianness
 *   check above is legal), but you get whatever the bytes happen to mean.
 *   The union does not remember which member was last written — that is what
 *   the tag is for.
 *
 * THE THREE WAYS, RANKED
 *
 *   1. memcpy            always correct, no aliasing question, free
 *   2. union member      correct in C, idiomatic for endianness checks
 *   3. *(T *) &x         strict-aliasing violation; works until -O2
 *
 * EXPERIMENTE:
 *
 *  1. Add a `double as_double` to union Word and print sizeof again. It
 *     jumps to 8, and now as_bytes[4] only covers half of it — a union is
 *     only as useful as your discipline about which member is live.
 *
 *  2. Set w.as_bytes[] to { 0x00, 0x00, 0x80, 0x3f } by hand and print
 *     as_float. You should get 1.0. You just wrote a float without ever
 *     naming a float.
 *
 *  3. In the tagged union, set kind to IS_TEXT but write to data.i, then
 *     print it as text. Nothing stops you: the tag is a promise you make to
 *     yourself, not a check the language performs.
 *
 *  4. Print sizeof(struct Value) and work out where the padding went
 *     (passo-16's diagram, and the byte view in the app). The double forces
 *     8-byte alignment on the whole union.
 *
 * -> passo-26
 * ========================================================================= */
