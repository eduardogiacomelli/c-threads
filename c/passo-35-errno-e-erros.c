/* ============================================================================
 * STEP 35 - errno, and how C functions report failure.
 *
 * C has no exceptions. A function that can fail has to tell you in its
 * return value, and there are exactly three conventions in wide use. Mixing
 * them up is how error handling quietly stops working.
 *
 *     Ctrl+Shift+B      (or: make 35)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strerror */
#include <errno.h>      /* errno, EINVAL, ENOENT, ERANGE: man 3 errno */
#include <limits.h>
#include <pthread.h>

/* ------------------------------------------------------------------ 1 --
 * THE THREE CONVENTIONS
 *
 *   a) return a sentinel, set errno       fopen -> NULL, malloc -> NULL,
 *                                         strtol -> sets ERANGE
 *   b) return the error code directly     pthread_create, pthread_join
 *   c) return 0 or -1, set errno          most system calls: open, write
 *
 * The trap is that (b) does NOT touch errno. Calling perror() after a
 * pthread function prints whatever unrelated failure happened last, which is
 * worse than printing nothing.
 */

static void demo_sentinel_and_errno(void)
{
    printf("1. sentinel return, errno holds the reason\n");

    /* Zero errno first. Nothing clears it for you, and a stale value from
     * some earlier call is indistinguishable from a fresh one. */
    errno = 0;
    FILE *f = fopen("/definitely/not/here", "r");

    /* CHECK THE RETURN VALUE FIRST. errno is only meaningful once you know
     * the call failed. A successful call may set errno to anything at all,
     * and is allowed to. */
    if (f == NULL) {
        printf("   fopen failed, errno = %d\n", errno);
        printf("   strerror(errno) = %s\n", strerror(errno));

        /* perror prints "your prefix: the message" to stderr. Convenient,
         * but it always goes to stderr and you cannot format around it. */
        perror("   perror says");
    } else {
        fclose(f);
    }
}

static void demo_errno_is_sticky(void)
{
    printf("\n2. errno is never cleared by success\n");

    errno = 0;
    FILE *bad = fopen("/definitely/not/here", "r");
    (void) bad;
    printf("   after a failed fopen:   errno = %d (%s)\n",
           errno, strerror(errno));

    FILE *good = fopen("/dev/null", "r");
    printf("   after a SUCCESSFUL one: errno = %d (%s)\n",
           errno, strerror(errno));
    printf("   still set. This is why the return value is the test,\n");
    printf("   and errno is only the explanation.\n");
    if (good)
        fclose(good);
}

static void demo_strtol(void)
{
    printf("\n3. strtol, which uses errno for one specific failure\n");

    /* passo-18 built this check. Here is the errno half of it. */
    const char *huge = "99999999999999999999999999";
    char *end;

    errno = 0;
    long v = strtol(huge, &end, 10);

    if (errno == ERANGE) {
        printf("   \"%s\"\n", huge);
        printf("   does not fit in a long: errno == ERANGE, value clamped\n");
        printf("   to LONG_MAX (%ld)\n", v);
        printf("   LONG_MAX is %ld\n", LONG_MAX);
    }
}

/* ------------------------------------------------------------------ 4 --
 * PTHREADS RETURN THE CODE. THEY DO NOT SET errno.
 */
static void *tiny_thread(void *arg)
{
    (void) arg;
    return NULL;
}

static void demo_pthread_convention(void)
{
    printf("\n4. pthreads: the code comes back in the return value\n");

    pthread_t t;
    int rc = pthread_create(&t, NULL, tiny_thread, NULL);

    if (rc != 0) {
        /* CORRECT: the code is in rc. */
        fprintf(stderr, "   pthread_create: %s\n", strerror(rc));
        return;
    }
    printf("   pthread_create returned %d (0 means success)\n", rc);

    rc = pthread_join(t, NULL);
    printf("   pthread_join returned   %d (0 means success)\n", rc);

    /* Now force a real failure. Note the choice: joining the same thread
     * twice would ALSO "fail", but that is undefined behaviour, not a
     * defined error, and AddressSanitizer aborts the program rather than
     * letting it return a code. A defined failure is one the standard
     * promises: a stack size below PTHREAD_STACK_MIN must give EINVAL. */
    errno = 0;                               /* prove pthreads leaves it alone */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    rc = pthread_attr_setstacksize(&attr, 1);

    printf("\n   pthread_attr_setstacksize(&attr, 1) returned %d\n", rc);
    printf("   strerror(rc) = %s\n", strerror(rc));
    printf("   and errno is still %d, because pthreads never touched it.\n",
           errno);
    printf("   PTHREAD_STACK_MIN on this machine is %d bytes.\n",
           PTHREAD_STACK_MIN);
    printf("   So: strerror(rc), NOT perror. perror reads errno, which\n");
    printf("   would print \"Success\" here and tell you nothing.\n");

    pthread_attr_destroy(&attr);
}

/* ------------------------------------------------------------------ 5 --
 * errno IS PER THREAD.
 *
 * It looks like a global and it is not. In glibc it expands to a function
 * call that returns a pointer into thread-local storage, so every thread has
 * its own. That is the only reason error handling works at all in a threaded
 * program: two threads failing at the same time do not overwrite each
 * other's reason.
 */
