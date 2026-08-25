/* ============================================================================
 * STEP 19 - when one file is not enough: header plus implementation.
 *
 * This step has THREE files: contas.h, contas.c and this one.
 *
 *     make 19
 *
 * NOTE: Ctrl+Shift+B does NOT work here, and the error it gives is the point
 * of the lesson. Press it anyway, read the error, then come back.
 * ========================================================================= */

#include <stdio.h>
#include "contas.h"     /* quotes = my file; < > = the system's */

int main(void)
{
    /* This file does NOT know how sum_to works. It has only seen the
     * signature in the header: takes an int, returns a long. That is enough
     * to compile.
     *
     * That is the whole idea: separate "what you can use" from "how it is
     * built". You have been doing it with printf all along, without ever
     * reading its source. */
    printf("sum_to(10)   = %ld\n", sum_to(10));
    printf("sum_to(100)  = %ld\n", sum_to(100));
    printf("is_even(7)   = %d\n",  is_even(7));

    /* The module's private state, reachable only through the function it
     * chooses to expose. */
    printf("\nsum_to has been called %d times\n", call_count());

    /* Try `calls++` here. It does not compile: that variable is `static` in
     * contas.c, so this file does not even know it exists. */

    return 0;
}

/* ============================================================================
 * THE TWO STAGES, NOW VISIBLE
 *
 * Step 01 said compiling has separate stages. With one file that is
 * invisible. With two it is not:
 *
 *     COMPILE (each .c becomes a .o, separately, knowing nothing of the other)
 *
 *       passo-19-varios-arquivos.c --> passo-19.o
 *          "there is a sum_to, it returns long. I will note a pending call
 *           to it and leave the address blank."
 *
 *       contas.c --> contas.o
 *          "here is the code for sum_to, at this offset."
 *
 *     LINK (join the .o files and resolve every pending reference)
 *
 *       passo-19.o + contas.o --> executable
 *          "the pending call points at the code that came from contas.o."
 *
 * Hence the two most confusing errors in C, which now have addresses:
 *
 *   "implicit declaration of function 'sum_to'"
 *        -> a COMPILE error. The #include is missing: this file never saw
 *           the signature.
 *
 *   "undefined reference to 'sum_to'"
 *        -> a LINK error. It saw the signature; it never got the code. You
 *           forgot to pass contas.c (or contas.o) on the command line.
 *
 * Memorise the difference. "undefined reference" is never fixed by an
 * #include. Step 28 runs all four stages by hand, and step 31 shows the
 * symbol table the linker is actually consulting.
 *
 * BY HAND, TO SEE IT HAPPEN
 *
 *     # both at once (what make does):
 *     gcc -std=gnu17 -Wall -Wextra -g passo-19-varios-arquivos.c contas.c -o prog
 *
 *     # or in two stages, which is what real projects do:
 *     gcc -c contas.c                     # -c = compile only, do not link
 *     gcc -c passo-19-varios-arquivos.c
 *     gcc passo-19-varios-arquivos.o contas.o -o prog
 *
 * The advantage of the second: change one .c and only that one is
 * recompiled. That is what a Makefile is for.
 *
 * AND WITH PTHREADS
 *
 *     gcc ... program.c -o program -pthread
 *
 * The `-pthread` flag (no "l") does both jobs: it links the library and
 * adjusts the compiler. Forgetting it gives exactly
 * "undefined reference to 'pthread_create'", which you can now read: the
 * declaration came from #include <pthread.h>, the code came from nowhere.
 *
 * WHEN TO SPLIT INTO FILES
 *
 * For the pthreads exercises, almost never: a 120-line program fits in one
 * file, and the professor asked for a program, not a library. It pays off
 * when you have a function you want to reuse across several exercises, such
 * as the timing helper, which turns up in Exercise 7 and in both challenges.
 *
 * What you really need from this step is the ability to READ link errors.
 * They will happen.
 *
 * EXPERIMENTE:
 *
 *  1. Press Ctrl+Shift+B on this file. It compiles only ${file} and you get
 *     "undefined reference to `sum_to'". Confirm in the panel that the error
 *     comes from the linker (the message mentions `ld` or `collect2`), not
 *     the compiler.
 *
 *  2. Delete the `#include "contas.h"` and run `make 19`. Now it is the
 *     other error: "implicit declaration". Two errors, one program,
 *     opposite causes.
 *
 *  3. Remove the guards (#ifndef/#define/#endif) from contas.h and include
 *     the header twice in a row in this file. With only function
 *     declarations gcc tolerates it; now add a `typedef struct { int x; }
 *     Point;` to the header and try again: "redefinition of 'Point'". That
 *     is what the guard is for.
 *
 *  4. Change sum_to's return type in contas.c (long to int) and leave the
 *     header alone. `make 19` reports the conflict, because contas.c
 *     includes its own header. That is why it includes it.
 *
 * -> passo-20, and from there the second block
 * ========================================================================= */
