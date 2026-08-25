/* ============================================================================
 * STEP 5 - & and *, with the diagram alongside.
 *
 * Nothing here is hard. It is new notation for an idea you already rely on in
 * Python without seeing it: every variable lives somewhere, and that
 * somewhere has a number.
 *
 *     Ctrl+Shift+B      (or: make 05)
 *
 * This step breaks nothing. It is the foundation for steps 06 through 16.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* A variable is a BOX with an address.
     *
     *     address             contents
     *     0x7ffd1234    ->    [ 25 ]      <- the variable `age`
     *
     * Two separate things, and the entire confusion around pointers comes
     * from mixing them: the ADDRESS (where the box is) and the VALUE (what
     * is inside it). */
    int age = 25;

    /* & means "give me the address of". %p prints an address, in hex.
     * The (void *) cast is what %p formally expects. */
    printf("age  holds    %d\n",  age);
    printf("age  lives at %p\n", (void *) &age);

    /* A POINTER is a box like any other. What it happens to hold is an
     * address.
     *
     *     0x7ffd1234    ->  [ 25 ]              <- age
     *     0x7ffd9999    ->  [ 0x7ffd1234 ]      <- p, holding the ADDRESS
     *                                              of age
     *
     * Read `int *p` as: "p has type 'address of int'". */
    int *p = &age;

    printf("\np    holds    %p   <- the address of age\n", (void *) p);
    printf("p    lives at %p   <- p is a box too\n",       (void *) &p);

    /* A * in front of a pointer means "go to that address and read what is
     * there". The word for this is DEREFERENCING.
     *
     * Watch out for the overloaded symbol:
     *     int *p        in a DECLARATION -> "p is a pointer"
     *     *p            in USE           -> "the thing p points at"
     * Same character, two meanings. */
    printf("*p   holds    %d   <- what is at the address stored in p\n", *p);

    /* Writing through the pointer changes the original box. No copy is
     * involved: p points at age's box, and that is where we write. */
    *p = 30;
    printf("\nafter *p = 30, age is now %d\n", age);

    /* What step 02 hinted at: a pointer's size does not depend on what it
     * points at. An address is an address, 8 bytes on a 64-bit machine. */
    double d = 3.14;
    char   c = 'x';
    printf("\nsizeof(int *)    = %zu\n", sizeof(int *));
    printf("sizeof(double *) = %zu\n", sizeof(double *));
    printf("sizeof(char *)   = %zu\n", sizeof(char *));

    /* So what is the pointer's type for, if they are all 8 bytes?
     * To know HOW MANY bytes to read at *p, and how to interpret them.
     * `int *` reads 4 bytes as an integer. `double *` reads 8 as floating
     * point. The type is a reading instruction for the compiler, not
     * something stored in memory. Step 25 makes this literal. */
    double *pd = &d;
    char   *pc = &c;
    printf("*pd = %.2f   *pc = %c\n", *pd, *pc);

    /* A pointer that points at nothing has an agreed value: NULL.
     * Never leave a pointer uninitialised. A pointer full of garbage points
     * at some arbitrary address, and writing through it destroys whatever
     * lives there. */
    int *empty = NULL;
    printf("\nempty = %p (this is NULL)\n", (void *) empty);
    if (empty == NULL)
        printf("checking before use is the habit that saves you\n");

    return 0;
}

/* ============================================================================
 * THE COMPLETE DIAGRAM FOR THIS PROGRAM
 *
 *     0x7ffd1234  [ 30 ]            age    (was 25, changed through *p)
 *     0x7ffd9999  [ 0x7ffd1234 ]    p      holds the address of age
 *                    |
 *                    +-----> points back at the box above
 *
 *     &age  = 0x7ffd1234    the address of the box
 *      age  = 30            the contents of the box
 *        p  = 0x7ffd1234    the copy of that address, stored in p
 *       *p  = 30            the contents where p points
 *       &p  = 0x7ffd9999    the address of the POINTER's own box
 *
 * If you can read those five lines without hesitating, the next eleven steps
 * are consequences.
 *
 * EXPERIMENTE:
 *
 *  1. Run it twice. The addresses change. That is ASLR, a system protection:
 *     every run puts the program somewhere different in memory. An address is
 *     never something to memorise or to compare across runs. Step 33 shows
 *     where those numbers come from.
 *
 *  2. Add `printf("%d\n", *empty);` at the end. Run it. The sanitizer kills
 *     the program and says exactly where:
 *     "load of null pointer of type 'int'".
 *     Without the sanitizer this would just be "Segmentation fault".
 *
 *  3. Point two pointers at the same box (`int *q = &age;`) and change it
 *     through one (`*q = 99;`). Read it through the other (`*p`). One box,
 *     two names. That is precisely what happens between threads, and it is
 *     why threads need a mutex.
 *
 *  4. Try `int *wrong = age;` (no &). gcc complains about an
 *     integer-to-pointer conversion. Read the message slowly: it is saying
 *     you tried to use the VALUE 30 as if it were an ADDRESS.
 *
 * -> passo-06, where pointers stop being a curiosity and become necessary
 * ========================================================================= */
