/* ============================================================================
 * STEP 1 - the smallest program there is, line by line.
 *
 * You already know how to program. What changes here is the execution model:
 * there is no interpreter. This text file becomes a binary, and the system
 * runs the binary. Nothing runs it "from outside".
 *
 *     Ctrl+Shift+B      (or: make 01)
 * ========================================================================= */

/* #include is NOT `import`. It loads nothing at run time.
 *
 * It is an instruction to the PREPROCESSOR, which runs before the compiler:
 * it deletes this line and pastes the entire contents of stdio.h in its place
 * (the file is at /usr/include/stdio.h, and you can open it).
 *
 * What gets pasted are DECLARATIONS: "there is a function called printf, it
 * takes a string plus whatever follows, and returns int". The signature only.
 * printf's actual code is compiled inside libc, and the LINKER joins the two
 * at the end.
 *
 * Two stages, two different errors:
 *   forgot the #include  -> "implicit declaration of function 'printf'"
 *   forgot to link       -> "undefined reference to 'printf'"
 */
#include <stdio.h>

/* `int main(void)` is where the system starts executing. It is not a
 * programmer convention: the binary points here.
 *
 *   int    -> main hands an integer back to the operating system
 *   (void) -> takes no arguments at all. An empty `main()` means something
 *             different in C ("I am saying nothing about the arguments"),
 *             so write (void) when you want none.
 */
int main(void)
{
    /* printf is not `print`. The first argument is a TEMPLATE (a format
     * string), and each % inside it is a hole to fill from the arguments
     * that follow.
     *
     *   %d = fill with an int, written in decimal
     *   %s = fill with a string
     *   \n = newline. printf does NOT add one for you, unlike Python's
     *        print(). If your output comes out run together, that is why.
     */
    printf("hello, C\n");
    printf("two plus two is %d\n", 2 + 2);

    /* The value main returns is the program's "exit code".
     *   0             = everything went fine
     *   anything else = something failed
     *
     * The shell reads this, not you. It is how `cmd1 && cmd2` knows whether
     * it may run cmd2.
     */
    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 *   passo-01.c  --preprocessor--->  one huge .c with stdio.h pasted in
 *               --compiler------->  machine code (.o)
 *               --linker--------->  an executable, with libc joined on
 *               --you------------>  ./passo-01-o-primeiro-programa
 *
 * An error can come from any of those four stages, and the messages look
 * quite different. Knowing which stage produced one saves a lot of time.
 * Step 28 runs all four by hand.
 *
 * EXPERIMENTE:
 *
 *  1. Delete the \n from the first printf. Run it. Watch the two lines run
 *     together.
 *
 *  2. Delete the #include <stdio.h> line. Run it.
 *     gcc complains: "implicit declaration of function 'printf'". It still
 *     produces the binary (a leftover from the 1970s), but that warning
 *     means it is guessing the types. Never ignore it.
 *
 *  3. Change `return 0;` to `return 3;`. Run it from the terminal and ask
 *     for the exit code:
 *
 *         make 01 ; echo "exited with: $?"
 *
 *  4. Change printf("hello, C\n") to printf("hello, %s\n", "C").
 *     Same result, now with one hole filled.
 *
 * -> passo-02
 * ========================================================================= */
