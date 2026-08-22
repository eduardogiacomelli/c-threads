/* ============================================================================
 * STEP 38 - _Atomic: what volatile could never give you.
 *
 * Step 37 ended on a promise: volatile is about the compiler, and gives you
 * no atomicity. This file measures exactly that, then fixes it three ways
 * and prices each fix.
 *
 *     make 38           takes about a fifth of a second
 *
 * Four threads, a million increments each. The right answer is 4000000.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>         /* strerror */
#include <stdatomic.h>      /* _Atomic, atomic_fetch_add */

#define THREADS  4
#define PER      1000000

/* Three counters, three strategies.
 *
 * `volatile` is here as the control, not as a suggestion. It forces the
 * load-add-store to actually happen every iteration (without it, gcc at -O2
 * hoists the whole loop into a single add and the race disappears), which is
 * precisely what makes the lost updates visible. */
static volatile long  counter_volatile = 0;
static _Atomic  long  counter_atomic   = 0;
static          long  counter_guarded  = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);      /* never time(), see step 39 */
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* WRONG. Three machine operations with no protection at all. */
static void *work_volatile(void *arg)
{
    (void) arg;
    for (int i = 0; i < PER; i++)
        counter_volatile++;
    return NULL;
}

/* RIGHT, and cheap. On x86-64 this compiles to a single `lock addq`. */
static void *work_atomic(void *arg)
{
    (void) arg;
    for (int i = 0; i < PER; i++)
        counter_atomic++;                    /* the ++ is atomic now */
    return NULL;
}

/* RIGHT, and general. A mutex protects any amount of code, not one variable,
 * which is why it costs more. */
static void *work_mutex(void *arg)
{
    (void) arg;
    for (int i = 0; i < PER; i++) {
        pthread_mutex_lock(&lock);
        counter_guarded++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static double run(void *(*fn)(void *))
{
    pthread_t t[THREADS];
    double start = now();

    for (int i = 0; i < THREADS; i++) {
        int rc = pthread_create(&t[i], NULL, fn, NULL);
        if (rc != 0) {                       /* step 35: the code is in rc */
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            exit(1);
        }
    }
    for (int i = 0; i < THREADS; i++)
        pthread_join(t[i], NULL);

    return now() - start;
}

int main(void)
{
    long expected = (long) THREADS * PER;

    double t_vol = run(work_volatile);
    double t_ato = run(work_atomic);
    double t_mtx = run(work_mutex);

    printf("%d threads x %d increments, expected %ld\n\n",
           THREADS, PER, expected);
    printf("  %-10s %9s  %12s  %s\n", "strategy", "time", "result", "lost");
    printf("  %-10s %9.4f  %12ld  %ld\n", "volatile", t_vol,
           counter_volatile, expected - counter_volatile);
    printf("  %-10s %9.4f  %12ld  %ld\n", "_Atomic", t_ato,
           (long) counter_atomic, expected - (long) counter_atomic);
    printf("  %-10s %9.4f  %12ld  %ld\n", "mutex", t_mtx,
           counter_guarded, expected - counter_guarded);

    printf("\n  _Atomic is %.1fx the volatile time, mutex is %.1fx\n",
           t_ato / t_vol, t_mtx / t_vol);
    printf("  and only the volatile one is wrong.\n");

    /* The explicit form, which is what ++ expands to. Worth knowing because
     * it is the only way to ask for a weaker memory order. */
    atomic_fetch_add(&counter_atomic, 1);
    atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
    printf("\n  atomic_fetch_add brings it to %ld\n", (long) counter_atomic);

    /* Read and write need no ceremony either: on an _Atomic variable, plain
     * assignment and plain reads are already atomic and sequentially
     * consistent. */
    counter_atomic = 0;
    printf("  plain assignment to an _Atomic is atomic too: %ld\n",
           (long) counter_atomic);

    return 0;
}

/* ============================================================================
 * MEASURED ON THIS MACHINE (Ryzen 7 9700X, 4 threads, -O2)
 *
 *     strategy        time        result       lost
 *     volatile      0.0026     1074477    2925523
 *     _Atomic       0.0218     4000000          0
 *     mutex         0.1512     4000000          0
 *
 * Read the first row again. The volatile counter lost 73% of its updates.
 * Not a rare interleaving, not a stress test: three quarters of the work
 * vanished, on an ordinary run, on the first try. volatile forced the
 * memory traffic and prevented nothing.
 *
 * That is step 37's closing claim, priced.
 *
 * WHY IT IS FASTER TO BE WRONG
 *
 *     objdump -d on the three:
 *
 *     bump_plain (volatile long):
 *       mov    0x0(%rip),%rax          load
 *       add    $0x1,%rax               add
 *       mov    %rax,0x0(%rip)          store
 *
 *     bump_atomic (_Atomic long):
 *       lock addq $0x1,0x0(%rip)       one instruction
 *
 * Three instructions against one, and the one is slower. The `lock` prefix
 * tells the CPU to make that read-modify-write indivisible, which means
 * taking exclusive ownership of the cache line. When four cores fight over
 * the same line, that ownership bounces between them on every increment.
 *
 * The cost is not the instruction. It is the contention.
 *
 * MEMORY ORDER, BRIEFLY
 *
 *     counter++                                   sequentially consistent
 *     atomic_fetch_add(&c, 1)                     same thing, spelled out
 *     atomic_fetch_add_explicit(&c, 1, memory_order_relaxed)
 *
 * The default, seq_cst, also guarantees ORDERING: other threads see your
 * atomic operations in the order you wrote them. `relaxed` gives atomicity
 * only, and is correct for a counter nobody uses as a signal.
 *
 * On x86-64 both compile to the same `lock addq`, because the hardware
 * already provides that ordering. On ARM they differ, and code that only
 * ever ran on x86 has a habit of breaking the first time it meets an M1 or
 * a Graviton. Do not reach for relaxed until you can say what you are giving
 * up.
 *
 * WHICH TO USE
 *
 *     one scalar, one operation          _Atomic. Simple and cheap.
 *     several variables that must agree  mutex. Atomics do not compose:
 *                                        two atomic updates are not one
 *                                        atomic update.
 *     a private per-thread accumulator   neither. Sum after the join.
 *
 * That third line is the one that matters most for the PPD exercises, and it
 * is the pattern the visualiser calls "split the work". Four threads
 * incrementing four separate longs and adding them up at the end beats every
 * row of the table above, because there is no shared line to fight over.
 * The fastest lock is the one you designed away.
 *
 *     _Atomic implies volatile and more. Never write both.
 *
 * EXPERIMENTE:
 *
 *  1. Run it three times. The volatile row changes every run; the other two
 *     never do. That is the definition of a race.
 *
 *  2. Take `volatile` off counter_volatile and rebuild at -O2 in the
 *     terminal. The lost count drops to 0 and the time to almost nothing:
 *     gcc turned the million increments into a single add. The race did not
 *     get fixed, it got optimised out of existence. Step 37, again.
 *
 *  3. Add a fourth strategy: a per-thread local `long sum` accumulated in
 *     the loop and added to a shared total once, under the mutex, at the
 *     end. Time it. It should beat the atomic version by a wide margin.
 *
 *  4. Change THREADS to 1 and rerun. The atomic and mutex costs mostly
 *     vanish, because there is no contention. Cost here is a function of how
 *     many cores want the same cache line, not of the instruction.
 *
 *  5. Look at the assembly yourself:
 *
 *         gcc -O2 -S -o - passo-38-atomics.c | grep -B2 -A2 "lock"
 *
 * -> passo-39
 * ========================================================================= */
