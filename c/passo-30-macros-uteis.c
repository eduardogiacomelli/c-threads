/* ============================================================================
 * STEP 30 - the preprocessor used for what only it can do.
 *
 * passo-29 said: prefer a function. That stands. This file is the short list
 * of jobs a function genuinely cannot do, because they need information that
 * exists only at the call site, before compilation.
 *
 *     Ctrl+Shift+B      (or: make 30)
 *     gcc -DDEBUG ...   to switch the debug build on (experiment 2)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* ------------------------------------------------------------------ 1 --
 * CAPTURING THE CALL SITE.
 *
 * A function can never know where it was called from. __FILE__, __LINE__ and
 * __func__ are substituted at the point of use, so a macro can hand them to
 * the function that does the real work. This is the only reason logging
 * macros exist.
 *
 * The `do { } while (0)` wrapper is rule 4 from passo-29: it makes the macro
 * a single statement, so `if (x) LOG("hi"); else ...` compiles. The trailing
 * semicolon at the call site then belongs to the while, which is why there
 * is no semicolon after the closing brace here.
 *
 * `##__VA_ARGS__` is a gcc extension that eats the comma when the variadic
 * part is empty, so LOG("plain") works as well as LOG("%d", x). */
#define LOG(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%d %s] " fmt "\n", \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

/* ------------------------------------------------------------------ 2 --
 * CONDITIONAL COMPILATION.
 *
 * The disabled version is not "turned off at runtime", it is deleted before
 * the compiler sees it. Zero cost, and the arguments are not evaluated,
 * which is a trap of its own: never put a side effect inside DBG(). */
#ifdef DEBUG
#  define DBG(fmt, ...) LOG("debug: " fmt, ##__VA_ARGS__)
#else
#  define DBG(fmt, ...) ((void) 0)
#endif

/* ------------------------------------------------------------------ 3 --
 * STRINGIFY (#) AND PASTE (##).
 *
 * # turns a macro argument into a string literal, so you can print the
 * expression and its value together. There is no way to do this in a
 * function: by then the expression is just a number. */
#define SHOW_INT(expr)    printf("  %-22s = %d\n", #expr, (expr))
#define SHOW_SIZE(type)   printf("  sizeof(%-14s) = %zu\n", #type, sizeof(type))

/* ## glues two tokens into one identifier before compilation. */
#define MAKE_GETTER(field) \
    int get_##field(void) { return field; }

static int width = 7;
MAKE_GETTER(width)          /* defines int get_width(void) */

/* ------------------------------------------------------------------ 4 --
 * A COMPILE-TIME CHECK.
 *
 * C11 gives you static_assert, which fails the BUILD rather than the run.
 * Use it to pin down assumptions the rest of the code depends on. */
#include <assert.h>
static_assert(sizeof(int) == 4, "this code assumes 32-bit int");
static_assert(sizeof(void *) == 8, "this code assumes 64-bit pointers");

/* ------------------------------------------------------------------ 5 --
 * THE ONE YOU SHOULD REACH FOR FIRST.
 *
 * static inline: obeys precedence, evaluates each argument exactly once,
 * type-checks, can be stepped through in gdb, and compiles to the same
 * instructions as the macro. `static` keeps it private to this file
 * (passo-19), `inline` invites the compiler to skip the call. */
static inline int max_int(int a, int b)
{
    return a > b ? a : b;
}

/* Array length. This one MUST be a macro: a function would receive a decayed
 * pointer and sizeof would give 8 (passo-10). It only works in the scope
 * that declared the array, and that limitation is inherent, not a defect. */
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

