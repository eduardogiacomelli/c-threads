/* ============================================================================
 * STEP 3 - WRONG ON PURPOSE. Arithmetic with mixed types.
 *
 * Run it BEFORE reading the explanation. Look at the wrong output with your
 * own eyes, and notice that gcc warned you (check the problems panel).
 *
 *     Ctrl+Shift+B      (or: make 03)
 *
 * This program COMPILES and RUNS. C allows it. That is what makes the bug
 * dangerous.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int total = 7;
    int people = 2;

    /* BUG 1 - dividing two ints is INTEGER division.
     * There is no 3.5 here: the result is 3, and the 0.5 is thrown away
     * BEFORE it ever reaches a double. Declaring the destination as double
     * saves nothing, because the arithmetic already happened. */
    double average = total / people;
    printf("average of %d across %d people: %f\n", total, people, average);

    /* BUG 2 - printf does not know the types of what you passed. It TRUSTS
     * the template: %d means "go and fetch an int from the place ints are
     * delivered". You delivered a double, which travels by another route.
     * printf takes whatever is sitting in that place and prints it. */
    double price = 19.9;
    printf("price with %%d: %d      <- garbage\n", price);

    /* BUG 3 - the reverse: %f fetches a double, and you handed over an int.
     * Look closely at the number that comes out. It is not random: it is the
     * `price` from the line above, still sitting where %f reads from. The
     * footer explains it, and it is more interesting than "got garbage". */
    int quantity = 3;
    printf("quantity with %%f: %f  <- garbage\n", quantity);

    /* BUG 4 - the same as BUG 1, hidden inside a larger expression.
     * (9/5) becomes 1, not 1.8. The formula is right; the arithmetic is not. */
    int celsius = 100;
    double fahrenheit = celsius * (9 / 5) + 32;
    printf("\n100 C in F: %.1f   (should be 212.0)\n", fahrenheit);

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 * Two rules, and all four wrong lines follow from them:
 *
 * 1. THE TYPE OF THE RESULT COMES FROM THE OPERANDS, NOT THE DESTINATION.
 *
 *        int / int  ->  int      (7/2 is 3, the remainder disappears)
 *
 *    The compiler computes 3 and only THEN converts it to double, giving
 *    3.0. The fractional part never existed.
 *
 * 2. printf DOES NOT KNOW WHAT YOU PASSED.
 *
 *    The arguments arrive with no labels at all. The template is the only
 *    instruction for where to fetch them and how to read them. And here a
 *    machine detail explains the strange output: on x86-64, integers and
 *    floating point travel by DIFFERENT ROUTES, in separate registers.
 *
 *        integer deliveries:        [ ? ][ ? ]        <- %d reads here
 *        floating point deliveries: [ 19.9 ]          <- %f reads here
 *
 *    In BUG 2 you delivered 19.9 by the floating point route and told %d to
 *    read from the integer route: it read whatever was left over there, which
 *    is that large meaningless number.
 *
 *    In BUG 3 you delivered an int and told %f to read the floating point
 *    route: it found the 19.9 still sitting there from the previous printf.
 *    That is why a quantity of "3" printed as 19.900000.
 *
 *    Not random, not a corrupted number. The wrong value, read from the wrong
 *    place, perfectly deterministically, which is exactly why this bug fools
 *    people: it looks stable.
 *
 * gcc WARNED about both printf cases (-Wformat, switched on by -Wall).
 * In C a warning is not noise: it is the compiler seeing the bug first.
 *
 * EXPERIMENTE:
 *
 *  1. Read the compiler output in the panel below. Find the two lines saying
 *     "format '%d' expects argument of type 'int', but argument 2 has type
 *     'double'". It told you exactly where.
 *
 *  2. Run it twice. The output is the SAME, and BUG 3's 19.900000 is still
 *     there. Now swap the order: put BUG 3's printf BEFORE BUG 2's. The
 *     number changes, because there is no longer a freshly delivered double
 *     for %f to find. The bug depends on the code around it.
 *
 *  3. Change `int people = 2;` to `double people = 2;` and run. The average
 *     fixes itself. Why? Because now one operand is a double, and C promotes
 *     the other before dividing. That is the key to step 04.
 *
 * -> passo-04, the fix
 * ========================================================================= */
