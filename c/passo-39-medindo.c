/* ============================================================================
 * STEP 39 - measuring: why the same arithmetic can be 20x slower.
 *
 * Two loops. Identical instruction counts, identical result, one is an order
 * of magnitude slower. The difference is not the code, it is where the code
 * touches memory.
 *
 * Then the same question for threads: why 8 threads does not give 8x.
 *
 *     make 39           a few seconds, most of it filling the matrix
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* 4096 x 4096 ints is 64 MB, which is twice the 32 MB L3 on this machine.
 * Working set size is the whole experiment: shrink this below the cache and
 * the effect disappears, which is itself worth trying. */
#define N       4096
#define THREADS 4

static int matrix[N][N];          /* .bss, so the file on disk stays small */

static double now(void)
{
    struct timespec t;
    /* CLOCK_MONOTONIC, never time(): it has second resolution, and it can
     * jump backwards when NTP adjusts the clock. */
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* ------------------------------------------------------------------ 1 --
 * SAME WORK, TWO ORDERS.
 *
 * Both loops read every element exactly once and add it up. The only
 * difference is which index moves fastest. */
static long sum_row_major(void)
{
    long total = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            total += matrix[i][j];    /* consecutive addresses */
    return total;
}

static long sum_column_major(void)
{
    long total = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            total += matrix[i][j];    /* 16 KB apart, every single step */
    return total;
}

/* ------------------------------------------------------------------ 2 --
 * SCALING: does more threads mean more speed?
 */
typedef struct { int from, to; long out; } Slice;

/* Memory-bound: almost no arithmetic, one memory touch per element. */
static void *memory_bound(void *arg)
{
    Slice *s = arg;
    long total = 0;
    for (int pass = 0; pass < 6; pass++)
        for (int i = s->from; i < s->to; i++)
            for (int j = 0; j < N; j++)
                total += matrix[i][j];
    s->out = total;
    return NULL;
}

/* Compute-bound: same loop shape, real arithmetic, no new memory traffic. */
static void *compute_bound(void *arg)
{
    Slice *s = arg;
    long total = 0;
    for (int i = s->from; i < s->to; i++)
        for (int j = 0; j < N; j++) {
            long v = (long) i * j;
            total += (v * v + 7) % 1000;
        }
    s->out = total;
    return NULL;
}

static double run(void *(*fn)(void *), int nthreads)
{
    pthread_t t[16];
    Slice slice[16];
    int chunk = N / nthreads;
    double start = now();

    for (int k = 0; k < nthreads; k++) {
        /* the last slice takes the remainder, step 18's warning */
        slice[k] = (Slice){ k * chunk, (k == nthreads - 1) ? N : (k + 1) * chunk, 0 };
        int rc = pthread_create(&t[k], NULL, fn, &slice[k]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            exit(1);
        }
    }
    for (int k = 0; k < nthreads; k++)
        pthread_join(t[k], NULL);

    return now() - start;
}

int main(void)
{
    printf("filling %d x %d ints (%zu MB)...\n",
           N, N, sizeof(matrix) / (1024 * 1024));
    srand(1);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            matrix[i][j] = rand() & 0xff;

    /* --------------------------------------------------------------- 1 */
    double t0 = now();
    long a = sum_row_major();
    double t1 = now();
    long b = sum_column_major();
    double t2 = now();

    printf("\n1. same additions, different order\n");
    printf("   row-major     %.4fs   sum = %ld\n", t1 - t0, a);
    printf("   column-major  %.4fs   sum = %ld\n", t2 - t1, b);
    printf("   column-major is %.1fx slower for the identical answer\n",
           (t2 - t1) / (t1 - t0));
    printf("\n   A cache line is 64 bytes = 16 ints. Row-major pays for one\n");
    printf("   line and uses all 16. Column-major pays for a line, uses one\n");
    printf("   int, and by the time it comes back the line is long gone.\n");

    /* --------------------------------------------------------------- 2 */
    printf("\n2. does adding threads help?\n");
    printf("   %-8s %14s %9s %16s %9s\n",
           "threads", "memory-bound", "speedup", "compute-bound", "speedup");

    double mem1 = 0, cpu1 = 0;
    for (int nt = 1; nt <= 8; nt *= 2) {
        double m = run(memory_bound, nt);
        double c = run(compute_bound, nt);
        if (nt == 1) { mem1 = m; cpu1 = c; }
        printf("   %-8d %14.4f %8.1fx %16.4f %8.1fx\n",
               nt, m, mem1 / m, c, cpu1 / c);
    }

    printf("\n   The compute-bound column keeps improving. The memory-bound\n");
    printf("   one stops: past a few threads they are all queueing for the\n");
    printf("   same memory bus, and the cores are idle waiting. More threads\n");
    printf("   cannot create more bandwidth.\n");

    return 0;
}

/* ============================================================================
 * MEASURED HERE (Ryzen 7 9700X, 8 cores / 16 threads, 32 MB L3, -O2)
 *
 *   row-major     0.0031s
 *   column-major  0.0716s          23x slower, same answer
 *
 *   threads   memory-bound  speedup   compute-bound  speedup
 *         1        0.0187     1.0x          0.0090     1.0x
 *         2        0.0102     1.8x          0.0045     2.0x
 *         4        0.0057     3.3x          0.0024     3.8x
 *         8        0.0065     2.9x          0.0023     4.0x
 *
 * Stable across three consecutive runs, so those are the shapes, not noise.
 *
 * Two lessons in one table. The memory-bound column peaks at 4 threads and
 * then goes BACKWARDS: 8 threads is slower than 4. The bottleneck moved from
 * the cores to the memory bus, and past that point extra threads only add
 * contention for a bus that cannot be widened.
 *
 * The compute-bound column keeps improving and then plateaus around 4x on an
 * 8-core part. It never reaches 8x because of thread startup cost, the
 * sequential fill at the beginning, and the fact that the two loops here are
 * not the only thing the machine is doing.
 *
 * "More threads is faster" is false in both columns past a point, and the
 * point is different for each. Only measurement tells you where it is.
 *
 * That is the honest answer to "from what size does threading pay off": it
 * depends on whether your threads are computing or waiting, and the only way
 * to know is to measure both.
 *
 * READING `time`
 *
 *     time ./passo-39-medindo
 *
 *     real  1.4s     wall clock. What the user experiences.
 *     user  4.9s     CPU time in your code, SUMMED over all threads
 *     sys   0.2s     CPU time in the kernel on your behalf
 *
 * user greater than real means real parallelism: 4.9s of work done in 1.4s
 * of wall time is roughly 3.5 cores busy. user well below real means you
 * were waiting for something, usually I/O or a lock. That one comparison
 * diagnoses most performance questions before you open any other tool.
 *
 * perf, AND WHY IT IS PROBABLY LOCKED ON YOUR MACHINE
 *
 *     perf stat ./passo-39-medindo
 *
 * On this machine that prints "<not supported>" for every hardware counter,
 * because:
 *
 *     cat /proc/sys/kernel/perf_event_paranoid
 *     4
 *
 * At 4, unprivileged users get nothing. To enable it for the session:
 *
 *     sudo sysctl kernel.perf_event_paranoid=1
 *
 * Then the interesting run becomes:
 *
 *     perf stat -e cycles,instructions,cache-references,cache-misses \\
 *         ./passo-39-medindo
 *
 * and the numbers that matter are:
 *
 *     instructions per cycle (IPC)   above 2 is good, below 1 means stalling
 *     cache-misses / cache-references   the miss rate
 *
 * The column-major loop executes the SAME number of instructions as the
 * row-major one and takes 23x longer, so its IPC is 23x worse. perf shows
 * you that directly instead of making you infer it from a stopwatch.
 *
 *     perf stat -r 5 ./prog        repeat 5 times, report mean and stddev
 *     perf record ./prog && perf report      which function, which line
 *
 * Until you enable it, clock_gettime and `time` answer most questions, which
 * is what this file uses.
 *
 * THE RULES OF MEASURING
 *
 *   1. Measure, never guess. This file exists because the guess is wrong.
 *   2. -O0 timings are meaningless. Benchmark what you will ship, at -O2.
 *   3. Sanitizers make everything 2-20x slower and distort the ratios.
 *      Time the plain build.
 *   4. Run it several times. One number is not a measurement.
 *   5. Make sure the compiler cannot delete your benchmark. If the result
 *      is unused, the loop disappears (step 37, step 38 experiment 2).
 *   6. Correct first. A fast wrong answer is worth nothing.
 *
 * EXPERIMENTE:
 *
 *  1. Drop N to 1024. The matrix is now 4 MB, it fits in L3, and the 23x
 *     collapses to almost nothing. The bug was never in the loop; it was in
 *     the working set.
 *
 *  2. Time the sanitized build against the plain one:
 *
 *         make 39
 *         gcc -std=gnu17 -O2 -pthread passo-39-medindo.c -o /tmp/fast
 *         time /tmp/fast
 *
 *     Rule 3, priced.
 *
 *  3. Look at `time`'s three numbers for the threaded section and work out
 *     roughly how many cores were busy.
 *
 *  4. Enable perf with the sysctl above and rerun with cache-misses. Compare
 *     the miss rate of the two loops. This is the step where the hardware
 *     stops being an abstraction.
 *
 *  5. Find the peak yourself: the loop tries 1, 2, 4 and 8. Add 3, 6 and 16
 *     and locate exactly where the memory-bound column turns around. That
 *     number is a property of this machine, not of the program.
 *
 *  6. Add a worker that writes rather than reads (matrix[i][j] = 0)
 *     and see whether it scales like the memory-bound or the compute-bound
 *     one. Writes are more expensive: the cache line has to be owned
 *     exclusively first, which is the same effect that made _Atomic costly
 *     in step 38.
 *
 * -> passo-40
 * ========================================================================= */
