/* ============================================================================
 * STEP 10 - an array "decays" to a pointer the moment you pass it on.
 *
 * This explains three things at once: why a function taking an array needs
 * the length as well, why sizeof lies inside that function, and why v[i] and
 * *(v+i) are the same thing written two ways.
 *
 *     Ctrl+Shift+B      (or: make 10)
 * ========================================================================= */

#include <stdio.h>

/* `int v[]` is written here on purpose, because it is how almost everyone
 * writes it. But it is a FRIENDLY LIE from C: the parameter is not an array.
 * The compiler silently rewrites it as `int *v`.
 *
 * The whole array is NOT copied at the call (that would be expensive). Only
 * the address of the first element is copied, which is step 06's rule with
 * no exception.
 *
 * Consequence: `n` is not ceremony. Without it the function has no way to
 * know where the array ends. */
int sum(int v[], size_t n)
{
    /* HERE IS THE TRAP. sizeof(v) is not the size of the array: v is a
     * pointer, so sizeof(v) is 8. The trick from step 08 only works in the
     * scope where the array was DECLARED. */
    printf("   [inside sum] sizeof(v) = %zu  <- 8, it is a pointer!\n",
           sizeof(v));

    int total = 0;
    for (size_t i = 0; i < n; i++)
        total += v[i];
    return total;
}

int main(void)
{
    int grades[5] = {7, 8, 10, 6, 9};

    printf("[in main] sizeof(grades) = %zu  <- 20, the whole block\n",
           sizeof(grades));

    /* The array's name, used on its own, IS the address of the first
     * element. These two lines print the same number: */
    printf("\ngrades     = %p\n", (void *) grades);
    printf("&grades[0] = %p   <- the same address\n", (void *) &grades[0]);

    /* So this is legal, and it is what the function call does underneath: */
    int *p = grades;
    printf("\n*p        = %d   (the first element)\n", *p);
    printf("p[2]      = %d   (indexing a POINTER, no array in sight)\n", p[2]);

    /* POINTER ARITHMETIC: adding 1 to a pointer moves ONE ELEMENT, not one
     * byte. The compiler multiplies by the sizeof the pointed-to type.
     *
     *     p + 1  ->  address + 4  (because it is an int *)
     *
     * And then the identity that defines indexing in C:
     *
     *     v[i]  IS BY DEFINITION  *(v + i)
     *
     * Brackets are syntactic sugar for "move i elements along, then
     * dereference". */
    printf("\np     = %p  -> *p     = %d\n", (void *) p,     *p);
    printf("p + 1 = %p  -> *(p+1) = %d   (moved 4 bytes)\n",
           (void *) (p + 1), *(p + 1));
    printf("grades[1] = %d  == *(grades + 1) = %d\n",
           grades[1], *(grades + 1));

    /* An amusing and useless consequence: since v[i] is *(v+i) and addition
     * commutes, 1[grades] compiles and works. Never write this. It exists
     * only to prove that brackets really are just notation. */
    printf("1[grades] = %d   (works, and is horrible)\n", 1[grades]);

    /* The call: we pass `grades` (which decays to a pointer) and the size. */
    size_t n = sizeof(grades) / sizeof(grades[0]);  /* compute it HERE */
    printf("\nsum = %d\n", sum(grades, n));

    /* And because the function only has a pointer, you can sum a SLICE:
     * `grades + 2` is the address of the third element, and 3 remain from
     * there.
     *
     * This is exactly how work is divided between threads: each one gets a
     * pointer to its slice and the length of it. Keep this. */
    printf("sum of the last 3 = %d\n", sum(grades + 2, 3));

    return 0;
}

/* ============================================================================
 * THE DIAGRAM
 *
 *     grades ---> 0x7ffd1000 [  7 ]   grades[0]  *(grades+0)
 *                 0x7ffd1004 [  8 ]   grades[1]  *(grades+1)
 *                 0x7ffd1008 [ 10 ]   grades[2]  *(grades+2)
 *                 0x7ffd100c [  6 ]   grades[3]
 *                 0x7ffd1010 [  9 ]   grades[4]
 *
 *     sum(grades, 5)  copies only the arrow, not the boxes.
 *
 *     sum(grades + 2, 3)
 *                        \---> starts at 0x7ffd1008, sees 3 boxes
 *
 * THE THREE SENTENCES OF THIS STEP
 *
 *   1. Passing an array to a function passes a POINTER. Nothing is copied.
 *   2. So the function always needs the length as an argument.
 *   3. v[i] is *(v + i). It always was.
 *
 * And the corollary that answers experiment 4 of step 08: you cannot write
 * `a = b` between arrays because an array's name is not a reassignable box,
 * it is the address of the block. Copying is a loop or memcpy.
 *
 * EXPERIMENTE:
 *
 *  1. Inside sum, change the loop to `for (size_t i = 0; i <= n; i++)`.
 *     ASan catches the same stack-buffer-overflow as step 09, now across a
 *     function boundary. Note that the report shows the whole stack: where
 *     it overflowed (sum) and who called it (main).
 *
 *  2. Try computing the size INSIDE sum with sizeof(v)/sizeof(v[0]).
 *     You get 2 (8 bytes / 4 bytes). Wrong and silent, though gcc does warn
 *     ("sizeof on array function parameter"). This one turns up in exams.
 *
 *  3. Call `sum(grades + 3, 5)`. You asked for 5 elements starting at the
 *     fourth, and only 2 exist. ASan catches it. An offset pointer plus a
 *     wrong length is the number one source of bugs when dividing work
 *     between threads.
 *
 *  4. Write `void double_all(int *v, size_t n)` that multiplies each element
 *     by 2, call it from main and print the array afterwards. It works: the
 *     function reaches main's boxes, for the same reason as step 07.
 *
 * -> passo-11: strings, which are just char arrays with one extra rule
 * ========================================================================= */
