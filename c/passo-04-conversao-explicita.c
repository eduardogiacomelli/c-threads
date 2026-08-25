/* ============================================================================
 * STEP 4 - the fix for step 03: casts and the right format specifier.
 *
 *     Ctrl+Shift+B      (or: make 04)
 *
 * Put this next to passo-03. The changes are small.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int total = 7;
    int people = 2;

    /* FIX 1 - (double) is a CAST: "treat this value as a double, here, in
     * this expression". It does not change `total`, which stays an int; it
     * produces a temporary double value just for this calculation.
     *
     * With one double operand, C promotes the other automatically and the
     * division becomes floating point. Converting ONE of the two is enough. */
    double average = (double) total / people;
    printf("average of %d across %d people: %.2f\n", total, people, average);

    /* %.2f is floating point with 2 digits after the point. The ".2" rounds
     * only the DISPLAY; the stored value keeps its full precision. */

    /* FIX 2 - the template has to match what you pass.
     * Memorise these, they are 95% of the use:
     *
     *     %d   int
     *     %ld  long
     *     %f   double  (and float too: a float is promoted to double when
     *                   passed as a variadic argument)
     *     %c   char
     *     %s   string, meaning the address of a char (step 11)
     *     %p   any address (step 05)
     *     %zu  size_t, what sizeof gives you
     */
    double price = 19.9;
    int quantity = 3;
    printf("price: %.2f   quantity: %d\n", price, quantity);

    /* Do you actually want a double printed as an integer? Say so explicitly
     * with a cast. Then it is not garbage, it is truncation, and the code
     * says it was deliberate. 19.9 becomes 19: it cuts, it does not round. */
    printf("price truncated: %d\n", (int) price);

    /* FIX 3 - write the constant as a double and the whole expression turns
     * into one. 9.0/5 is 1.8. No cast needed: the `.0` does it. */
    int celsius = 100;
    double fahrenheit = celsius * (9.0 / 5.0) + 32;
    printf("\n100 C in F: %.1f\n", fahrenheit);

    /* Compile this file and check the panel: zero warnings. That is the
     * target for everything you write. */
    return 0;
}

/* ============================================================================
 * THE RULE, IN ONE SENTENCE
 *
 *   If you want a result with a fractional part, AT LEAST ONE operand has to
 *   have one, whether through a cast `(double)x` or a literal `9.0`.
 *
 * And the second:
 *
 *   A printf template is a promise. Break the promise, read garbage.
 *
 * EXPERIMENTE:
 *
 *  1. Change `(double) total / people` to `(double) (total / people)`.
 *     It goes back to 3.00. Why? The parentheses make the integer division
 *     happen FIRST, and the cast then converts a 3 that has already lost the
 *     remainder. Where the cast goes matters more than whether it is there.
 *
 *  2. Print `(int) -19.9`. You get -19, not -20. A cast to int truncates
 *     towards zero. For real rounding there is round(), in <math.h>, and
 *     then you need to compile with -lm.
 *
 *  3. Divide by zero with integers: `total / z` with `int z = 0;` (a
 *     variable, so the compiler cannot fold it). UBSan kills the program
 *     with "division by zero". Now do the same with doubles: `1.0 / 0.0`.
 *     It prints `inf` and carries on. Completely different rules for
 *     integers and for floating point.
 *
 * -> passo-05: pointers, from the beginning
 * ========================================================================= */
