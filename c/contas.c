/* ============================================================================
 * contas.c - the IMPLEMENTATION. The real code lives here.
 * ========================================================================= */

/* A module includes its own header. It looks redundant, but it lets the
 * compiler check that the implementation matches what was promised: change a
 * return type here and forget to change it there, and it says so at once. */
#include "contas.h"
/* Quotes, not < >:
 *     "contas.h"   looks in this file's directory first  -> your code
 *     <stdio.h>    looks in the system directories       -> the library
 */

/* `static` on a GLOBAL variable means "visible only in this file". It has
 * nothing to do with the static inside a function (step 14). This is how a
 * module keeps state without exposing it: no other file can touch this
 * counter, because no other file knows it exists. */
static int calls = 0;

long sum_to(int n)
{
    calls++;

    if (n < 1)
        return 0;

    long total = 0;
    for (int i = 1; i <= n; i++)
        total += i;
    return total;
}

int is_even(int n)
{
    return n % 2 == 0;
}

int call_count(void)
{
    return calls;
}

/* `static` works on functions too: one declared that way exists only in this
 * file, is absent from the header, and cannot be called from anywhere else.
 * It is C's version of private. If nobody uses it, gcc warns that it is
 * unused, which is helpful. */
