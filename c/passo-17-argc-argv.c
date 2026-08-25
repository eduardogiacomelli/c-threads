/* ============================================================================
 * STEP 17 - command line arguments, and the atoi that lies.
 *
 * Exercise 7 and both challenges in the pthreads list require this: "the
 * program receives parameters on the command line (use argc and argv)".
 *
 * The first half of this file is correct. The second half is WRONG ON
 * PURPOSE, and the error is silent: nothing breaks, the number is just wrong.
 *
 *     make 17                          (no arguments)
 *     make 17 ARGS="10 4"              (with two arguments)
 *     ./passo-17-argc-argv 1e9 abc     (after building, the interesting case)
 *
 * Ctrl+Shift+B cannot pass arguments. This is one of the few steps that
 * needs the terminal.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* atoi, atof */

/* Until now main was `int main(void)`. This is the other form, and it
 * receives whatever you typed after the program's name:
 *
 *   argc  ("argument count")  how many words, INCLUDING the program name
 *   argv  ("argument vector") an array of strings holding those words
 *
 * `char *argv[]` is an array of pointers to char, which is to say an array
 * of strings (step 10 plus step 11). It is also written `char **argv`: the
 * same thing, for the same reason as step 10. */
int main(int argc, char *argv[])
{
    /* argv[0] is ALWAYS the name the program was invoked with. That is why
     * `./prog 10 4` gives argc = 3, not 2. A classic off-by-one. */
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);

    /* The assignment asks for two numbers. Without them there is nothing to
     * do: print the usage and EXIT. Two important conventions here:
     *
     *   - the error message goes to stderr (fprintf(stderr, ...)), not
     *     stdout, so it still appears on screen when the output is
     *     redirected to a file;
     *   - the exit code is non-zero (step 01), so whoever called knows it
     *     failed.
     *
     * Use argv[0] in the message rather than hard-coding the name: if the
     * binary is renamed, the message stays correct. */
    if (argc < 3) {
        fprintf(stderr, "\nusage: %s <size> <threads>\n", argv[0]);
        fprintf(stderr, "example: %s 1000 4\n", argv[0]);
        return 1;
    }

    /* AN ARGUMENT IS ALWAYS A STRING. Always. Even when it is "10".
     * argv[1] is the bytes '1','0','\0', not the number ten. Converting is
     * your job. */
    printf("\nargv[1] as a string: \"%s\"\n", argv[1]);
    printf("argv[1][0] is the character '%c' (code %d), not the number %c\n",
           argv[1][0], argv[1][0], argv[1][0]);

    /* ================= EVERYTHING BELOW IS WRONG =================
     *
     * atoi is the conversion everybody reaches for first, and it has no way
     * to report an error: its return type is int, and every int is a
     * possible result. So it guesses:
     *
     *   atoi("abc")   ->  0      no number at all, returns 0
     *   atoi("1e9")   ->  1      reads the 1, stops at the 'e', returns 1
     *   atoi("")      ->  0
     *   atoi("12abc") ->  12     reads what it can and ignores the rest
     *
     * Note that 0 means both "it failed" and "the user typed zero". There is
     * no way to tell them apart. */
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    printf("\nwith atoi:  n = %d, m = %d\n", n, m);

    if (n <= 0 || m <= 0) {
        fprintf(stderr, "n and m have to be positive\n");
        return 1;
    }

    /* And here the program carries on happily with the wrong value. */
    printf("about to process %d elements with %d threads\n", n, m);
    printf("(check: is that really what you typed?)\n");

    return 0;
}

/* ============================================================================
 * TRY THESE FOUR CASES IN THE TERMINAL
 *
 *     make                                          # build everything
 *     ./passo-17-argc-argv                          # missing argument
 *     ./passo-17-argc-argv 1000 4                   # correct
 *     ./passo-17-argc-argv abc 4                    # <- atoi returns 0
 *     ./passo-17-argc-argv 1e9 4                    # <- atoi returns 1
 *
 * The last one is what bites in Challenge 1, which asks you to pass the
 * iteration count in scientific notation:
 *
 *     ./calcula-pi 1e9 4
 *
 * With atoi you will not compute 1,000,000,000 iterations. You will compute
 * ONE, in microseconds, and conclude your program is astonishingly fast. The
 * bug disguises itself as a good result.
 *
 * THE argv DIAGRAM
 *
 *   ./prog 1000 4
 *
 *   argc = 3
 *   argv ---> [ 0 ] --> "./prog\0"
 *             [ 1 ] --> "1000\0"
 *             [ 2 ] --> "4\0"
 *             [ 3 ] --> NULL      <- the standard guarantees this NULL
 *
 *   An array of pointers. Each element is the address of a string, exactly
 *   as in step 11. That is why `char *argv[]` and `char **argv` are the same
 *   declaration, and step 26 contrasts this layout with a real 2D array.
 *
 * EXPERIMENTE:
 *
 *  1. Run `./passo-17-argc-argv one two three "four five"`.
 *     How many arguments did the shell hand over? The quotes make one. The
 *     shell splits the words, not your program.
 *
 *  2. Print `argv[argc]`. It is NULL, so you can walk argv with
 *     `while (*argv != NULL)` instead of using argc. Do not, but know it.
 *
 *  3. Run `./passo-17-argc-argv 99999999999 4` (eleven nines). atoi
 *     overflows the int silently. Compare with what step 18 does with the
 *     same input.
 *
 *  4. Swap atoi for atof and print with %f. `atof("1e9")` gives
 *     1000000000.000000, which solves the scientific notation but still
 *     cannot tell you whether the input was garbage. Halfway there.
 *
 * -> passo-18, the validation you can actually hand in
 * ========================================================================= */
