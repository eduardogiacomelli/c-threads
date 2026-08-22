/* ============================================================================
 * STEP 29 - WRONG ON PURPOSE. Macros are text, and text does not know maths.
 *
 * The preprocessor does not parse C. It pastes tokens. Every bug below comes
 * from forgetting that one fact. Four of the five compile in total silence;
 * gcc catches exactly one of them, and the footer says which.
 *
 *     Ctrl+Shift+B      (or: make 29)
 *
 * Predict each answer before you run it. Getting them wrong is the point.
 * ========================================================================= */

#include <stdio.h>

/* BUG 1: no parentheses around the parameter. */
#define SQUARE(x) x * x

/* BUG 2: no parentheses around the whole expansion. */
#define DOUBLE(x) (x) + (x)

/* BUG 3: the parameter appears twice, so the argument is evaluated twice. */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* BUG 4: two statements with no braces, used after an if. */
#define RESET(p, q) p = 0; q = 0

int main(void)
{
    /* ---------------------------------------------------------------- 1 */
    /* SQUARE(1 + 2) is pasted as  1 + 2 * 1 + 2, which is 1 + 2 + 2 = 5.
     * Precedence applies AFTER substitution, and by then your parentheses
     * are gone because you never wrote any. */
    printf("SQUARE(3)     = %d   (correct by luck)\n", SQUARE(3));
    printf("SQUARE(1 + 2) = %d   (expected 9)\n", SQUARE(1 + 2));

    /* ---------------------------------------------------------------- 2 */
    /* DOUBLE(5) expands to (5) + (5), fine on its own. But in a larger
     * expression: 10 * DOUBLE(5) becomes 10 * (5) + (5) = 55.
     * The parameters were protected; the result was not. */
    printf("\nDOUBLE(5)      = %d\n", DOUBLE(5));
    printf("10 * DOUBLE(5) = %d   (expected 100)\n", 10 * DOUBLE(5));

    /* ---------------------------------------------------------------- 3 */
    /* MAX looks correct, and its parentheses are right. The problem is that
     * `a` appears twice in the expansion, so an argument with a side effect
     * happens twice:
     *
     *     ((i++) > (j) ? (i++) : (j))
     *
     * i is incremented once by the comparison and again by the branch. */
    int i = 5, j = 3;
    printf("\nbefore: i=%d j=%d\n", i, j);
    int biggest = MAX(i++, j);
    printf("MAX(i++, j) = %d, and i is now %d   (expected 5, and i == 6)\n",
           biggest, i);

    /* ---------------------------------------------------------------- 4 */
    /* RESET expands to two statements. Only the first belongs to the if.
     *
     *     if (flag) a = 0; b = 0;
     *
     * The `b = 0;` runs unconditionally. Indentation lies; the semicolon
     * decides. There is no else here, but add one and it will not compile,
     * with an error that points nowhere useful. */
    int a = 1, b = 1;
    int flag = 0;
    if (flag)
        RESET(a, b);
    printf("\nflag was 0, so nothing should change: a=%d b=%d\n", a, b);
    printf("b was reset anyway, because the macro was two statements.\n");

    /* ---------------------------------------------------------------- 5 */
    /* And the classic that is not even a function-like macro: */
#define WIDTH  10 + 2
    int area = WIDTH * 3;
    printf("\n#define WIDTH 10 + 2, so WIDTH * 3 = %d   (expected 36)\n", area);

    return 0;
}

/* ============================================================================
 * WHAT THE PREPROCESSOR ACTUALLY DID
 *
 * See it for yourself, this is the most useful debugging command for macros:
 *
 *     cpp passo-29-macros-erradas.c | tail -40
 *
 * or for a single expression, without the noise of stdio.h:
 *
 *     echo '#define SQUARE(x) x * x
 *     int v = SQUARE(1 + 2);' | cpp -P
 *
 *     -> int v = 1 + 2 * 1 + 2;
 *
 * There it is. Nothing clever happened, and nothing was calculated. Tokens
 * were pasted where the name used to be.
 *
 * THE FIVE RULES, EARNED
 *
 *   1. Wrap every parameter:            #define SQUARE(x) ((x) * (x))
 *   2. Wrap the whole body too.
 *   3. Never use a parameter twice if the argument might have a side effect.
 *   4. Wrap multi-statement macros in do { ... } while (0).
 *   5. Prefer a `static inline` function. It obeys precedence, evaluates
 *      each argument once, type-checks, and is just as fast.
 *
 * Rule 5 is the real one. In C99 and later there is almost no reason for a
 * function-like macro. The exceptions are things a function genuinely cannot
 * do: capture __FILE__ and __LINE__ at the call site, stringify a token, or
 * build a name with ##. All of those are in passo-30.
 *
 * WHY ALMOST NO WARNINGS
 *
 * By the time the compiler sees the code, the macro is gone. It sees
 * `1 + 2 * 1 + 2`, a perfectly good expression that means exactly what it
 * says. There is nothing left to warn about, and the sanitizers see even
 * less, which puts bugs 1, 2, 3 and 5 in the same category as passo-23: only
 * you can catch them.
 *
 * The exception is bug 4. This file produces exactly one warning:
 *
 *     warning: macro expands to multiple statements
 *              [-Wmultistatement-macros]
 *
 * gcc added that check because the bug was common enough to be worth special
 * casing. It fires only when the macro is used after an if, for or while,
 * which is the only place the bug bites. One warning out of five problems is
 * a fair summary of how much help you get with macros.
 *
 * EXPERIMENTE:
 *
 *  1. Run each of the five through `cpp -P` as shown above and read the
 *     expansion. Match it to the wrong answer.
 *
 *  2. Fix SQUARE to ((x) * (x)) and rerun. Then call it as SQUARE(i++) and
 *     watch it still be wrong: parentheses do not fix double evaluation.
 *
 *  3. Find the -Wmultistatement-macros warning in the panel, then add an
 *     `else` after the `if (flag) RESET(a, b);`. Now it is a hard error:
 *     "else without a previous if", which is true and unhelpful until you
 *     know the macro put an extra statement in between.
 *
 *  4. Turn MAX into `static inline int max(int a, int b)` and call it with
 *     i++. One increment, correct answer, no parentheses needed anywhere.
 *
 * -> passo-30, the same tools used properly
 * ========================================================================= */
