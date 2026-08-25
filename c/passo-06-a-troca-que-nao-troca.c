/* ============================================================================
 * STEP 6 - WRONG ON PURPOSE. The function that swaps two values, and does not.
 *
 * Run it first. Nothing crashes, nothing warns, the compiler stays quiet. It
 * simply does not do what it says it does.
 *
 *     Ctrl+Shift+B      (or: make 06)
 * ========================================================================= */

#include <stdio.h>

/* The idea is obvious and the code looks right. */
void swap(int a, int b)
{
    printf("   [inside] received a=%d b=%d\n", a, b);

    int temp = a;
    a = b;
    b = temp;

    printf("   [inside] now a=%d b=%d - swapped in here!\n", a, b);
}

int main(void)
{
    int x = 10;
    int y = 20;

    printf("before: x=%d y=%d\n", x, y);
    swap(x, y);
    printf("after:  x=%d y=%d   <- nothing changed\n", x, y);

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 * C ALWAYS passes arguments by COPY. No exceptions, for any type.
 *
 * When main calls swap(x, y), what the function receives are NEW boxes with
 * the same contents:
 *
 *     main:                      swap:
 *     0x7ffd1000 [ 10 ]  x       0x7ffd0900 [ 10 ]  a   <- a copy of x
 *     0x7ffd1004 [ 20 ]  y       0x7ffd0904 [ 20 ]  b   <- a copy of y
 *
 * The function swaps ITS OWN copies:
 *
 *     main:                      swap:
 *     0x7ffd1000 [ 10 ]  x       0x7ffd0900 [ 20 ]  a
 *     0x7ffd1004 [ 20 ]  y       0x7ffd0904 [ 10 ]  b
 *
 * and when the function ends, the boxes `a` and `b` stop existing. All that
 * work is thrown away. `x` and `y` never heard about any of it.
 *
 * "But in Python a function can change my list!" It can, and it is the same
 * rule: Python also copies the ARGUMENT, which for a list is a reference. The
 * copied reference points at the same object. C has no automatic references:
 * if you want the function to reach your box, you hand over its address
 * yourself.
 *
 * The question that settles this, and that keeps paying off for the rest of
 * your time in C:
 *
 *     "Does the function need to CHANGE something of mine, or only READ it?"
 *
 *     read only -> pass the value
 *     change    -> pass the address
 *
 * EXPERIMENTE:
 *
 *  1. Print the addresses on both sides and see that they are different
 *     boxes:
 *
 *         inside swap:  printf("   [inside] &a=%p\n", (void *) &a);
 *         inside main:  printf("&x=%p\n", (void *) &x);
 *
 *     Different addresses means different boxes, which means nothing the
 *     function does can reach yours.
 *
 *  2. Make swap() return something (`return a;`). You can return ONE value.
 *     You need to change two. That is why a pointer is not optional here.
 *
 * -> passo-07, the fix
 * ========================================================================= */
