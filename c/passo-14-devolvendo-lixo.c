/* ============================================================================
 * STEP 14 - WRONG ON PURPOSE. Keeping the address of a local variable.
 *
 * So far you have seen WHERE the boxes are. This step is about HOW LONG they
 * exist. It is the missing idea, and it is the one that causes the most
 * trouble in threaded programs.
 *
 *     Ctrl+Shift+B      (or: make 14)
 *
 * ASan kills the program on the very first read. Experiment 1 shows what
 * would have happened without it, and that is where the bug gets frightening.
 * ========================================================================= */

#include <stdio.h>

/* A global pointer, to keep an address between calls. */
int *saved;

/* Every local variable is born on the STACK when the function starts and
 * disappears when the function returns. "Disappears" does not mean erased:
 * it means that stretch of memory becomes available again, and the next
 * function called will use the same space for its own variables.
 *
 * Keeping &number is keeping the address of a box that is being taken apart
 * at that very moment. */
void make_number(void)
{
    int number = 42;

    printf("   [make]   number = %d, lives at %p\n",
           number, (void *) &number);

    saved = &number;         /* <- THE BUG. We noted an address that expires
                              *    one line from now.
                              *
                              *    gcc already complained in the panel:
                              *    "storing the address of local variable
                              *    'number' in 'saved' [-Wdangling-pointer]".
                              *    It saw this before you ran it. */
}

/* This function has nothing to do with the one above. It is simply called
 * afterwards, and so it gets the same piece of stack, with a variable of
 * its own in it. */
void other_function(void)
{
    int other = 777;
    printf("   [other]  other  = %d, lives at %p\n",
           other, (void *) &other);
}

int main(void)
{
    make_number();

    /* It may well print 42. THAT IS THE WORST POSSIBLE CASE: the old value
     * is still there because nothing has stepped on it yet. The bug stays
     * invisible until the day something does. */
    printf("right after:  *saved = %d\n", *saved);

    other_function();

    printf("after another function ran:  *saved = %d\n", *saved);
    printf("^ compare the two addresses printed above. They are the SAME.\n");

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 * The stack is reused constantly:
 *
 *   1) main calls make_number. The stack grows:
 *
 *        [ main ...................... ]
 *        [ make_number: number = 42    ]  <- 0x7fff...bf4
 *
 *   2) make_number returns. That frame is abandoned, but the number
 *      0x7fff...bf4 we noted down is still an address:
 *
 *        [ main ...................... ]
 *        [ ...available, still with 42 ]  <- 0x7fff...bf4
 *
 *   3) main calls other_function, which gets exactly the same space:
 *
 *        [ main ...................... ]
 *        [ other_function: other = 777 ]  <- 0x7fff...bf4, now 777
 *
 *   4) *saved reads 0x7fff...bf4 and finds what the other function left.
 *
 * The pointer never "knew" it had become invalid. A pointer is just a
 * number. It has no way to know its box has gone; you are the one who has to
 * know. The name for this is a DANGLING POINTER.
 *
 * WHAT ASan SAID
 *
 *     ERROR: AddressSanitizer: stack-use-after-return
 *     READ of size 4 at 0x...
 *         #0 in main passo-14-devolvendo-lixo.c:LINE
 *     Address is located in stack of thread T0 in frame
 *         #0 in make_number
 *       This frame has 1 object(s):
 *         [32, 36) 'number' <== Memory access is inside this variable
 *
 * Read the important part: the address main read belongs to ANOTHER
 * FUNCTION'S FRAME, one that has already returned. ASan keeps that map
 * precisely so it can tell you this.
 *
 * THE THREE LIFETIMES IN C
 *
 *   automatic (stack)   int x;            dies at the closing brace
 *   static              static int x;     lives for the whole program
 *   allocated (heap)    malloc            lives until YOU call free
 *
 * Need it to survive the return? Only the last two will do. Step 15.
 *
 * WHY THIS MATTERS SO MUCH WITH THREADS
 *
 * Swap "the function returned" for "the thread has not run yet" and it is
 * the same bug, much harder to see:
 *
 *     for (int i = 0; i < 4; i++)
 *         pthread_create(&t[i], NULL, worker, &i);   // &i: main's box
 *
 * The thread will read that address later, once main has already changed `i`
 * or left the loop where it existed. This is the number one mistake in the
 * PPD assignment, and you have just seen its mechanics with no thread
 * involved at all.
 *
 * EXPERIMENTE:
 *
 *  1. THE MOST IMPORTANT EXPERIMENT IN THIS FILE. Build without the
 *     sanitizer, in the terminal:
 *
 *         gcc -std=gnu17 -Wall -g passo-14-devolvendo-lixo.c -o /tmp/s14
 *         /tmp/s14
 *
 *     No error at all. The program runs to the end, prints 42 and then 777,
 *     and the two addresses printed are identical. A production program
 *     would do this silently, with the wrong value, for years.
 *
 *  2. Try the version everybody writes first: turn it into
 *     `int *make_number(void)` with `return &number;`. Two things happen,
 *     and both are worth seeing:
 *       - gcc warns: "function returns address of local variable";
 *       - and it REPLACES the return with NULL, deliberately, so you hit the
 *         wall. The program dies with "load of null pointer".
 *     The compiler knows this mistake so well that it sabotages your version
 *     of it.
 *
 *  3. Change `int number = 42;` to `static int number = 42;` and run.
 *     It works, and keeps working after other_function, because the variable
 *     left the stack for the static area. Now think: what if two threads
 *     called that function at the same time? They share the SAME box. We
 *     traded one bug for another, and that is why the right answer is
 *     step 15.
 *
 * -> passo-15, malloc and free
 * ========================================================================= */
