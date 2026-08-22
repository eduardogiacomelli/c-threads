/* ============================================================================
 * STEP 22 — the fix: a generic function is told the size.
 *
 * One swap that works for int, double, char, a struct, anything — because it
 * never pretends to know what the bytes mean. It only moves them.
 *
 *     Ctrl+Shift+B      (or: make 22)
 * ========================================================================= */

#include <stdio.h>
#include <string.h>     /* memcpy: man 3 memcpy */

typedef struct {
    char name[8];
    int  score;
} Player;

/* THE FIX is the third parameter.
 *
 *   void *a, void *b   two addresses, type deliberately forgotten
 *   size_t size        how many bytes live there — the piece `void *` lost
 *
 * Note `size_t`, not `int`: it is the unsigned type the language uses for
 * sizes, it is what `sizeof` gives you, and it cannot be negative. Using int
 * here would invite the bug in passo-23. */
void swap(void *a, void *b, size_t size)
{
    /* unsigned char is THE type for "raw byte" in C:
     *   - exactly 1 byte, guaranteed by the standard
     *   - no sign, so no surprises above 127
     *   - the one type you are explicitly allowed to alias any object with
     *
     * Casting void * to unsigned char * lets us do pointer arithmetic in
     * single bytes (passo-10: p + 1 advances by sizeof(*p)). */
    unsigned char *pa = (unsigned char *) a;
    unsigned char *pb = (unsigned char *) b;

    for (size_t i = 0; i < size; i++) {
        unsigned char temp = pa[i];
        pa[i] = pb[i];
        pb[i] = temp;
    }
}

/* Same job with memcpy, which is what you would actually write: the library
 * version is vectorised and will beat your loop. The scratch buffer is why
 * the loop above is still worth understanding — this one needs somewhere to
 * put the bytes, and picking a fixed size is a limit you must document. */
void swap_fast(void *a, void *b, size_t size)
{
    unsigned char temp[64];
    if (size > sizeof(temp)) {
        fprintf(stderr, "swap_fast: %zu bytes is over the %zu limit\n",
                size, sizeof(temp));
        return;                      /* refuse rather than corrupt */
    }
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

/* A generic printer needs the same treatment plus one more thing: it has to
 * be told how to INTERPRET the bytes, which is a job for a function pointer
 * (passo-20). This is exactly the shape of qsort's comparator. */
void print_all(const void *base, size_t count, size_t size,
               void (*show)(const void *))
{
    const unsigned char *p = (const unsigned char *) base;
    for (size_t i = 0; i < count; i++) {
        show(p + i * size);          /* the arithmetic sizeof lost, done by hand */
        printf(i + 1 < count ? ", " : "\n");
    }
}

static void show_int(const void *v)    { printf("%d", *(const int *) v); }
static void show_double(const void *v) { printf("%.2f", *(const double *) v); }
static void show_player(const void *v)
{
    const Player *p = (const Player *) v;
    printf("%s:%d", p->name, p->score);
}

int main(void)
{
    int a = 10, b = 20;
    swap(&a, &b, sizeof(a));
    printf("int:     a=%d b=%d\n", a, b);

    double p = 1.5, q = 3.14159;
    swap(&p, &q, sizeof(p));
    printf("double:  p=%.5f q=%.5f   <- all 8 bytes this time\n", p, q);

    char c = 'A', d = 'B';
    swap(&c, &d, sizeof(c));
    printf("char:    c=%c d=%c\n", c, d);

    /* Whole structs, 12 bytes each, and the function needed no changes. */
    Player one = { "Ana", 90 };
    Player two = { "Bruno", 75 };
    swap(&one, &two, sizeof(one));
    printf("struct:  %s=%d  %s=%d\n", one.name, one.score, two.name, two.score);

    swap_fast(&a, &b, sizeof(a));
    printf("\nmemcpy version: a=%d b=%d\n", a, b);

    /* Always sizeof(the thing), never a number you worked out yourself. If
     * Player grows a field tomorrow, every one of these calls stays correct. */
    int    ints[]    = { 3, 1, 4, 1, 5 };
    double doubles[] = { 2.5, 0.5 };
    Player squad[]   = { { "Ana", 90 }, { "Bruno", 75 }, { "Carla", 88 } };

    printf("\nints:    ");
    print_all(ints, 5, sizeof(ints[0]), show_int);
    printf("doubles: ");
    print_all(doubles, 2, sizeof(doubles[0]), show_double);
    printf("players: ");
    print_all(squad, 3, sizeof(squad[0]), show_player);

    return 0;
}

/* ============================================================================
 * THE SHAPE OF EVERY GENERIC FUNCTION IN C
 *
 *     void f(void *data, size_t count, size_t size, callback);
 *            \_______/   \___________________/      \______/
 *             where       how much                   what to do
 *
 * Compare with the real thing:
 *
 *     void qsort(void *base, size_t nmemb, size_t size,
 *                int (*compar)(const void *, const void *));
 *
 * You can now read that whole declaration, including the comparator. Try
 * `man 3 qsort` — it should look ordinary rather than cryptic.
 *
 * WHY `const void *` IN THE CALLBACKS
 *
 * `const` on a parameter is a promise to the caller: this function will not
 * write through that pointer. The compiler enforces it. It costs nothing and
 * it documents the function better than a comment, so put it on every
 * pointer parameter you only read from.
 *
 * AND WHERE THIS GOES IN PPD
 *
 * `pthread_create` hands your thread a single `void *`. Same erasure, same
 * cure: you know what you passed, so you cast it back. The difference is
 * that with a struct you rarely need the size — you cast to `Task *` and the
 * type tells the compiler everything. The size only becomes your problem
 * when the data is genuinely of unknown type, as here.
 *
 * EXPERIMENTE:
 *
 *  1. Call `swap(&a, &b, sizeof(double))` on two ints. You lied about the
 *     size; the sanitizer catches you reading and writing past both. The
 *     size argument is trust, not proof — the compiler cannot check it.
 *
 *  2. Delete `sizeof` and pass 4 by hand everywhere. It still works. Now
 *     change Player to hold a `long score` and run again: the struct swap
 *     breaks and nothing warns you. This is the argument for sizeof in one
 *     experiment.
 *
 *  3. Call swap_fast with a 100-byte struct. It refuses and says why,
 *     instead of smashing the stack. Compare that with what strcpy does in
 *     passo-12 — a function that cannot refuse is a function that will
 *     corrupt.
 *
 *  4. Write `show_hex` that prints any object as raw bytes, and pass it to
 *     print_all with the players. You have just written a debugger.
 *
 * -> passo-23
 * ========================================================================= */