static void *set_errno_thread(void *arg)
{
    (void) arg;
    errno = 0;
    fopen("/definitely/not/here", "r");     /* sets this thread's errno */
    printf("   worker thread: &errno = %p, errno = %d\n",
           (void *) &errno, errno);
    return NULL;
}

static void demo_errno_per_thread(void)
{
    printf("\n5. errno is thread-local, not global\n");

    errno = EINVAL;      /* main's errno, set to something recognisable */
    printf("   main sets its own errno to %d (%s)\n", EINVAL, strerror(EINVAL));

    pthread_t t;
    int rc = pthread_create(&t, NULL, set_errno_thread, NULL);
    if (rc != 0) {
        fprintf(stderr, "   pthread_create: %s\n", strerror(rc));
        return;
    }
    pthread_join(t, NULL);

    printf("   main:          &errno = %p, errno = %d\n",
           (void *) &errno, errno);
    printf("   different addresses, and main's %d survived untouched.\n", errno);
}

/* ------------------------------------------------------------------ 6 --
 * THE CLEANUP PATTERN.
 *
 * With several allocations that can each fail, nested ifs become
 * unreadable. `goto cleanup` is the idiomatic answer in C, and it is one of
 * the very few places goto is not a smell: every exit path runs the same
 * teardown, in reverse order of acquisition.
 */
static int build_three(void)
{
    int   *a = NULL;
    int   *b = NULL;
    char  *c = NULL;
    int    status = -1;

    a = malloc(16 * sizeof *a);
    if (a == NULL) goto cleanup;

    b = malloc(16 * sizeof *b);
    if (b == NULL) goto cleanup;

    c = malloc(64);
    if (c == NULL) goto cleanup;

    snprintf(c, 64, "all three allocated");
    printf("   %s\n", c);
    status = 0;

cleanup:
    /* free(NULL) is defined and does nothing, so this needs no ifs. */
    free(c);
    free(b);
    free(a);
    return status;
}

int main(void)
{
    demo_sentinel_and_errno();
    demo_errno_is_sticky();
    demo_strtol();
    demo_pthread_convention();
    demo_errno_per_thread();

    printf("\n6. the goto cleanup pattern\n");
    printf("   build_three() returned %d\n", build_three());

    return 0;
}

/* ============================================================================
 * THE RULES
 *
 * 1. Test the RETURN VALUE. errno only explains a failure you already
 *    detected; it never tells you that one happened.
 *
 * 2. Set errno = 0 before a call whose only failure signal is errno.
 *    strtol is the usual case: it can legitimately return 0.
 *
 * 3. Read errno immediately. Any library call in between, including printf,
 *    is allowed to change it.
 *
 * 4. pthreads returns the code. Use strerror(rc). Never perror.
 *
 * 5. errno is thread-local. You measured that in section 5.
 *
 * strerror IS NOT THREAD SAFE
 *
 * It may return a pointer to a shared static buffer. Two threads calling it
 * at once can interleave. For threaded code use the reentrant version:
 *
 *     char buf[128];
 *     strerror_r(rc, buf, sizeof buf);     // needs _GNU_SOURCE or POSIX
 *
 * In practice glibc's strerror is fine for the constant messages, but the
 * habit is worth having, and `man 3 strerror` says so in the NOTES section.
 *
 * DESIGNING YOUR OWN
 *
 * Pick one convention per project and hold it. For anything more than
 * success or failure, return an int code and keep the real result in an out
 * parameter, which is the shape passo-18 used:
 *
 *     int read_positive(const char *text, long *out);
 *      -> 1 on success, 0 on failure, result written through `out`
 *
 * That way the caller cannot accidentally use a value that was never
 * produced, because the return value is not the value.
 *
 * EXPERIMENTE:
 *
 *  1. Delete `errno = 0;` from demo_strtol and call it after the failed
 *     fopen. It now reports ERANGE for an input that parsed fine, because
 *     errno was left over.
 *
 *  2. Put a printf between the failed fopen and the errno read. On glibc it
 *     usually survives, and it is not guaranteed to. Read errno first.
 *
 *  3. Replace strerror(rc) with perror("attr") in demo_pthread_convention.
 *     It prints "attr: Success", because errno was 0 and the real code was
 *     sitting in rc all along. That is the whole trap in one line.
 *
 *  3b. Try `pthread_join(t, NULL)` twice instead. That is undefined
 *     behaviour rather than a defined error, and ASan aborts with a CHECK
 *     failure inside its own interceptor. Worth seeing once: "it returns an
 *     error code" and "it is UB" are not the same thing, and only the
 *     manual can tell you which you are looking at.
 *
 *  4. `errno` is a macro. Prove it:
 *
 *         echo '#include <errno.h>
 *         int x = errno;' | cpp -P | tail -3
 *
 *     You will see it expand to a dereference of a function call, which is
 *     how it manages to be per thread.
 *
 *  5. `man 3 errno` lists every E-name. Skim it once; ENOENT, EACCES,
 *     EINVAL, ERANGE, EAGAIN and ENOMEM cover almost everything you will
 *     meet.
 *
 * -> passo-36
 * ========================================================================= */
