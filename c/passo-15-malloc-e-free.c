/* ============================================================================
 * STEP 15 - the fix for step 14: memory you control.
 *
 * malloc asks the system for a piece of memory. That piece belongs to no
 * function: it exists until you hand it back with free. It is the only
 * memory in C whose lifetime is your decision.
 *
 *     Ctrl+Shift+B      (or: make 15)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* malloc, calloc, free: man 3 malloc */

/* Now the function returns an address that stays valid after the return,
 * because the box is not on its stack. */
int *make_number(void)
{
    /* malloc takes a number of BYTES and returns the address of the start of
     * the block, or NULL if it could not.
     *
     * Write sizeof(int), not 4. sizeof documents the intent and stays
     * correct if the type ever changes. */
    int *p = malloc(sizeof(int));

    /* Always test the return. In practice malloc almost never fails on
     * modern Linux, but "almost never" with a pointer is a silent crash:
     * without this if, the *p below would write to address 0. */
    if (p == NULL) {
        perror("malloc");
        return NULL;
    }

    *p = 42;                 /* write THROUGH the pointer, as in step 07 */
    printf("   [make]  allocated at %p and stored %d\n", (void *) p, *p);
    return p;                /* we return the address, and it stays valid */
}

/* The same function as in step 14: it reuses the piece of stack that
 * make_number gave up. Watch the address it prints. */
void other_function(void)
{
    int other = 777;
    printf("   [other] other = %d, lives at %p\n", other, (void *) &other);
}

int main(void)
{
    int *p = make_number();
    if (p == NULL)
        return 1;

    printf("right after:  *p = %d\n", *p);
    other_function();
    printf("after another function ran:  *p = %d   <- untouched\n", *p);
    printf("^ the malloc address has nothing to do with the stack one.\n");

    /* free hands the block back. After that the pointer is dangling again:
     * the address is still a valid number, and the memory is no longer
     * yours.
     *
     * The habit that avoids the bug: null the pointer right after freeing. */
    free(p);
    p = NULL;
    printf("\nfreed. p is NULL now, so an accidental use blows up straight\n"
           "away instead of reading garbage.\n");

    /* An array whose size is decided at RUN time, which a fixed `int v[n]`
     * cannot give you. This is the other half of what malloc is for. */
    int count = 5;
    int *v = malloc(count * sizeof(int));   /* n elements, not n bytes! */
    if (v == NULL)
        return 1;

    for (int i = 0; i < count; i++)
        v[i] = (i + 1) * 10;                /* indexed exactly like an array */

    printf("\narray on the heap: ");
    for (int i = 0; i < count; i++)
        printf("%d ", v[i]);
    printf("\n");

    /* calloc does the same and zeroes it too. Different signature:
     * (how many, size of each). */
    int *zeroed = calloc(count, sizeof(int));
    if (zeroed == NULL)
        return 1;
    printf("calloc arrives zeroed: zeroed[3] = %d\n", zeroed[3]);

    free(v);
    free(zeroed);

    /* If you forget a free, LeakSanitizer reports it when the program ends.
     * Experiment 1. */
    return 0;
}

/* ============================================================================
 * STACK vs HEAP, SIDE BY SIDE
 *
 *   STACK                              HEAP
 *   int x;  inside a function          malloc
 *   ---------------------------        --------------------------------
 *   the compiler manages it            you manage it
 *   dies at the closing brace          dies at the free
 *   fast, automatic                    slower, manual
 *   fixed size, decided at             size decided at run time
 *     compile time
 *   about 8 MB total, and it runs out  limited by RAM
 *
 *   +-------------------+
 *   | main: p [0x5a10] -|---> HEAP: 0x5a10 [ 42 ]
 *   +-------------------+                (belongs to no function;
 *   | make_number       |                 survives the return)
 *   |   (already gone)  |
 *   +-------------------+
 *
 * THE FOUR RULES
 *
 *   1. Every malloc has exactly one free. One.
 *   2. Test malloc's return against NULL.
 *   3. After free, assign NULL. Using freed memory is "use-after-free", and
 *      it is a security bug, not merely a correctness one.
 *   4. Make it explicit WHO frees. "This function returns allocated memory;
 *      the caller frees it" is a mandatory comment. In C that is a social
 *      contract, and the compiler will not help you keep it.
 *
 * WHERE THIS GOES IN PPD
 *
 *   A thread can receive only ONE argument, and it has to keep existing
 *   while the thread runs, possibly after the function that created the
 *   thread has already returned. That is step 14 again.
 *
 *   The answer is this: one malloc per thread, each with ITS OWN block, and
 *   the free done by whoever agreed to do it.
 *
 * EXPERIMENTE:
 *
 *  1. Comment out `free(v);` and run. LeakSanitizer prints at the end:
 *     "Direct leak of 20 byte(s) in 1 object(s)", with the allocation stack.
 *     It shows you the LINE OF THE MALLOC that leaked.
 *
 *  2. Call `free(p)` twice. ASan kills it with "attempting double-free" and
 *     shows three stacks: where it was allocated, where it was freed, and
 *     where you tried again. Without the sanitizer this corrupts malloc's
 *     internal structures and breaks somewhere far away.
 *
 *  3. After `free(v)`, read `v[0]` (with the NULL out of the way). ASan says
 *     "heap-use-after-free". Compare with step 14: same bug, different
 *     region of memory.
 *
 *  4. The classic mistake: write `malloc(count)` instead of
 *     `malloc(count * sizeof(int))`. You asked for 5 BYTES and used 5 INTS,
 *     which is 20 bytes. ASan: "heap-buffer-overflow". Always multiply by
 *     the sizeof.
 *
 *  5. Return an allocated ARRAY: `int *make_array(int n)` that mallocs,
 *     fills and returns it. Call it from main, use it, free it in main.
 *     Write in the function's comment who frees.
 *
 * -> passo-16: structs, the last brick before threads
 * ========================================================================= */
