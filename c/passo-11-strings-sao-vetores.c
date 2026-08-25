/* ============================================================================
 * STEP 11 - a C string is a char array terminated by '\0'.
 *
 * There is no string type. There is no .length. What there is, is a
 * convention: the string ends when a zero byte turns up. Every string
 * function in C trusts that convention, and it is the convention step 12
 * breaks.
 *
 *     Ctrl+Shift+B      (or: make 11)
 * ========================================================================= */

#include <stdio.h>
#include <string.h>     /* strlen, strcpy, strcmp: man 3 string */

int main(void)
{
    /* Two ways of writing the same thing. The first is what you use; the
     * second shows what it actually is. */
    char name[6]  = "Ana";
    char same[6]  = {'A', 'n', 'a', '\0', 0, 0};

    printf("name = %s\n", name);
    printf("same = %s\n", same);

    /* The '\0' (zero byte) is not the character '0'. It is the numeric value
     * 0, and it occupies a position in the array. "Ana" needs 4 bytes, not
     * 3. */
    printf("\nname[6], byte by byte:\n");
    for (size_t i = 0; i < 6; i++)
        printf("  name[%zu] = %3d  '%c'\n", i, name[i],
               name[i] ? name[i] : ' ');

    /* strlen COUNTS bytes until it finds the zero. It is a loop, costing
     * O(n). It is not a stored field like Python's len(). Calling strlen in
     * a loop condition walks the whole string on every iteration. */
    printf("\nstrlen(name)  = %zu  <- characters, the \\0 does not count\n",
           strlen(name));
    printf("sizeof(name)  = %zu  <- bytes reserved, the \\0 does count\n",
           sizeof(name));

    /* Two declarations that look alike and are not:
     *
     *   char v[] = "hi";    an ARRAY of your own, on the stack, holding a
     *                       copy of the text. You may write to it.
     *
     *   char *p  = "hi";    a POINTER to text stored in a READ-ONLY area of
     *                       the binary. Writing there kills the program,
     *                       which is why the correct spelling is
     *                       `const char *`. Step 33 shows the r-- permission
     *                       that enforces it.
     */
    char mine[] = "hi";
    const char *literal = "hi";

    mine[0] = 'H';                     /* fine: the array is yours */
    printf("\nmine    = %s  (can be changed)\n", mine);
    printf("literal = %s  (const: the compiler stops you changing it)\n",
           literal);

    /* Comparing strings with == compares ADDRESSES, not contents. It is
     * almost always a bug. Use strcmp, which returns 0 when they are equal.
     * (Yes: ZERO means equal. Read it as "zero difference".) */
    char a[] = "abc";
    char b[] = "abc";

    /* Written through pointers because `a == b` directly between two arrays
     * makes gcc warn ("comparison between two arrays"). With `char *` it
     * says nothing, and that is how the bug reaches real code. */
    char *pa = a;
    char *pb = b;
    printf("\npa == pb       ? %s  <- compares addresses: different boxes\n",
           pa == pb ? "yes" : "no");
    printf("strcmp(a,b)==0 ? %s  <- compares contents\n",
           strcmp(a, b) == 0 ? "yes" : "no");

    /* Copying is a function too, not `=`. `dest = src` between arrays does
     * not even compile (step 10). strcpy copies byte by byte UP TO AND
     * INCLUDING the '\0'. */
    char dest[10];
    strcpy(dest, "Ana");
    printf("\ndest after strcpy = %s\n", dest);

    return 0;
}

/* ============================================================================
 * THE DIAGRAM
 *
 *     char name[6] = "Ana";
 *
 *     0x7ffd1000  [ 'A' ]  65
 *     0x7ffd1001  [ 'n' ]  110
 *     0x7ffd1002  [ 'a' ]  97
 *     0x7ffd1003  [ \0  ]  0     <- the string ends HERE
 *     0x7ffd1004  [  0  ]        <- reserved, spare
 *     0x7ffd1005  [  0  ]
 *
 *     printf("%s") receives the address 0x7ffd1000 and prints byte by byte
 *     until it finds the zero. If the zero is not there, it keeps reading on
 *     into memory. That is step 12.
 *
 * THE ARITHMETIC YOU ALWAYS HAVE TO DO
 *
 *     text of N characters  ->  an array of at least N+1 bytes.
 *
 * Forgetting the +1 is the most common string bug there is.
 *
 * EXPERIMENTE:
 *
 *  1. Erase the '\0' by hand: `name[3] = 'X';` before the printf. Run it.
 *     printf no longer finds a terminator at name[3] and keeps reading
 *     through the rest of the array and past it. With luck ASan catches the
 *     out-of-bounds read. Without luck, garbage on screen.
 *
 *  2. Declare `char tight[3] = "Ana";`, three letters in three bytes with no
 *     room for a terminator. gcc allows it (with a warning). Print it with
 *     %s and see what happens.
 *
 *  3. Try writing to a literal:
 *
 *         char *p = "hi";     // no const, so the compiler allows it
 *         p[0] = 'H';
 *
 *     Segmentation fault immediately. That memory really is read only, and
 *     the operating system enforces it. This is why a literal should be
 *     written as `const char *`.
 *
 *  4. Print `strcmp("abc", "abd")` and `strcmp("abd", "abc")`. Negative and
 *     positive: it is alphabetical order, in the style of "a - b". Zero
 *     means equal.
 *
 * -> passo-12: overflowing a string
 * ========================================================================= */
