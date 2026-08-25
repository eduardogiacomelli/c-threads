/* ============================================================================
 * STEP 2 - every value has a fixed size in bytes.
 *
 * In Python an int grows until memory runs out. In C an int is a 4-byte box,
 * and whatever does not fit is lost. That one sentence explains half the bugs
 * in the language.
 *
 *     Ctrl+Shift+B      (or: make 02)
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* Each declaration reserves a known amount of memory and writes a value
     * into it. The type tells the compiler TWO things: how many bytes to
     * reserve, and how to interpret those bytes. */

    char   letter  = 'A';       /* 1 byte.  SINGLE quotes = one character.   */
    int    age     = 25;        /* 4 bytes. The default integer.             */
    long   big     = 25L;       /* 8 bytes on 64-bit Linux.                  */
    float  height  = 1.75f;     /* 4 bytes. Poor precision, avoid.           */
    double precise = 1.75;      /* 8 bytes. Use this one for decimals.       */

    /* sizeof is an OPERATOR, not a function: the compiler works it out while
     * compiling and writes the number straight into the binary. Nothing is
     * computed at run time. It yields a special type, size_t, whose format
     * specifier is "%zu". */
    printf("char   takes %zu byte  and holds %c\n",  sizeof(char),   letter);
    printf("int    takes %zu bytes and holds %d\n",  sizeof(int),    age);
    printf("long   takes %zu bytes and holds %ld\n", sizeof(long),   big);
    printf("float  takes %zu bytes and holds %f\n",  sizeof(float),  height);
    printf("double takes %zu bytes and holds %f\n",  sizeof(double), precise);

    /* 'A' is literally the number 65. A char IS a small integer; the only
     * difference is how printf decides to show it. One box, two readings. */
    printf("\n'A' as a character: %c | as a number: %d\n", letter, letter);
    printf("'A' + 1 = %c\n", letter + 1);

    /* The limit of a 4-byte box: an int reaches 2147483647.
     * <limits.h> has these as constants; the number is spelled out here so
     * you can see it. */
    printf("\nlargest possible int: %d\n", 2147483647);

    return 0;
}

/* ============================================================================
 * WHAT HAPPENED
 *
 * Memory is a tape of bytes. Declaring a variable reserves a stretch of that
 * tape and gives it a name:
 *
 *     letter  [A]                           1 byte
 *     age     [25][ 0][ 0][ 0]              4 bytes
 *     big     [25][ 0][ 0][ 0][0][0][0][0]  8 bytes
 *
 * The name exists only for you and the compiler. Only addresses survive into
 * the binary. Turn on the byte view in the visualiser to see this laid out.
 *
 * EXPERIMENTE:
 *
 *  1. Add `int huge = 2147483647 + 1;` and print it with %d.
 *     gcc warns while compiling ("integer overflow"), and the value comes
 *     out negative: the box filled up and the leftover bit landed in the
 *     sign bit. In Python this simply does not happen.
 *
 *  2. Make the overflow happen at run time instead, to escape the warning:
 *
 *         int x = 2147483647;
 *         x = x + 1;
 *         printf("%d\n", x);
 *
 *     Now UndefinedBehaviorSanitizer catches it as you run, with a file and
 *     a line: "signed integer overflow". That is what it is there for.
 *
 *  3. Change `char letter = 'A'` to double quotes: "A". gcc objects.
 *     Single quotes are one character (1 byte). Double quotes are a string
 *     (2 bytes here: the 'A' and the terminator). Different things, and
 *     step 11 is about exactly that.
 *
 *  4. Print the size of a pointer: `printf("%zu\n", sizeof(int *));`
 *     It is 8 on any 64-bit machine, whatever it points at. An address is an
 *     address. Keep that for step 05.
 *
 * -> passo-03, which is this step done wrong on purpose
 * ========================================================================= */
