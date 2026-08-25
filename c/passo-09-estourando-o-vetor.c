/* ============================================================================
 * STEP 9 - WRONG ON PURPOSE. Writing past the end of an array.
 *
 * This is the bug C is famous for letting through. The program compiles
 * clean, runs, and the damage can surface in a variable you never touched.
 *
 *     Ctrl+Shift+B      (or: make 09)
 *
 * AddressSanitizer will KILL the program on the first bad line. Read its
 * whole message: it is long, and it is the content of this step.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int before = 111;
    int v[5]   = {0, 0, 0, 0, 0};
    int after  = 999;

    printf("before=%d  after=%d\n", before, after);
    printf("v has 5 boxes: valid indices are 0 to 4\n\n");

    /* THE BUG IS THE <=.
     *
     * With i <= 5 the loop runs six times: 0,1,2,3,4 and 5. But v[5] does
     * not exist; the array ends at 4. The write lands in the 4 bytes after
     * the array, which belong to something else.
     *
     * C does not check indices. Ever. There is no IndexError. The address is
     * computed (start + 5*4) and the write simply happens. */
    for (int i = 0; i <= 5; i++) {
        printf("writing 7 into v[%d]\n", i);
        v[i] = 7;
    }

    /* If you got this far, you compiled WITHOUT the sanitizer. Look at what
     * may have happened to the neighbouring variables: */
    printf("\nv[0]=%d v[4]=%d\n", v[0], v[4]);
    printf("before=%d  after=%d\n", before, after);
    printf("one of them may have turned into 7 without you touching it.\n");

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 *     0x7ffd1000  [ 7 ]   v[0]
 *     0x7ffd1004  [ 7 ]   v[1]
 *     0x7ffd1008  [ 7 ]   v[2]
 *     0x7ffd100c  [ 7 ]   v[3]
 *     0x7ffd1010  [ 7 ]   v[4]     <- end of the array
 *     0x7ffd1014  [ 7 ]   <- v[5]: NOT YOURS. We wrote here anyway.
 *
 * What lives at 0x7ffd1014 depends on how the compiler laid out the stack:
 * it could be `before`, it could be `after`, it could be unused alignment
 * padding, it could be the function's return address. It changes with the
 * optimisation level, the gcc version, and the order of the declarations.
 *
 * That is why this bug is so expensive: it does not fail where it is. It
 * corrupts something, and the program breaks three functions later, in code
 * that is correct.
 *
 * WHAT THE SANITIZERS DID
 *
 * You got TWO complaints from two different tools. Read them in the order
 * they came out.
 *
 * First UBSan, which knows the TYPE of the variable and sees a bad index:
 *
 *     runtime error: index 5 out of bounds for type 'int [5]'
 *
 * Then ASan, which does not look at types: it puts "redzones" around every
 * array and watches every memory access:
 *
 *     ERROR: AddressSanitizer: stack-buffer-overflow
 *     WRITE of size 4 at 0x...
 *         #0 in main passo-09-estourando-o-vetor.c:34
 *     Address ... is located in stack of thread T0 at offset 52 in frame
 *       This frame has 1 object(s):
 *         [32, 52) 'v' (line 18) <== Memory access at offset 52 overflows
 *                                    this variable
 *
 * Translating that last line: the array `v` occupies stack positions 32 to
 * 52 (20 bytes, the five ints), and you wrote at exactly 52, the first byte
 * past the end. One step beyond, and ASan saw it.
 *
 * Notice also what did NOT appear: the printf output from before the error.
 * ASan aborts the process without flushing stdout. If your debugging printf
 * ever "disappears" in a crash, that is why, and it does not mean the line
 * failed to run. Step 34 is about exactly this.
 *
 * EXPERIMENTE:
 *
 *  1. Fix it: change `i <= 5` to `i < 5`. Better still, use the sizeof trick
 *     from step 08 so the number 5 is not written in two places. Repeated
 *     numbers in code are how off-by-one is born.
 *
 *  2. See the version with no safety net. In the terminal:
 *
 *         gcc -std=gnu17 -Wall -g passo-09-estourando-o-vetor.c -o /tmp/bare
 *         /tmp/bare
 *
 *     Without the sanitizer the program probably finishes "normally" with
 *     one of the neighbours corrupted. Compare the two outputs. That
 *     difference is the argument for leaving the sanitizer on.
 *
 *  3. Change `v[i] = 7` to `v[i + 1000] = 7`. Now you are far from the
 *     array. Run with and without the sanitizer: without it, a bare
 *     Segmentation fault.
 *
 *  4. Only READ past the end (`printf("%d", v[5]);`). ASan catches it just
 *     the same: "READ of size 4". Reading out of bounds is a bug too, even
 *     when it breaks nothing.
 *
 * -> passo-10: why an array and a pointer are almost the same thing
 * ========================================================================= */
