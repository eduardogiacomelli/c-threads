/* ============================================================================
 * STEP 13 - the fix: every writing function is told the destination size.
 *
 *     Ctrl+Shift+B      (or: make 13)
 *
 * The idea is always the same: whoever writes needs to know where to stop.
 * Since the destination does not know its own size, you tell it.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>

int main(void)
{
    char name[8];

    /* snprintf is the standard tool. It:
     *   - takes the destination size (sizeof(name), worked out by the
     *     compiler; never write the 8 by hand, or the two numbers drift
     *     apart the day you resize the array);
     *   - truncates whatever does not fit;
     *   - ALWAYS terminates with '\0';
     *   - and returns how many bytes it WANTED to write.
     *
     * That return value is the truncation detector: if it is >= the size of
     * the destination, the text did not fit.
     *
     * gcc warns here ("output truncated"), and it can only do that because
     * the string is a constant it reads while compiling. With a name typed
     * by a user it would have no way to know, and no warning would come. So
     * the run-time check below is the protection that actually counts. */
    int wanted = snprintf(name, sizeof(name), "%s", "Eduardo Giacomelli");

    printf("name  = \"%s\"  (%zu bytes available)\n", name, sizeof(name));
    printf("wanted to write %d bytes -> ", wanted);
    if (wanted >= (int) sizeof(name))
        printf("DID NOT FIT, truncated\n");
    else
        printf("fitted whole\n");

    /* Truncating silently beats corrupting memory, but it is still wrong
     * data. In real code you handle the case: report an error, or allocate
     * something bigger (step 15). What matters is that you now KNOW it
     * happened. */

    /* snprintf is also how you build text with numbers in it: it is printf,
     * writing into an array instead of onto the screen. */
    char line[64];
    int  age = 25;
    double grade = 8.75;
    snprintf(line, sizeof(line), "%s is %d and scored %.1f",
             "Ana", age, grade);
    printf("\nline  = \"%s\"\n", line);

    /* Concatenating safely: write starting from the current end, with the
     * space that REMAINS as the limit. The two strlen calculations are the
     * whole of the care required. */
    size_t used = strlen(line);
    snprintf(line + used, sizeof(line) - used, " (group B)");
    printf("line  = \"%s\"\n", line);

    /* And the easiest case of all, which people forget: if the text is a
     * constant you wrote, let the compiler count. An empty [] reserves
     * exactly what is needed, terminator included. */
    char exact[] = "Eduardo Giacomelli";
    printf("\nexact[] = \"%s\" (%zu bytes, counted by the compiler)\n",
           exact, sizeof(exact));

    return 0;
}

/* ============================================================================
 * THE PRACTICAL SUMMARY
 *
 *     instead of             use
 *     ----------             ---
 *     strcpy(d, s)           snprintf(d, sizeof(d), "%s", s)
 *     strcat(d, s)           snprintf(d + strlen(d), sizeof(d) - strlen(d), ...)
 *     sprintf(d, ...)        snprintf(d, sizeof(d), ...)
 *     gets(d)                never. It was REMOVED from C11 for being that
 *                            bad. To read a line: fgets(d, sizeof(d), stdin)
 *
 * And the rule that generates all of them:
 *
 *     sizeof(destination) only works where the array was DECLARED (step 10).
 *     In a function that received `char *d`, sizeof(d) is 8. There the size
 *     has to arrive as a parameter, exactly like the `n` for an int array.
 *
 * EXPERIMENTE:
 *
 *  1. Print `name` byte by byte as in step 11 and confirm the '\0' is in the
 *     right place, in the last position. snprintf guarantees that; strncpy,
 *     the function that looks like the obvious choice, does NOT, which is
 *     why this step recommends snprintf instead.
 *
 *  2. Replace sizeof(name) with a hand-written 8. It works. Now change the
 *     declaration to char name[4] and run: the 8 is still there, lying, and
 *     ASan catches the overflow. Duplicated magic numbers are dormant bugs.
 *
 *  3. Write a function `void greet(char *dest, size_t size,
 *     const char *who)` that builds "hello, X" with snprintf. Call it with
 *     sizeof of the array back in main. This is the API shape of all of C:
 *     pointer plus size, always as a pair.
 *
 *  4. Read `man 3 snprintf` and find the description of the return value.
 *     Check it against what this program did. Learning to read the man pages
 *     is half of learning C.
 *
 * -> passo-14: variable lifetime, and the bug it causes
 * ========================================================================= */
