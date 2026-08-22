/* ============================================================================
 * STEP 34 - printf is not how you talk to the kernel.
 *
 * printf is a library function. It formats text, puts it in a buffer that
 * belongs to your process, and only sometimes asks the kernel to write it
 * out. write() is a SYSTEM CALL: it crosses into the kernel every time.
 *
 * That difference explains the thing that has been quietly confusing you
 * since passo-09: why your printf output disappears when the program
 * crashes.
 *
 *     Ctrl+Shift+B      (or: make 34)
 *     make 34 | cat     run it again through a pipe, and compare
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* write, isatty, STDOUT_FILENO: man 2 write */

int main(int argc, char *argv[])
{
    /* write() takes a file descriptor, not a FILE *. Descriptors are small
     * integers the kernel uses to identify open files:
     *
     *     0  stdin      1  stdout      2  stderr
     *
     * No formatting, no buffering, no interpretation. Bytes go out now. */
    const char *raw = "1. write(2): straight to the kernel, no buffer\n";
    ssize_t written = write(STDOUT_FILENO, raw, strlen(raw));

    /* Always check the return. write can legally write FEWER bytes than you
     * asked for, and on a pipe or socket it regularly does. Ignoring that is
     * a real bug in real programs. */
    if (written < 0) {
        perror("write");
        return 1;
    }

    printf("2. printf(3): formatted into a buffer first\n");

    /* WHICH BUFFERING MODE AM I IN?
     *
     * The C library picks by asking whether stdout is a terminal:
     *
     *   terminal  -> line buffered:  flushed at every \n
     *   pipe/file -> fully buffered: flushed when the 4 KiB buffer fills,
     *                                or at exit, or at fflush
     *   stderr    -> unbuffered, always. That is why error messages survive
     *                a crash and printf output does not.
     */
    printf("\nstdout is %s, so it is %s buffered\n",
           isatty(STDOUT_FILENO) ? "a terminal" : "NOT a terminal (pipe/file)",
           isatty(STDOUT_FILENO) ? "line" : "fully");

    /* THE DEMONSTRATION.
     *
     * Three lines go out by three routes, in this source order:
     *   a) printf, no newline    -> sits in the buffer
     *   b) write                 -> leaves immediately
     *   c) fprintf to stderr     -> leaves immediately (unbuffered)
     *
     * On a terminal you will see them out of order: b and c first, a only
     * when something flushes it. */
    printf("\n--- ordering test ---\n");
    printf("(a) printf with no newline, so it waits");

    /* strlen, not a hand-counted 20. The same rule as passo-13: never write
     * a length that has to be kept in sync with a literal by hand. */
    const char *b = "\n(b) write, out now\n";
    write(STDOUT_FILENO, b, strlen(b));

    fprintf(stderr, "(c) stderr, always out now\n");
    fflush(stdout);
    printf("\n(a) was flushed by the explicit fflush just now\n");

    /* THE CRASH CASE, which is passo-09 explained.
     *
     * If the process dies without flushing, whatever is in the buffer is
     * lost. It was never given to the kernel, so there is nothing on disk or
     * on the terminal to recover. The line DID run. Its output did not
     * survive.
     *
     * abort(), _exit(), a segfault and a sanitizer abort all skip the
     * flush. return from main and exit() do not: they run the cleanup. */
    printf("\n--- what a crash loses ---\n");
    if (argc > 1 && strcmp(argv[1], "crash") == 0) {
        fprintf(stderr, "stderr: about to abort\n");
        printf("stdout: THIS LINE IS IN THE BUFFER AND WILL BE LOST");
        abort();     /* skips the exit-time flush, exactly like a segfault */
    }
    printf("run:  ./passo-34-syscall-e-buffer crash | cat\n");
    printf("and the buffered line before the abort will be missing,\n");
    printf("while the stderr line right before it survives.\n");
    printf("Use write(2), or fflush, or stderr, when you are debugging a\n");
    printf("crash. This is why printf debugging lies to you.\n");

    /* Turning buffering off entirely, when you want printf to behave like
     * write. setvbuf must be called before anything is written to the
     * stream. */
    printf("\n--- unbuffered mode ---\n");
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("stdout is now unbuffered; every printf is its own write(2)\n");
    printf("which is correct and slow: measure it before shipping it.\n");

    return 0;
}

/* ============================================================================
 * COUNT THE SYSCALLS YOURSELF
 *
 * strace shows every crossing into the kernel. This is the single best way
 * to understand what a library function is doing on your behalf.
 *
 *     make 34
 *     strace -e trace=write ./passo-34-syscall-e-buffer
 *     strace -e trace=write ./passo-34-syscall-e-buffer | cat
 *
 * Compare the two. Through a pipe, the many printf calls collapse into far
 * fewer write() syscalls, because the buffer fills up before it is handed
 * over. On a terminal, each newline forces one.
 *
 * A syscall costs a context switch into the kernel and back, on the order of
 * hundreds of nanoseconds. Buffering exists to turn a thousand of them into
 * one. That is all stdio is for.
 *
 *     strace -c ./passo-34-syscall-e-buffer     summary by syscall
 *     strace -f ./prog                          follow threads too
 *
 * `-f` matters for PPD: without it you only see the main thread.
 *
 * THE LAYERS, FROM YOUR CODE DOWN
 *
 *     printf("%d\n", x)          libc: format into a buffer
 *       -> fwrite / internal
 *          -> write(1, buf, n)   libc wrapper, still user space
 *             -> syscall         the actual instruction, enters the kernel
 *                -> the tty driver or the filesystem
 *
 * `man 3 printf` documents the library function. `man 2 write` documents the
 * system call. That section number is the layer, and knowing which one you
 * are reading is most of knowing where to look.
 *
 *     man 2 intro     the system call interface
 *     man 3 intro     the C library
 *     man 7 pthreads  the whole threads API in one page
 *
 * WHY THIS SHOWS UP AGAIN WITH THREADS
 *
 * All threads in a process share one stdout and one buffer. Two threads
 * calling printf at the same time do not interleave mid-line, because glibc
 * locks the stream for you, but the ORDER between them is whatever the
 * scheduler decided. That is exactly what the pthreads exercises ask you to
 * observe, and it is a property of the scheduler, not of printf.
 *
 * It also means printf is a synchronisation point in disguise: it takes a
 * lock. A printf inside a hot loop can hide a race by serialising the
 * threads, which is experiment 3 of the pthreads passo-12.
 *
 * EXPERIMENTE:
 *
 *  1. Run the program normally, then piped through `cat`. The isatty line
 *     changes, and the ordering test changes with it.
 *
 *  2. `strace -e trace=write ./passo-34-syscall-e-buffer | cat 2>/dev/null`
 *     and count the write syscalls. Then set _IONBF at the top of main and
 *     count again.
 *
 *  3. Write the crash case yourself:
 *
 *         printf("this line is lost");   // no newline
 *         abort();
 *
 *     Run it piped through cat: nothing appears. Add fflush(stdout) before
 *     the abort and it does. Now reread the passo-09 footer.
 *
 *  4. `strace -c ls` and look at the summary. Then `strace ls 2>&1 | head -40`
 *     and find the openat, the mmap of libc, and the first write. That is a
 *     program starting up, and you can read all of it now.
 *
 * -> back to "00 - COMECE AQUI.md"
 * ========================================================================= */
