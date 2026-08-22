/* ============================================================================
 * STEP 24 — bits: packing several yes/no answers into one integer.
 *
 * You have already used this API without seeing it:
 *
 *     open("f", O_WRONLY | O_CREAT | O_TRUNC);
 *     gcc -fsanitize=address,undefined
 *
 * Those pipes are not "or". They are bitwise OR, building one integer out of
 * several independent flags. Same idea in pthread attributes, permission
 * bits, and every hardware register you will ever touch.
 *
 *     Ctrl+Shift+B      (or: make 24)
 * ========================================================================= */

#include <stdio.h>

/* One bit per flag. `1u << n` puts a single 1 in position n:
 *
 *     1u << 0  =  0000 0001
 *     1u << 1  =  0000 0010
 *     1u << 2  =  0000 0100
 *
 * The `u` makes the literal unsigned. Shifting a signed 1 into the top bit
 * is undefined behaviour, and this is the habit that avoids ever thinking
 * about it. */
#define READ    (1u << 0)
#define WRITE   (1u << 1)
#define EXECUTE (1u << 2)
#define HIDDEN  (1u << 3)

/* Print the low 8 bits, most significant first — the order humans write
 * numbers in, which is the opposite of the byte view's address order. */
static void show_bits(const char *label, unsigned value)
{
    printf("%-22s ", label);
    for (int bit = 7; bit >= 0; bit--)
        printf("%u", (value >> bit) & 1u);
    printf("   (%2u)\n", value);
}

int main(void)
{
    unsigned flags = 0;
    show_bits("nothing set", flags);

    /* SET a flag: OR. Bits already on stay on, so this is safe to repeat. */
    flags |= READ;
    show_bits("flags |= READ", flags);

    flags |= WRITE;
    show_bits("flags |= WRITE", flags);

    /* Several at once — this is the `O_WRONLY | O_CREAT` you have seen. */
    flags |= EXECUTE | HIDDEN;
    show_bits("|= EXECUTE | HIDDEN", flags);

    /* TEST a flag: AND, then ask whether anything survived.
     *
     * The result is not 1, it is the mask itself (or 0). Compare against 0,
     * or use it directly as a truth value — never compare it against 1. */
    printf("\nflags & WRITE = %u -> %s\n",
           flags & WRITE, (flags & WRITE) ? "on" : "off");

    /* CLEAR a flag: AND with the inverse. ~ flips every bit, so ~WRITE is
     * all ones except position 1, and ANDing keeps everything but that. */
    flags &= ~WRITE;
    show_bits("flags &= ~WRITE", flags);

    /* TOGGLE: XOR. Twice returns you to the start. */
    flags ^= HIDDEN;
    show_bits("flags ^= HIDDEN", flags);
    flags ^= HIDDEN;
    show_bits("flags ^= HIDDEN again", flags);

    /* Shifting is multiplying and dividing by powers of two, and it is how
     * the byte view's arithmetic works (passo-02). */
    printf("\n5 << 1 = %u   (5 * 2)\n", 5u << 1);
    printf("5 << 3 = %u  (5 * 8)\n", 5u << 3);
    printf("40 >> 2 = %u  (40 / 4)\n", 40u >> 2);

    /* THE PRECEDENCE TRAP, and it is a famous one.
     *
     * `&` binds LOOSER than `==`. So this:
     *
     *     flags & HIDDEN == 0
     *
     * parses as `flags & (HIDDEN == 0)`, which is `flags & 0`, which is 0,
     * which is always false. The compiler warns, and the fix is parentheses.
     * When in doubt, parenthesise bitwise operators — always. */
    printf("\nwith parentheses:  (flags & HIDDEN) == 0 -> %s\n",
           ((flags & HIDDEN) == 0) ? "true" : "false");

    /* Counting set bits, the readable way. gcc also has __builtin_popcount,
     * which compiles to one instruction. */
    unsigned count = 0;
    for (unsigned tmp = flags; tmp; tmp >>= 1)
        count += tmp & 1u;
    printf("bits set: %u\n", count);

    return 0;
}

/* ============================================================================
 * THE FIVE OPERATIONS, AS A TABLE YOU WILL REREAD
 *
 *     set     flags |=  MASK
 *     clear   flags &= ~MASK
 *     toggle  flags ^=  MASK
 *     test    if (flags & MASK)
 *     extract (value >> shift) & mask
 *
 * And two things that are NOT the same:
 *
 *     &   bitwise AND, works on every bit of the operands
 *     &&  logical AND, gives 0 or 1, and short-circuits
 *
 * Writing `&&` where you meant `&` gives 1 instead of the mask; writing `&`
 * where you meant `&&` evaluates the right side even when the left is false,
 * which breaks `if (p && p->field)`.
 *
 * WHY IT SHOWS UP IN CONCURRENCY
 *
 * A flags word is the smallest thing several threads can want to change at
 * once. `flags |= READY` is read-modify-write — exactly the three operations
 * of passo-12, so two threads setting different bits can lose one of them.
 * Bit flags shared between threads need atomics, not just care. That is next
 * semester's material, but this is where the need comes from.
 *
 * EXPERIMENTE:
 *
 *  1. Write `flags & HIDDEN == 0` without the parentheses and read the
 *     warning gcc gives you. Then print both versions side by side.
 *
 *  2. Set the same flag twice. Nothing changes — `|=` is idempotent. Now
 *     toggle it twice and watch it come back. Idempotent versus involutive,
 *     and the difference matters when you are not sure how many times your
 *     code runs.
 *
 *  3. Print `~0u` and `~0u >> 1`. Then try `~0 >> 1` with a signed 0 and see
 *     the difference — right-shifting a negative signed value keeps the sign
 *     bit, which is why masks are always unsigned.
 *
 *  4. Pack two 4-bit values into one byte: `packed = (hi << 4) | lo`, then
 *     get them back with `>> 4` and `& 0x0f`. That is how colours, dates and
 *     network headers are stored.
 *
 * -> passo-25
 * ========================================================================= */