int main(void)
{
    printf("1. call-site capture (look at stderr):\n");
    LOG("starting up");
    LOG("two values: %d and %s", 42, "text");

    printf("\n2. conditional compilation:\n");
    DBG("this line only exists in a -DDEBUG build");
#ifdef DEBUG
    printf("  built WITH -DDEBUG\n");
#else
    printf("  built without -DDEBUG, so DBG expanded to nothing\n");
#endif

    printf("\n3. stringify, so the label cannot drift from the value:\n");
    int items = 5;
    SHOW_INT(items);
    SHOW_INT(items * 2 + 1);
    SHOW_INT(max_int(3, 9));

    printf("\n   sizes, with the type name printed by the macro:\n");
    SHOW_SIZE(char);
    SHOW_SIZE(int);
    SHOW_SIZE(long);
    SHOW_SIZE(void *);

    printf("\n4. token pasting made get_width() out of thin air: %d\n",
           get_width());

    printf("\n5. static inline, evaluated once:\n");
    int i = 5;
    /* The increment gets its own statement on purpose. Writing
     *
     *     printf("...", max_int(i++, 3), i);
     *
     * reads i and modifies it in the same expression, and the order in which
     * arguments are evaluated is unspecified, so the result is undefined
     * behaviour. gcc catches it with -Wsequence-point. Separating the two
     * statements gives the increment a sequence point to finish at. */
    int biggest = max_int(i++, 3);
    printf("  max_int(i++, 3) = %d, and i is now %d  (compare passo-29)\n",
           biggest, i);

    int values[] = { 2, 4, 6, 8, 10 };
    printf("  ARRAY_LEN(values) = %zu\n", ARRAY_LEN(values));

    /* assert() is itself a macro that uses __FILE__ and __LINE__, and
     * -DNDEBUG deletes every one of them. That is why an assert must never
     * contain code the program needs: in a release build it is gone. */
    assert(ARRAY_LEN(values) == 5);
    printf("\n  assert passed. Rebuild with -DNDEBUG and it is not even\n");
    printf("  compiled in, so never write assert(x = compute()).\n");

    return 0;
}

/* ============================================================================
 * WHEN A MACRO IS THE RIGHT ANSWER
 *
 *   yes   capturing __FILE__ / __LINE__ / __func__ at the call site
 *   yes   stringify (#) and token pasting (##)
 *   yes   conditional compilation, including deleting code entirely
 *   yes   ARRAY_LEN, because a function cannot see the array type
 *   no    everything else. Write static inline.
 *
 * THE do { } while (0) IDIOM
 *
 * Compare the three ways to write a multi-statement macro:
 *
 *     #define R(a,b) a = 0; b = 0            breaks after if (passo-29)
 *     #define R(a,b) { a = 0; b = 0; }       breaks before else, because
 *                                            the call site's ; ends the if
 *     #define R(a,b) do { a=0; b=0; } while (0)   works everywhere
 *
 * The third is one statement that needs a terminating semicolon, which is
 * exactly how a function call behaves. That is the whole trick.
 *
 * READING YOUR OWN MACROS
 *
 *     cpp -P passo-30-macros-uteis.c | grep -A3 get_width
 *     gcc -E passo-30-macros-uteis.c | tail -60
 *
 * When a macro misbehaves, look at the expansion before you look at anything
 * else. It is always the fastest route.
 *
 * EXPERIMENTE:
 *
 *  1. Move a LOG() call to a different line and rerun. The line number
 *     follows it. Wrap the same fprintf in a function and it cannot.
 *
 *  2. Rebuild with debug on, from the terminal:
 *
 *         gcc -std=gnu17 -Wall -Wextra -g -DDEBUG \
 *             passo-30-macros-uteis.c -o /tmp/dbg && /tmp/dbg
 *
 *     The DBG line appears. Confirm it is really deleted in the normal
 *     build: `cpp passo-30-macros-uteis.c | grep -c "debug:"`.
 *
 *  3. Break a static_assert on purpose: change it to sizeof(int) == 8. The
 *     BUILD fails with your message. A check that costs nothing at runtime
 *     and cannot be forgotten.
 *
 *  4. Write `#define SWAP(a, b, T) do { T t = a; a = b; b = t; } while (0)`
 *     and use it. Then ask why passo-22 needed a size argument and this does
 *     not: the macro is expanded where the types are still known.
 *
 *  5. Add `-DNDEBUG` and confirm with `cpp` that the assert vanished.
 *
 *  6. Put the increment back inside the printf call:
 *
 *         printf("...", max_int(i++, 3), i);
 *
 *     gcc warns "operation on 'i' may be undefined [-Wsequence-point]". The
 *     order in which a function's arguments are evaluated is unspecified, so
 *     reading and modifying the same variable in one call is undefined
 *     behaviour, macro or no macro. This is a different bug from passo-29's
 *     double evaluation, and it bites plain function calls too.
 *
 * -> passo-31
 * ========================================================================= */
