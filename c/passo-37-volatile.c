/* ============================================================================
 * STEP 37 - volatile: "reload this, I mean it".
 *
 * The compiler is allowed to assume nothing changes a variable except the
 * code it can see. When something outside that code DOES change it, the
 * assumption becomes a bug, and the bug only appears with optimisation on.
 *
 * `volatile` withdraws the assumption. It is a small keyword with a narrow
 * job, and it is very widely misused, so the second half of this file is
 * about what it does NOT do.
 *
 *     Ctrl+Shift+B      (or: make 37)      takes about a second
 * ========================================================================= */

#include <stdio.h>
#include <signal.h>     /* signal, sig_atomic_t: man 7 signal-safety */
#include <unistd.h>     /* alarm, write, ssize_t */
#include <string.h>

/* THE CANONICAL CORRECT USE, and there are only a few.
 *
 *   volatile      the compiler must read it from memory every time, and must
 *                 not cache it in a register or optimise the access away
 *   sig_atomic_t  a type the standard guarantees can be read and written in
 *                 one uninterruptible step, so a signal cannot land halfway
 *
 * Together they are the ONLY variable type the C standard lets you share
 * between a signal handler and the rest of the program. */
static volatile sig_atomic_t stop_requested = 0;

/* A signal handler runs at a moment of the kernel's choosing, interrupting
 * whatever was executing. Almost nothing is safe to call in here: printf
 * takes a lock (step 34), and if the interrupted code already held that
 * lock, you deadlock inside your own handler.
 *
 * The list of functions that ARE safe is in `man 7 signal-safety`. write()
 * is on it. printf is not.
 *
 * Best practice, and what this does: set a flag, return, let the main loop
 * deal with it. */
static void on_alarm(int sig)
{
    (void) sig;
    stop_requested = 1;

    const char *msg = "  [handler] SIGALRM: flag set\n";

    /* Step 34 said always check what write returns, and that still holds.
     * The difference here is what you can DO about a failure: nothing. You
     * cannot print an error from inside a handler, and there is nobody to
     * report to. So capture it and discard it EXPLICITLY, which also stops
     * gcc warning about the ignored result at -O2. Silence by accident and
     * silence on purpose look the same to the reader; only one of them is. */
    ssize_t written = write(STDERR_FILENO, msg, strlen(msg));
    (void) written;
}

int main(void)
{
    printf("counting until a signal arrives...\n");

    if (signal(SIGALRM, on_alarm) == SIG_ERR) {
        perror("signal");
        return 1;
    }
    alarm(1);                                   /* SIGALRM in one second */

    /* The loop reads stop_requested every single iteration, because it is
     * volatile. Take the keyword away and at -O2 the compiler is entitled to
     * read it once, see 0, and turn this into `while (1)`. It has looked at
     * the loop body, seen nothing that writes to the variable, and concluded
     * the value cannot change. It is not wrong about the code it can see. */
    unsigned long spins = 0;
    while (!stop_requested)
        spins++;

    printf("stopped after %lu spins\n", spins);

    /* ------------------------------------------------------------------ */
    printf("\nWHAT volatile DOES NOT DO\n");
    printf("  it does NOT make an operation atomic\n");
    printf("  it does NOT order accesses against other variables\n");
    printf("  it does NOT publish a write to another CPU core\n");
    printf("\n  volatile x++ is still load, add, store (pthreads step 12).\n");
    printf("  Two threads can still lose an update. volatile is about the\n");
    printf("  COMPILER, not about the hardware or about other threads.\n");

    return 0;
}

