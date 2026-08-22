/* ============================================================================
 * STEP 26 - int m[3][4] and int *rows[3] are NOT the same thing.
 *
 * They index identically - m[i][j] either way - and they have completely
 * different memory layouts. Confusing them is the reason "why can't I pass my
 * 2D array to this function" is one of the most asked C questions.
 *
 *     Ctrl+Shift+B      (or: make 26)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>

#define ROWS 3
#define COLS 4

/* Taking a real 2D array: the INNER dimension is mandatory.
 *
 *     void f(int m[][COLS], size_t rows)
 *
 * The compiler needs COLS to compute m[i][j] - that is the whole address
 * calculation. The outer dimension it does not need, and would ignore if you
 * wrote it, because the array decays to a pointer to its first ROW.
 *
 * `int m[][4]` is exactly `int (*m)[4]`: pointer to an array of 4 ints. Read
 * it with the passo-20 rule and it stops being mysterious. */
static long sum_matrix(int m[][COLS], size_t rows)
{
    long total = 0;
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < COLS; j++)
            total += m[i][j];
    return total;
}

/* Taking an array of pointers: no inner dimension, because there is no inner
 * array - just addresses. You need the length of each row separately. */
static long sum_rows(int *rows[], size_t nrows, size_t ncols)
{
    long total = 0;
    for (size_t i = 0; i < nrows; i++)
        for (size_t j = 0; j < ncols; j++)
            total += rows[i][j];
    return total;
}

int main(void)
{
    /* ------------------------------------------------ a real 2D array */
    int m[ROWS][COLS] = {
        { 1,  2,  3,  4 },
        { 5,  6,  7,  8 },
        { 9, 10, 11, 12 },
    };

    printf("int m[3][4] - ONE contiguous block of %zu bytes\n", sizeof(m));
    printf("row addresses, 16 bytes apart (4 ints each):\n");
    for (size_t i = 0; i < ROWS; i++)
        printf("  m[%zu] at %p\n", i, (void *) m[i]);

    /* Row-major: the whole thing is one flat run, rows laid end to end.
     *
     *   m[i][j]  ==  *(&m[0][0] + i * COLS + j)
     *
     * That identity is why the compiler must know COLS. */
    int *flat = &m[0][0];
    printf("\nflat walk: ");
    for (size_t k = 0; k < ROWS * COLS; k++)
        printf("%d ", flat[k]);
    printf("\nm[1][2] = %d, and flat[1*4+2] = %d - same box\n",
           m[1][2], flat[1 * COLS + 2]);

    printf("\nsum via m[][COLS] = %ld\n", sum_matrix(m, ROWS));

    /* --------------------------------------- an array of row pointers */
    /* Three separate allocations. The rows can be anywhere, they need not be
     * the same length, and there is no single block. */
    int *rows[ROWS];
    for (size_t i = 0; i < ROWS; i++) {
        rows[i] = malloc(COLS * sizeof(int));
        if (rows[i] == NULL) return 1;
        for (size_t j = 0; j < COLS; j++)
            rows[i][j] = (int) (100 + i * COLS + j);
    }

    printf("\nint *rows[3] - %zu bytes of POINTERS, plus 3 separate blocks\n",
           sizeof(rows));
    for (size_t i = 0; i < ROWS; i++)
        printf("  rows[%zu] holds %p\n", i, (void *) rows[i]);
    printf("  (heap addresses, and the gaps between them are not 16)\n");

    printf("\nsum via *rows[] = %ld\n", sum_rows(rows, ROWS, COLS));

    /* The one that catches everyone: you cannot pass `m` where `int **` is
     * expected. `m` decays to `int (*)[4]`, a pointer to an array - not a
     * pointer to a pointer. There is no array of row pointers anywhere in
     * `m` for it to decay into. Experiment 1 makes the compiler say it. */

    for (size_t i = 0; i < ROWS; i++)
        free(rows[i]);

    return 0;
}

/* ============================================================================
 * THE TWO LAYOUTS
 *
 *   int m[3][4]                          int *rows[3]
 *
 *   0x1000 [ 1][ 2][ 3][ 4]              0x2000 [ 0x5a10 ] --> [100][101][102][103]
 *   0x1010 [ 5][ 6][ 7][ 8]              0x2008 [ 0x5b80 ] --> [104][105][106][107]
 *   0x1020 [ 9][10][11][12]              0x2010 [ 0x5c30 ] --> [108][109][110][111]
 *
 *   one block, 48 bytes                  24 bytes of pointers + 3 blocks
 *   one allocation                       four allocations, four frees
 *   m[i][j] = one multiply and add       rows[i][j] = load a pointer, THEN index
 *
 * Which to use:
 *
 *   fixed, rectangular, known at compile time   ->  int m[R][C]
 *   ragged rows, or sizes known only at runtime ->  int *rows[]
 *
 * The flat block is also faster: one allocation, and consecutive rows are
 * already in cache. The pointer array costs an extra memory load per row and
 * scatters the data.
 *
 * YOU HAVE ALREADY MET THE SECOND ONE
 *
 *     int main(int argc, char *argv[])
 *
 * `char *argv[]` is an array of pointers to char - exactly this layout, with
 * strings of different lengths at the far end of each arrow. That is why the
 * strings can have different lengths, and why there is no inner dimension.
 *
 * FOR DIVIDING WORK BETWEEN THREADS
 *
 * A row-major matrix splits cleanly by rows: thread k gets rows
 * [k*rows/n, (k+1)*rows/n), and each thread touches a contiguous stretch of
 * memory that no other thread touches. That is the passo-09 slice in two
 * dimensions, and the contiguity is what makes it fast - threads working on
 * interleaved rows would keep invalidating each other's cache lines.
 *
 * EXPERIMENTE:
 *
 *  1. Try to call `sum_rows(m, ROWS, COLS)` - passing the 2D array where an
 *     array of pointers is expected. Read the error: "expected 'int **' but
 *     argument is of type 'int (*)[4]'". Now that you can read both types,
 *     the message says exactly what is wrong.
 *
 *  2. Print `sizeof(m)`, `sizeof(m[0])` and `sizeof(m[0][0])`: 48, 16, 4.
 *     Dividing them gives you the dimensions - and only works in the scope
 *     that declared the array (passo-10).
 *
 *  3. Make the rows ragged: allocate row 1 with only 2 ints, then call
 *     sum_rows with ncols = 4. The sanitizer catches the read past the end
 *     of that block. A ragged array needs its lengths carried alongside.
 *
 *  4. Walk `m` flat and time it against walking it column by column
 *     (`for j { for i { m[i][j] } }`) with a much bigger matrix. Same
 *     arithmetic, very different speed - that is the cache, and it is the
 *     first performance idea that matters in PPD.
 *
 * -> passo-27
 * ========================================================================= */
