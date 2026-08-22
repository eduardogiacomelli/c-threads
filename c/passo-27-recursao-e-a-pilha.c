/* ============================================================================
 * STEP 27 - watching the stack grow, one frame at a time.
 *
 * passo-14 showed that a frame dies at the return. This one shows the frames
 * piling up while the calls are still open, with real addresses - and then
 * what happens when you pile up too many.
 *
 *     Ctrl+Shift+B      (or: make 27)
 *
 * This closes the loop on memoria.md: after this, the stack diagram is
 * something you have measured rather than something you were told.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Each call gets its own `depth`, its own `marker`, its own everything.
 * The recursion is not the point - the FRAMES are. */
static void descend(int depth, int limit, const void *previous)
{
    /* A local. There is one of these per live call, all at once. */
    int marker = depth * 100;

    long gap = previous
        ? (long) ((const char *) previous - (const char *) &marker)
        : 0;

    printf("  depth %d: &marker = %p   marker = %d", depth, (void *) &marker,
           marker);
    if (previous)
        printf("   %+ld bytes from the caller", -gap);
    printf("\n");

    if (depth < limit)
        descend(depth + 1, limit, &marker);

    /* On the way back out, every frame is still intact - the value printed
     * here is the same one printed on the way in. Each call kept its own. */
    printf("  depth %d returning, marker still %d\n", depth, marker);
}

/* A frame with a big local. The array has to live somewhere, and that
 * somewhere is this frame - which is how you make a frame expensive. */
static void heavy(int depth, int limit)
{
    char buffer[16 * 1024];          /* 16 KiB of stack, per call */
    memset(buffer, 0, sizeof(buffer));

    if (depth == 1 || depth == limit)
        printf("  depth %d: buffer at %p\n", depth, (void *) buffer);

    if (depth < limit)
        heavy(depth + 1, limit);
}

int main(int argc, char *argv[])
{
    printf("stack frames, one per live call:\n");
    descend(1, 5, NULL);

    /* Read the sign of those gaps carefully, because it depends on how you
     * compiled.
     *
     *   without sanitizers:  -64 bytes each   the real stack, growing DOWN
     *   with AddressSanitizer: +64 bytes each  ASan's "fake stack", growing UP
     *
     * ASan moves locals onto a separate region so it can detect the
     * passo-14 bug (stack-use-after-return): the old frame has to stay
     * poisoned after the return, which it cannot do if the next call reuses
     * the same bytes. Same program, different layout, because the tool is
     * part of the experiment.
     *
     * Either way the SPACING is constant, and that gap is the frame size the
     * compiler chose for `descend`: locals, saved registers, the return
     * address and alignment padding. */

    printf("\n16 KiB per frame, 8 levels:\n");
    heavy(1, 8);
    printf("  (8 x 16 KiB = 128 KiB of stack, and it barely registers)\n");

    /* HOW MUCH IS THERE?
     *
     *   ulimit -s          8192 on this machine, i.e. 8 MiB for the main thread
     *   man 3 pthread_attr_setstacksize
     *
     * Each thread gets its OWN stack, and the default for a pthread is also
     * around 8 MiB. Ten threads means ten stacks - which is why "just make
     * the buffer a local" scales badly, and why big buffers belong on the
     * heap (passo-15). */
    printf("\nmain's stack is around 8 MiB (check with: ulimit -s)\n");
    printf("EVERY thread gets its own - 8 threads is 8 stacks.\n");

    /* Run with an argument to blow it deliberately:  ./passo-27 boom  */
    if (argc > 1 && strcmp(argv[1], "boom") == 0) {
        printf("\nrecursing until the stack runs out...\n");
        fflush(stdout);      /* flush first: the crash will not do it for us */
        heavy(1, 100000);
    }

    return 0;
}

/* ============================================================================
 * WHAT THE ADDRESSES SHOWED
 *
 * Compiled plainly (gcc -Wall -g, no sanitizers) on this machine:
 *
 *   depth 1: &marker = 0x7ffcf776afec
 *   depth 2: &marker = 0x7ffcf776afac      64 bytes LOWER
 *   depth 3: &marker = 0x7ffcf776af6c      64 bytes LOWER
 *
 * Constant spacing, downwards - the real stack. Each call pushes a frame of
 * the same size because it is the same function with the same locals. Change
 * the locals and the spacing changes; try experiment 2.
 *
 * Under Ctrl+Shift+B the same program prints +64 instead. That is not a bug
 * in either the program or the tool: ASan gives each frame a slot on a
 * separately managed region so it can keep a returned frame poisoned and
 * catch use-after-return. The lesson is worth more than the diagram - an
 * instrumented build is not the program you ship, and for anything about
 * layout you check both.
 *
 * WHY RECURSION IS EXPENSIVE IN C IN A WAY IT IS NOT IN PYTHON
 *
 * Python raises RecursionError at a limit it enforces itself (1000 by
 * default). C has no limit and no check: it keeps pushing frames until the
 * stack region runs into the guard page, and then the process dies. There is
 * no exception to catch, and without a sanitizer the message is just
 * "Segmentation fault".
 *
 * The depth you can reach is stack size divided by frame size, so it is not a
 * number - it depends on how fat your frames are. 8 MiB of 48-byte frames is
 * about 175,000 calls. 8 MiB of 16 KiB frames is 512.
 *
 * EXPERIMENTE:
 *
 *  1. Blow the stack on purpose, from the terminal:
 *
 *         make 27
 *         ./passo-27-recursao-e-a-pilha boom
 *
 *     AddressSanitizer reports "stack-overflow" and points at the frame.
 *     Now compile without sanitizers and run it again: bare "Segmentation
 *     fault", no line, no reason. Same bug, and only one of them is
 *     debuggable.
 *
 *  2. Add `double pad[64];` to `descend` and rerun. The gap between frames
 *     jumps by about 512 bytes. You are reading the compiler's frame layout
 *     straight off the addresses.
 *
 *  2b. Compile both ways and compare the direction:
 *
 *         gcc -std=gnu17 -Wall -g passo-27-recursao-e-a-pilha.c -o /tmp/plain
 *         /tmp/plain            # gaps are negative: the real stack
 *         make 27               # gaps are positive: ASan's fake stack
 *
 *  3. Rewrite `descend` as a loop. The stack stops growing entirely - one
 *     frame, reused. Any recursion whose recursive call is the last thing it
 *     does can be rewritten this way, and gcc at -O2 often does it for you
 *     (tail-call optimisation). Check with `gcc -O2 -S`, or the assembly
 *     task in the playground.
 *
 *  4. Compute your own limit: divide `ulimit -s` (in KiB) by the frame gap
 *     you measured in part 1, then test the prediction with `descend(1, N)`.
 *     Being within 10% is a good sign you have understood the model.
 *
 * -> back to "00 - COMECE AQUI.md", and then the pthreads tutorial
 * ========================================================================= */
