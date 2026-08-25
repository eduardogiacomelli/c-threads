/* ============================================================================
 * STEP 18 - the fix for step 17: strtol and strtod, which can report failure.
 *
 * Challenge 2 asks explicitly for a "consistency test of the input
 * provided". This is that test.
 *
 *     make 18 ARGS="1e9 4"
 *     ./passo-18-validando-a-entrada abc 4
 *     ./passo-18-validando-a-entrada 1e9 0
 *     ./passo-18-validando-a-entrada 99999999999999999999 4
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* strtol, strtod */
#include <errno.h>      /* errno, ERANGE  */
#include <limits.h>     /* INT_MAX        */

/* The difference between atoi and strtol is that strtol HAS a way to tell
 * you what happened, through two channels:
 *
 *   - `end`, a pointer it makes point at the first character it could NOT
 *     read. If it points at the '\0', it read the whole string; if it points
 *     at the start, it read nothing;
 *   - `errno`, a system variable that becomes ERANGE if the number did not
 *     fit in the type.
 *
 * That is why it takes a `char **end`: it is step 07 (a function needs to
 * write into a variable of yours), except the variable is itself a pointer.
 * You pass the address of a `char *`, and it writes in there.
 *
 * This function returns 1 on success and 0 on failure, writing the result
 * through *out. Same pattern, and step 35 says more about why. */
int read_positive_int(const char *text, long *out)
{
    char *end;

    errno = 0;                                /* zero it BEFORE the call */
    long value = strtol(text, &end, 10);      /* 10 = base ten */

    if (end == text)        return 0;   /* read nothing at all: "abc"   */
    if (*end != '\0')       return 0;   /* trailing junk: "12abc"       */
    if (errno == ERANGE)    return 0;   /* did not fit in a long        */
    if (value <= 0)         return 0;   /* the assignment's own rule    */
    if (value > INT_MAX)    return 0;   /* will it become an int later? */

    *out = value;
    return 1;
}

/* Same idea with strtod, which understands scientific notation: the "1e9"
 * Challenge 1 requires. strtod returns a double; convert afterwards. */
int read_iterations(const char *text, long *out)
{
    char *end;

    errno = 0;
    double value = strtod(text, &end);

    if (end == text)      return 0;
    if (*end != '\0')     return 0;
    if (errno == ERANGE)  return 0;
    if (value < 1.0)      return 0;

    *out = (long) value;      /* 1e9 -> 1000000000 */
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <iterations> <threads>\n", argv[0]);
        fprintf(stderr, "  iterations: an integer, or scientific notation (1e9)\n");
        fprintf(stderr, "  threads:    a positive integer\n");
        return 1;
    }

    long iterations;
    long threads;

    /* Validate EACH argument separately, with a message that says which one
     * is wrong and what arrived. A bare "invalid input" is useless to whoever
     * is running the program, including you while testing. */
    if (!read_iterations(argv[1], &iterations)) {
        fprintf(stderr, "error: invalid iterations: \"%s\"\n", argv[1]);
        return 1;
    }
    if (!read_positive_int(argv[2], &threads)) {
        fprintf(stderr, "error: invalid threads: \"%s\"\n", argv[2]);
        return 1;
    }

    /* Rules that depend on both values come after both are valid. This is
     * where the question Exercise 7 asks belongs: does having more threads
     * than work make any sense? */
    if (threads > iterations) {
        fprintf(stderr, "error: %ld threads for %ld iterations makes no sense\n",
                threads, iterations);
        return 1;
    }

    printf("ok: %ld iterations, %ld threads\n", iterations, threads);
    printf("each thread takes %ld, and %ld are left over\n",
           iterations / threads, iterations % threads);

    /* That leftover is the detail Challenge 2 warns about in small print:
     * "remember to handle the case where the worksize does not divide
     * evenly by the number of threads". Run it with 10 and 3 and look. */

    return 0;
}

/* ============================================================================
 * THE PATTERN, IN THREE LINES
 *
 *     errno = 0;
 *     long v = strtol(text, &end, 10);
 *     ok = (end != text) && (*end == '\0') && (errno != ERANGE);
 *
 * The `&end` is step 07 applied to a pointer:
 *
 *     "1e9\0"
 *      ^   ^
 *      |   +-- if `end` stops here (at the \0), it read everything
 *      +------ if `end` stays here (at the start), it read nothing
 *
 * With strtol, "1e9" stops at the 'e', so `*end` is 'e', so it is rejected
 * and you know about it. With atoi the same input silently becomes 1. The
 * difference between the two is not the conversion, it is the report.
 *
 * THE VALIDATION CHECKLIST THE ASSIGNMENTS ASK FOR
 *
 *   [ ] argc is what you expect (remember argv[0])
 *   [ ] each argument converted completely, with nothing left over
 *   [ ] it did not overflow the type (ERANGE)
 *   [ ] it respects the assignment's rule (positive, non-zero)
 *   [ ] the combination makes sense (M > N? threads = 0?)
 *   [ ] a message on stderr saying WHICH argument and HOW to use it
 *   [ ] a non-zero return
 *
 * EXPERIMENTE:
 *
 *  1. Run the four cases from the header and read each message. Then run
 *     `./passo-18-validando-a-entrada 10 3` and look at the remainder: one
 *     element with no owner. Who processes it? Keep the question; it comes
 *     back in Exercise 6, Exercise 7 and Challenge 2.
 *
 *  2. Remove the `errno = 0;` and run with a huge number twice in a row.
 *     errno is not cleared for you: it holds the error from the last
 *     function that failed, possibly from somewhere else entirely. Step 35
 *     is the whole story.
 *
 *  3. Change strtol's base to 16 and pass "ff". You get 255. The base is a
 *     parameter, and base 0 makes it guess from the prefix (0x, 0).
 *
 *  4. Write the validation Challenge 2 asks for, threads and worksizetotal,
 *     and test it with empty input, negative, zero, text and an absurd
 *     number. Five ten-second tests that protect the grade.
 *
 * -> passo-19: when one file is not enough
 * ========================================================================= */