/* ============================================================================
 * PROOF, MEASURED ON THIS MACHINE
 *
 * Two loops, identical except for one keyword:
 *
 *     static int plain = 0;
 *     static volatile sig_atomic_t marked = 0;
 *     long loop_plain(void)    { long n=0; while (!plain)  n++; return n; }
 *     long loop_volatile(void) { long n=0; while (!marked) n++; return n; }
 *
 *     gcc -O2 -c vol.c && objdump -d --no-show-raw-insn vol.o
 *
 * loop_plain, compiled at -O2:
 *
 *     0000000000000000 <loop_plain>:
 *        0:   endbr64
 *        4:   nopl   0x0(%rax)
 *        8:   jmp    8 <loop_plain+0x8>
 *
 * Three instructions, and the last one jumps to itself. The compiler read
 * `plain` at compile time, proved nothing in the function changes it, and
 * emitted an unconditional infinite loop. It did not even bother to count:
 * `n` is unreachable, so it does not exist.
 *
 * loop_volatile, same flags:
 *
 *     0000000000000010 <loop_volatile>:
 *       10:   endbr64
 *       14:   mov    0x0(%rip),%eax        <- load, before the loop
 *       1a:   test   %eax,%eax
 *       21:   jne    40
 *       28:   mov    0x0(%rip),%edx        <- load, EVERY iteration
 *       2e:   add    $0x1,%rax
 *       32:   test   %edx,%edx
 *       34:   je     28
 *       36:   ret
 *
 * The load is inside the loop. That is the entire difference, and it is why
 * the flag can ever be seen to change.
 *
 * Reproduce it:
 *
 *     printf 'static int p;\\nlong f(void){long n=0;while(!p)n++;return n;}\\n' > /tmp/v.c
 *     gcc -O2 -c /tmp/v.c -o /tmp/v.o
 *     objdump -d --no-show-raw-insn /tmp/v.o
 *
 * WHERE volatile IS RIGHT
 *
 *   1. a flag shared with a signal handler   (this file)
 *   2. memory-mapped hardware registers, where reading has a side effect
 *   3. a variable modified by setjmp/longjmp
 *
 * That is close to the whole list.
 *
 * WHERE IT IS WRONG, AND THIS IS THE IMPORTANT PART
 *
 * volatile is not a threading tool. It is a promise about the COMPILER, and
 * it says nothing about the CPU or about other cores:
 *
 *   no atomicity   `volatile int x; x++;` is still load, add, store. Two
 *                  threads still lose updates, exactly as in pthreads
 *                  step 12. volatile changes nothing about that.
 *
 *   no ordering    the CPU may still reorder your stores relative to other
 *                  variables, and another core may observe them in a
 *                  different order than you wrote them.
 *
 *   no visibility  nothing forces the write out of this core's store buffer
 *                  at any particular moment.
 *
 * The correct tools, in order of what you will meet them:
 *
 *     pthread_mutex_t          next lecture
 *     _Atomic int / <stdatomic.h>    C11 atomics, lock free for small types
 *     pthread_barrier_t, condition variables
 *
 * A mutex gives you all three properties at once, which is why it is the
 * default answer. `_Atomic int counter; counter++;` is atomic AND ordered,
 * and needs no volatile: atomic implies everything volatile gives you.
 *
 * If you ever find yourself typing volatile in threaded code, it is almost
 * certainly the wrong keyword. The Linux kernel documentation has a file
 * called "volatile-considered-harmful.txt" making exactly this argument.
 *
 * WHY IT LOOKS LIKE IT WORKS
 *
 * At -O0 the compiler reloads everything from memory anyway, so a plain
 * `int` flag behaves like a volatile one and threaded code with no
 * synchronisation appears fine. Turn on -O2 for the release build and it
 * breaks. Same shape as step 23 and step 29: the bug was always there, and
 * the optimiser only revealed it.
 *
 * EXPERIMENTE:
 *
 *  1. Remove `volatile` from stop_requested, then build at -O2 in the
 *     terminal:
 *
 *         gcc -std=gnu17 -Wall -O2 passo-37-volatile.c -o /tmp/hang
 *         /tmp/hang
 *
 *     It prints the handler message and never stops. Ctrl+C to kill it. Now
 *     build the same source at -O0 and it works. One source file, two
 *     behaviours, decided by a flag.
 *
 *  2. Compare the assembly of the two builds:
 *
 *         gcc -O2 -S -masm=intel -o - passo-37-volatile.c | grep -A12 "main:"
 *
 *  3. Put a printf inside the handler instead of write. It will usually
 *     work, which is the problem: it is undefined behaviour that mostly
 *     behaves. Read `man 7 signal-safety` and look for printf in the list.
 *
 *  4. Change stop_requested to `volatile int` instead of
 *     `volatile sig_atomic_t`. It still works on x86-64, because int
 *     happens to be atomic here. sig_atomic_t is the type that PROMISES it,
 *     on every platform. The difference between "works here" and
 *     "guaranteed" is most of portable C.
 *
 *  5. Write the counter increment with C11 atomics and compare:
 *
 *         #include <stdatomic.h>
 *         static _Atomic long spins;
 *
 *     Then look at the generated instruction: `lock add` rather than a plain
 *     add. That `lock` prefix is the atomicity, and it is what volatile
 *     never gave you.
 *
 * -> back to "00 - COMECE AQUI.md" and [[tools]]
 * ========================================================================= */
