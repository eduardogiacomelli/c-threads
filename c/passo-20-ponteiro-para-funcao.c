/* ============================================================================
 * STEP 20 — a function has an address too.
 *
 * You have been passing a function pointer since passo-01 of the pthreads
 * tutorial without knowing it:
 *
 *     pthread_create(&t, NULL, my_worker, arg);
 *                              ^^^^^^^^^
 *                              this is a function pointer
 *
 * This file is that mechanism on its own, with no threads in the way.
 *
 *     Ctrl+Shift+B      (or: make 20)
 *
 * NOTE: from here on the comments are in English. Steps 01-19 are in
 * Portuguese; ask if you want them translated so the folder matches.
 * ========================================================================= */

#include <stdio.h>

/* Three ordinary functions with the SAME shape: two ints in, one int out.
 * "Same shape" is the whole game — a pointer can only hold the address of a
 * function whose signature matches its type exactly. */
int add(int a, int b)      { return a + b; }
int subtract(int a, int b) { return a - b; }
int multiply(int a, int b) { return a * b; }

/* THE DECLARATION, read from the inside out:
 *
 *     int (*op)(int, int)
 *          ^^^^                op is a pointer
 *         ^    ^^^^^^^^^^      ...to a function taking (int, int)
 *     ^^^                      ...that returns int
 *
 * The parentheses around *op are not decoration. Without them:
 *
 *     int *op(int, int)        a FUNCTION returning int *
 *
 * Two completely different things. This is the single most misread
 * declaration in C, and experiment 1 makes the compiler say so.
 *
 * Here it is as a parameter: this function takes an operation and applies it.
 * That is a callback, and it is how you tell library code what to do without
 * the library knowing anything about your program. */
int apply(int x, int y, int (*op)(int, int))
{
    /* Call it like any function. You may also write (*op)(x, y) — identical.
     * The bare form works because a function name in an expression decays to
     * its address, exactly like an array name (passo-10). */
    return op(x, y);
}

int main(void)
{
    /* Assigning: no & needed. `add` on its own already means "the address of
     * add". Writing &add is legal and means the same thing. */
    int (*op)(int, int) = add;

    printf("op points at %p\n", (void *) op);
    printf("op(3, 4)  = %d\n", op(3, 4));

    /* Re-aim the pointer. Same variable, different behaviour. This is the
     * passo-05 idea — a pointer is a box holding an address — applied to code
     * instead of data. */
    op = multiply;
    printf("after op = multiply, op(3, 4) = %d\n\n", op(3, 4));

    /* Passing behaviour as an argument. */
    printf("apply(10, 3, add)      = %d\n", apply(10, 3, add));
    printf("apply(10, 3, subtract) = %d\n", apply(10, 3, subtract));

    /* AN ARRAY OF FUNCTION POINTERS — a dispatch table.
     * This replaces a switch, and it is how interpreters, state machines and
     * command handlers are usually built. The type is exactly the same as the
     * variable above, with [] added. */
    int (*table[3])(int, int) = { add, subtract, multiply };
    const char *names[3]      = { "add", "subtract", "multiply" };

    printf("\ndispatch table:\n");
    for (int i = 0; i < 3; i++)
        printf("  %-9s(6, 2) = %d\n", names[i], table[i](6, 2));

    /* typedef makes the type readable, and you will see this everywhere in
     * real code. Read it as: "BinaryOp is the name of the type 'pointer to a
     * function taking two ints and returning int'". */
    typedef int (*BinaryOp)(int, int);
    BinaryOp chosen = subtract;
    printf("\nvia typedef: chosen(9, 4) = %d\n", chosen(9, 4));

    return 0;
}

/* ============================================================================
 * WHY THIS MATTERS FOR PTHREADS
 *
 *     int pthread_create(pthread_t *thread,
 *                        const pthread_attr_t *attr,
 *                        void *(*start_routine)(void *),   <- LOOK
 *                        void *arg);
 *
 * Read the third parameter with the rule above:
 *
 *     void *(*start_routine)(void *)
 *            ^^^^^^^^^^^^^^                a pointer
 *           ^               ^^^^^^^^       ...to a function taking void *
 *     ^^^^^^                               ...returning void *
 *
 * That is why the thread function must be written EXACTLY as
 * `void *name(void *arg)` — the signature has to match the pointer type. Get
 * one word wrong and the compiler rejects the call, and the message is
 * confusing until you can read the declaration.
 *
 * And it is why you pass `my_worker` with no parentheses: with them you would
 * be CALLING it and passing the result.
 *
 * EXPERIMENTE:
 *
 *  1. Drop the parentheses: change the declaration to
 *     `int *op(int, int) = add;`. The compiler now thinks you are declaring a
 *     function and refuses to initialise it. Read the error — it is the same
 *     error you will get the day you mistype a pthread signature.
 *
 *  2. Write a function `int power(int, int)` and add it to the table. Note
 *     that nothing else has to change: the table does not care what the
 *     functions do, only that they have the right shape.
 *
 *  3. Try to put a function with a DIFFERENT shape in the table, for example
 *     `double divide(double, double)`. The compiler stops you. That check is
 *     the only safety net you get, so let it work for you.
 *
 *  4. Print `(void *) add` and `(void *) main`. Both are addresses, and both
 *     live in a completely different region from your variables — compare
 *     them with the address of a local. That region is the code segment, and
 *     it is read-only (see memoria.md).
 *
 * -> passo-21
 * ========================================================================= */
