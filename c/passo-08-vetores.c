/* ============================================================================
 * STEP 8 - an array is not a list.
 *
 * An array in C is a contiguous block of same-typed boxes whose size is fixed
 * at compile time. It does not grow, it does not know its own length, it has
 * no .append, and nobody checks whether you walked off the end of it.
 *
 *     Ctrl+Shift+B      (or: make 08)
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* 5 int boxes, GLUED TOGETHER in memory, in this order.
     * The name `grades` refers to the whole block. */
    int grades[5] = {7, 8, 10, 6, 9};

    /* Indices start at 0, so the last one is 4, never 5. */
    printf("first grade: %d\n", grades[0]);
    printf("last grade:  %d\n", grades[4]);

    /* The array does not know it has 5 elements. You do.
     * A trick that works ONLY HERE, in the scope that declared it:
     *
     *     sizeof(grades)     = 20 bytes (the whole block)
     *     sizeof(grades[0])  = 4 bytes  (one box)
     *     20 / 4             = 5
     *
     * Remember the "only here" caveat: step 10 shows where it breaks. */
    size_t count = sizeof(grades) / sizeof(grades[0]);
    printf("\nsizeof(grades) = %zu bytes, each int = %zu -> %zu elements\n",
           sizeof(grades), sizeof(grades[0]), count);

    /* The classic C loop. Note i < count, with <, never <=.
     * Changing that to <= is the off-by-one, and it is what step 09 is about. */
    printf("\nall the grades:\n");
    for (size_t i = 0; i < count; i++)
        printf("  grades[%zu] = %d\n", i, grades[i]);

    /* Summing. No sum(): you write the loop. */
    int total = 0;
    for (size_t i = 0; i < count; i++)
        total += grades[i];

    printf("\ntotal = %d, average = %.2f\n", total, (double) total / count);
    /* that (double) from step 04 again: without it the average is integer */

    /* The addresses show the block is contiguous: each int advances 4 bytes. */
    printf("\nwhere each box lives:\n");
    for (size_t i = 0; i < count; i++)
        printf("  grades[%zu] at %p\n", i, (void *) &grades[i]);

    /* Initialising with fewer values than the size zero-fills the rest.
     * This is the idiomatic way to zero a whole array. */
    int zeroed[5] = {0};
    printf("\nzeroed[3] = %d (the {0} filled everything with zero)\n",
           zeroed[3]);

    /* CAREFUL: with NO initialiser at all, a local array holds GARBAGE,
     * whatever was in that stretch of memory before. Not zero.
     *     int junk[5];          <- all 5 values are unpredictable
     */

    return 0;
}

/* ============================================================================
 * THE DIAGRAM
 *
 *     address       contents     name
 *     0x7ffd1000    [  7 ]       grades[0]
 *     0x7ffd1004    [  8 ]       grades[1]
 *     0x7ffd1008    [ 10 ]       grades[2]
 *     0x7ffd100c    [  6 ]       grades[3]
 *     0x7ffd1010    [  9 ]       grades[4]
 *     0x7ffd1014    [ ??? ]      <- past the end. There IS memory here, and
 *                                   it belongs to something else.
 *
 * Each index advances 4 bytes because each int takes 4. The address of
 * grades[i] is simply:  start + i * sizeof(int).
 *
 * That is a multiplication, not a search. It is why reaching grades[9999] is
 * exactly as fast as grades[0], and why nothing notices it is invalid.
 *
 * EXPERIMENTE:
 *
 *  1. Check the arithmetic against the real output: subtract the printed
 *     addresses. The difference is 4 (in hex: 1000, 1004, 1008...).
 *
 *  2. Declare `int junk[5];` with no initialiser and print all five. Run it
 *     twice. In Python this is impossible; here it is routine.
 *
 *  3. Change it to `int grades[5] = {7, 8};`. Print all of them. The last
 *     three are zero, but only because there WAS an initialiser. Without
 *     `= {...}`, garbage.
 *
 *  4. Try `grades = zeroed;`. Compile error. An array's name is not a
 *     variable you can reassign. Copying an array in C is a loop or memcpy,
 *     never `=`. Step 10 explains why.
 *
 * -> passo-09, where we walk off the end on purpose
 * ========================================================================= */
