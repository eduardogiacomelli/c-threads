/* ============================================================================
 * STEP 21 - WRONG ON PURPOSE. A "generic" swap that only knows about int.
 *
 * `void *` means "an address, and I have forgotten what lives there". The
 * forgetting is the point: it lets one function work with any type. But the
 * function still has to be told, somehow, HOW MUCH memory to move - and this
 * version guesses.
 *
 *     Ctrl+Shift+B      (or: make 21)
 *
 * It compiles clean. No warning, no sanitizer error. It just quietly ruins
 * half of every double you give it.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>

/* The idea is right: take two addresses, swap what is at them.
 * The bug is on the cast. */
void swap(void *a, void *b)
{
    /* THE BUG. We decided, here, that everything is an int.
     *
     * `void *` carries no size information at runtime - there is nothing in
     * those 8 bytes that says "4 bytes of int live here". The only thing that
     * decides how much gets copied is the TYPE WE CAST TO, and we hard-coded
     * it. */
    int *pa = (int *) a;
    int *pb = (int *) b;

    int temp = *pa;      /* reads 4 bytes */
    *pa = *pb;           /* writes 4 bytes */
    *pb = temp;
}

int main(void)
{
    /* With ints it works, which is exactly why the bug survives review. */
    int x = 10, y = 20;
    swap(&x, &y);
    printf("ints:    x=%d y=%d   <- correct, and misleading\n", x, y);

    /* With doubles it does not. A double is 8 bytes; we moved 4.
     * Each value keeps half of its own bits and takes half of the other's. */
    double p = 1.5, q = 3.14159;
    printf("\nbefore:  p=%.12f q=%.12f\n", p, q);
    swap(&p, &q);
    printf("after:   p=%.12f q=%.12f\n", p, q);
    printf("         ^ neither swapped nor unchanged. Both are slightly\n");
    printf("           wrong, which reads like a rounding error.\n");

    /* Look at the raw bytes and the mechanism is obvious. */
    unsigned char pb[8], qb[8];
    memcpy(pb, &p, 8);
    memcpy(qb, &q, 8);
    printf("\np bytes: ");
    for (int i = 0; i < 8; i++) printf("%02x ", pb[i]);
    printf("\nq bytes: ");
    for (int i = 0; i < 8; i++) printf("%02x ", qb[i]);
    printf("\n         \\_______/ \\_______/\n");
    printf("         swapped   left alone\n");

    /* With char it is worse in the other direction: we read and write 4 bytes
     * where only 1 belongs to us, so we trample the neighbours. THIS one the
     * sanitizer does catch - see experiment 2. */

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 * A double holds its bits like this (little-endian, passo-02 + byte view):
 *
 *     1.5      = [ 00 00 00 00 ][ 00 00 f8 3f ]
 *     3.14159  = [ 6e 86 1b f0 ][ f9 21 09 40 ]
 *                 \__ low 4 __/  \_ high 4 __/
 *                   swapped        untouched
 *
 * so 1.5 ends up holding pi's low mantissa bits under its own exponent:
 *
 *     result   = [ 6e 86 1b f0 ][ 00 00 f8 3f ]  = 1.500000894470
 *
 * The low four bytes of a double are the least significant bits of the
 * mantissa. Swapping only those produces a number that is *almost* the
 * original - which is the worst possible failure mode, because a rounding-ish
 * looking result gets blamed on floating point rather than on the swap.
 *
 * THE REAL LESSON
 *
 *   A `void *` is an address with the type erased. Erasing the type erases
 *   the size. Any function that takes `void *` and needs to MOVE the data
 *   must be told the size as a separate argument. There is no other way -
 *   the information genuinely is not there.
 *
 *   This is why the standard library looks like this:
 *
 *       void *memcpy(void *dst, const void *src, size_t n);
 *       void  qsort(void *base, size_t nmemb, size_t size, ...);
 *                                             ^^^^^^^^^^^
 *
 *   Every generic function in C carries a size. Now you know why.
 *
 * EXPERIMENTE:
 *
 *  1. Swap two floats (4 bytes each). It works - by accident, because float
 *     happens to be the same size as int. Accidentally-correct code is how
 *     this bug reaches production.
 *
 *  2. Swap two chars:
 *
 *         char c = 'A', d = 'B';
 *         swap(&c, &d);
 *
 *     Now the sanitizer fires: reading and writing 4 bytes where 1 byte
 *     lives. Compare the two failures - with double it is silent corruption,
 *     with char it is a detectable overflow. Same bug, different luck.
 *
 *  3. Print sizeof(void *) and sizeof(int *). Both 8. The pointer knows
 *     nothing; only the type in your source does.
 *
 *  4. Before reading passo-22, try to fix it yourself. You need to move
 *     `size` bytes without knowing the type - which standard function does
 *     that? (`man 3 memcpy`.)
 *
 * -> passo-22, the fix
 * ========================================================================= */
