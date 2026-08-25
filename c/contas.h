/* ============================================================================
 * contas.h - the module's CONTRACT. Read it alongside passo-19.
 *
 * A header holds no code that runs. It holds DECLARATIONS: the signatures of
 * the functions another file may call. It is the list of what this module
 * offers, and it is the only part other files ever see.
 *
 * What belongs here: what others need to know (signatures, structs,
 *                    #define, typedef)
 * What belongs in the .c: how any of it actually works
 * ========================================================================= */

/* INCLUDE GUARD. Without these three lines, a header included twice (once
 * directly and once through another header) makes the compiler see the same
 * declarations twice.
 *
 * The name is convention: the file's, in capitals, with an underscore.
 * The first time, CONTAS_H is not defined, so it defines it and carries on.
 * The second time it is already defined, so the preprocessor skips
 * everything up to the #endif. */
#ifndef CONTAS_H
#define CONTAS_H

/* Adds every integer from 1 to n. Returns 0 when n < 1. */
long sum_to(int n);

/* 1 if n is even, 0 if odd. */
int is_even(int n);

/* How many times sum_to has been called since the program started.
 * (It exists only to show how a module keeps private state.) */
int call_count(void);

#endif /* CONTAS_H */
