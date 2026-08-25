/* ============================================================================
 * STEP 12 - WRONG ON PURPOSE. strcpy into a destination that is too small.
 *
 * This is, literally, the bug behind most of the security failures in the
 * history of software. And it is step 09 with char.
 *
 *     Ctrl+Shift+B      (or: make 12)
 *
 * AddressSanitizer kills the program. Read its whole report.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>

int main(void)
{
    /* Room for 7 characters plus the '\0'. */
    char first[8];
    char last[8] = "Silva";

    printf("first[] is %zu bytes -> 7 letters plus the terminator fit\n\n",
           sizeof(first));

    /* BUG 1 - strcpy does not ask how big the destination is. IT CANNOT: it
     * receives two addresses and nothing else. It copies from the source
     * until it finds a '\0' and writes all of that into the destination,
     * whatever the cost.
     *
     * "Eduardo Giacomelli" is 18 characters plus a terminator, 19 bytes.
     * The destination holds 8. Where do the other 11 go? Into the memory
     * after first[]. */
    strcpy(first, "Eduardo Giacomelli");

    printf("first = %s\n", first);
    printf("last  = %s   <- look closely\n", last);

    /* BUG 2 - the same mistake by another route: concatenating without
     * checking whether it fits. strcat walks to the destination's '\0' and
     * starts writing from there. */
    char small[10] = "12345";
    strcat(small, "67890abcdef");
    printf("small = %s\n", small);

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 *     char first[8];          char last[8] = "Silva";
 *
 *     before the strcpy:
 *     0x..00 [?][?][?][?][?][?][?][?]  [S][i][l][v][a][\0][0][0]
 *            \__________ first _____/  \___________ last ______/
 *
 *     strcpy(first, "Eduardo Giacomelli") writes 19 bytes from 0x..00:
 *
 *     0x..00 [E][d][u][a][r][d][o][ ]  [G][i][a][c][o][m][e][l][l][i][\0]
 *            \__________ first _____/  \____ where last used to be ____/
 *                                       \-- and then past even that
 *
 * `last` was overwritten by a variable that has nothing to do with it. No
 * line in the program mentions `last`, and it changed.
 *
 * Now imagine that instead of `last`, the function's return address sat
 * there. Whoever controls the input text then controls where the program
 * jumps. That is the mechanism behind "buffer overflow" in security news.
 *
 * THE ASan REPORT
 *
 *     ERROR: AddressSanitizer: stack-buffer-overflow
 *     WRITE of size 19 at 0x...
 *         #0 in memcpy
 *         #1 in main passo-12-estourando-a-string.c:30
 *     This frame has 3 object(s):
 *       [32,  40) 'first' (line 18) <== Memory access at offset 40 overflows
 *                                       this variable
 *       [64,  72) 'last' (line 19)
 *       [96, 106) 'small' (line 37)
 *
 * "WRITE of size 19" into an 8-byte object: the arithmetic is right there.
 * And note that ASan lists all three stack variables with their exact
 * bounds, which is how you find out WHO you stepped on.
 *
 * (strcpy became memcpy in the report: gcc swaps in an optimised version
 * when it knows the length. It is still your line 30.)
 *
 * THE RULE
 *
 *   Never use strcpy, strcat or sprintf with data whose size you do not
 *   control. There are versions that take the destination size: step 13.
 *
 * EXPERIMENTE:
 *
 *  1. Comment out the strcpy and run only the strcat. Same kind of error,
 *     different function. ASan points at strcat.
 *
 *  2. Enlarge it to char first[64] and run. It passes clean. Notice the
 *     discomfort: the program is "correct" only because the name happened to
 *     fit this time. A size bug is always a "what if the data is bigger?"
 *     bug.
 *
 *  3. Run it without the sanitizer and compare:
 *
 *         gcc -std=gnu17 -Wall -g passo-12-estourando-a-string.c -o /tmp/s12
 *         /tmp/s12
 *
 *     It may print everything "normally" with `last` corrupted, or it may
 *     crash. It depends on the compiler's mood that day.
 *
 *  4. Copy exactly 7 letters ("Eduardo") into first[8]. It fits, with the
 *     terminator in the eighth byte. Now try 8 letters. One byte more, and
 *     ASan catches it. That is the margin you work with in C.
 *
 * -> passo-13, the right way
 * ========================================================================= */
