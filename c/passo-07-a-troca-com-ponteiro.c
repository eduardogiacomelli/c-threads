/* ============================================================================
 * STEP 7 - the fix: hand over the ADDRESS of the boxes.
 *
 *     Ctrl+Shift+B      (or: make 07)
 *
 * Three changes from step 06. Only three. Find all three before reading on.
 * ========================================================================= */

#include <stdio.h>

/* CHANGE 1 - the parameters are now ADDRESSES of int, not ints.
 * The function no longer receives "two numbers"; it receives "where two
 * numbers live". */
void swap(int *a, int *b)
{
    /* CHANGE 2 - every access goes through *.
     *
     *   a   is the address  (the box back in main)
     *   *a  is the contents (the number inside it)
     *
     * Writing `a = b` here would swap only the addresses inside this
     * function, which is step 06 again with one extra level of indirection.
     * What we want is to swap the CONTENTS. */
    printf("   [inside] received the addresses %p and %p\n",
           (void *) a, (void *) b);
    printf("   [inside] which hold %d and %d\n", *a, *b);

    int temp = *a;   /* read over in main and keep it here      */
    *a = *b;         /* write into main whatever the other holds */
    *b = temp;       /* write the saved value into main          */
}

int main(void)
{
    int x = 10;
    int y = 20;

    printf("before: x=%d y=%d\n", x, y);
    printf("(x lives at %p, y lives at %p)\n", (void *) &x, (void *) &y);

    /* CHANGE 3 - pass &x and &y, not x and y.
     * That & is the difference between "here is the value" and "here is the
     * key to my box, go ahead". */
    swap(&x, &y);

    printf("after:  x=%d y=%d   <- swapped for real\n", x, y);

    return 0;
}

/* ============================================================================
 * THE DIAGRAM
 *
 * The copying still happens. C did not change its rule. What gets copied now
 * is the ADDRESS, and a copy of an address points at the same place as the
 * original:
 *
 *     main:                        swap:
 *     0x7ffd1000 [ 10 ]  x   <---- 0x7ffd0900 [ 0x7ffd1000 ]  a
 *     0x7ffd1004 [ 20 ]  y   <---- 0x7ffd0908 [ 0x7ffd1004 ]  b
 *
 * `a` is indeed a new box, and it dies at the end of the function. But while
 * it exists, `*a` reaches main's box. A copy of your home address still
 * leads to your home.
 *
 * THIS IS THE ONLY WAY A C FUNCTION CAN CHANGE SOMETHING OUTSIDE ITSELF.
 * That is why C is full of & at call sites, and it is why scanf wants one:
 *
 *     scanf("%d", &n);      "here is where to put what the user types"
 *
 * You have just understood scanf. Forgetting that & is the classic beginner
 * bug: scanf receives the VALUE of n as if it were an address, and writes
 * into some arbitrary place in memory.
 *
 * EXPERIMENTE:
 *
 *  1. Inside swap, drop the asterisks: `int t = a; a = b; b = t;`.
 *     gcc complains about the types (an int receiving an int *) and no swap
 *     happens. You swapped the addresses inside the function, which is step
 *     06 all over again.
 *
 *  2. Call `swap(x, y)` without the &. gcc warns: "passing argument 1 makes
 *     pointer from integer without a cast". Run it anyway and watch the
 *     sanitizer kill the program: 10 was used as if it were an address.
 *
 *  3. Call `swap(&x, &x)`. Both pointers go to the same box. Trace on paper
 *     what happens to the value before you run it. Then run it.
 *
 *  4. Write `void zero_out(int *n)` that puts 0 in the box it is given, and
 *     call it with `zero_out(&x)`. Same pattern, one parameter. Repeat until
 *     it is automatic: pthread_create will ask exactly this of you.
 *
 * -> passo-08: arrays
 * ========================================================================= */
