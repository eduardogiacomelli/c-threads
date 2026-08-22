/* ============================================================================
 * STEP 23 — signed vs unsigned, the bug no sanitizer will catch.
 *
 * Everything so far had a tool watching it: ASan for memory, UBSan for
 * overflow. This one has nothing. Unsigned wraparound is not undefined
 * behaviour — it is DEFINED to wrap, so no tool considers it an error. The
 * compiler will warn if you let it, and that warning is the only help you get.
 *
 *     Ctrl+Shift+B      (or: make 23)
 *
 * Read the warnings in the panel before the output.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>
#include <stdint.h>     /* SIZE_MAX */

int main(void)
{
    int v[5] = { 10, 20, 30, 40, 50 };
    size_t n = sizeof(v) / sizeof(v[0]);

    /* ---------------------------------------------------------------- 1 */
    /* THE COMPARISON TRAP.
     *
     * When you compare a signed and an unsigned value of the same width, C
     * does not "notice" that one is negative. It applies the usual arithmetic
     * conversions: the SIGNED operand is converted to unsigned, and -1 becomes
     * the largest possible value.
     *
     *     (int) -1   ->  bits 11111111 11111111 11111111 11111111
     *     read as unsigned  ->  4294967295
     *
     * The bits never moved. Only the interpretation changed (passo-05: the
     * type is an instruction about how to read memory). */
    int i = -1;

    printf("i is %d and n is %zu\n", i, n);

    /* Written the way it appears in real code, with no cast in sight.
     * The compiler warns here: -Wsign-compare, part of -Wall. */
    if (i < n)
        printf("is i < n?  yes\n");
    else
        printf("is i < n?  NO   <- -1 is not less than 5\n");

    printf("because i as unsigned is %zu\n\n", (size_t) i);

    /* This is why a bounds check can pass a negative index straight through:
     *
     *     if (index < count)  ...  with index int and count size_t
     *
     * A negative index is not caught. It becomes enormous, fails the test,
     * and you conclude the check works — until the day the other branch
     * matters. */

    /* ---------------------------------------------------------------- 2 */
    /* THE REVERSE LOOP.
     *
     *     for (size_t k = n - 1; k >= 0; k--)
     *
     * `k >= 0` is ALWAYS true: an unsigned value cannot be negative. When k
     * is 0 and we decrement, it wraps to SIZE_MAX and the loop reads v[huge].
     *
     * The guard below is only here so this file terminates. Take it out and
     * the program runs until it segfaults. */
    printf("walking backwards with size_t:\n");
    int guard = 0;
    for (size_t k = n - 1; k >= 0; k--) {
        if (guard++ < 5)
            printf("  k=%zu -> v[k]=%d\n", k, v[k]);
        else {
            printf("  k=%zu  <- wrapped around; the condition k >= 0 can\n", k);
            printf("     never be false, so this loop does not end\n\n");
            break;
        }
    }

    /* THE FIX, and it is worth memorising as an idiom:
     *
     *     for (size_t k = n; k-- > 0; )
     *
     * `k-- > 0` tests the value BEFORE decrementing, so the body sees
     * n-1 .. 0 and the test fails cleanly at 0 without ever wrapping. The
     * empty third clause is not a typo — the decrement already happened. */
    printf("the idiom that works:\n");
    for (size_t k = n; k-- > 0; )
        printf("  k=%zu -> v[k]=%d\n", k, v[k]);

    /* ---------------------------------------------------------------- 3 */
    /* strlen RETURNS size_t, so arithmetic on it is unsigned. */
    const char *empty = "";
    size_t len = strlen(empty);

    printf("\nstrlen(\"\") = %zu\n", len);
    printf("strlen(\"\") - 1 = %zu   <- not -1\n", len - 1);
    printf("SIZE_MAX       = %zu\n", SIZE_MAX);

    /* So this innocent line indexes far outside the string:
     *     if (s[strlen(s) - 1] == '\n') ...
     * Trimming a trailing newline is the usual context, and an empty line is
     * exactly the input that reaches it. Check the length first. */
    if (len > 0 && empty[len - 1] == '\n')
        printf("ends with newline\n");
    else
        printf("guarded with len > 0 first, so nothing was read\n");

    return 0;
}

/* ============================================================================
 * THE RULES, IN ORDER OF HOW OFTEN THEY BITE
 *
 * 1. Mixing signed and unsigned in one comparison converts the signed side
 *    to unsigned. Negative becomes huge.
 *
 * 2. `size_t` is unsigned. Anything derived from `sizeof` or `strlen` is
 *    unsigned. Subtracting from it can never produce a negative number.
 *
 * 3. Unsigned wraparound is legal and silent. Signed overflow is undefined
 *    behaviour and UBSan catches it (passo-02, experiment 2). The safer-
 *    looking type is the one with no safety net.
 *
 * 4. The compiler sees both of these. This file produces two warnings, and
 *    each one names its trap exactly:
 *
 *      -Wsign-compare   "comparison of integer expressions of different
 *                        signedness"                      <- section 1
 *      -Wtype-limits    "comparison of unsigned expression in '>= 0' is
 *                        always true"                     <- section 2
 *
 *    Both come from -Wall -Wextra, which is on in this playground. Do not
 *    silence either with a cast until you know which side is wrong.
 *
 * WHAT TO ACTUALLY DO
 *
 *   - index and count with `size_t`, and use the `k-- > 0` idiom to go
 *     backwards;
 *   - if a value can legitimately be negative, keep it signed and compare it
 *     against a signed length: `(int) n`, cast once, deliberately;
 *   - never subtract from an unsigned value without first checking it is big
 *     enough.
 *
 * WHERE IT SHOWS UP IN PPD
 *
 * Dividing work between threads is subtraction on sizes:
 *
 *     size_t chunk = total / nthreads;
 *     size_t start = id * chunk;
 *     size_t end   = (id == nthreads - 1) ? total : start + chunk;
 *
 * If `nthreads` were 0 that `nthreads - 1` wraps to SIZE_MAX, and every
 * thread thinks it is the last one. This is why passo-18 validates the
 * arguments before any of this arithmetic happens.
 *
 * EXPERIMENTE:
 *
 *  1. Find both warnings in the panel and match each to its line. Then fix
 *     section 1 properly — should the count become signed, or the index
 *     unsigned? The answer depends on whether a negative index is meaningful
 *     here at all.
 *
 *  2. Remove the `guard` from the backwards loop. It runs, prints nonsense,
 *     and dies. Note that the sanitizer only complains once it reads far
 *     enough out of bounds to hit a red zone — the wrap itself is invisible
 *     to it.
 *
 *  3. Change `size_t n` to `int n` and put the naive backwards loop back.
 *     It works. Signed is not "safer" in general, but for a countdown it is
 *     the honest type.
 *
 *  4. Print `(unsigned char) -1` and `(char) 200`. Whether plain `char` is
 *     signed is implementation-defined; on x86 Linux it is signed, on ARM it
 *     is not. If you need a byte, write `unsigned char` (passo-22).
 *
 * -> passo-24
 * ========================================================================= */
