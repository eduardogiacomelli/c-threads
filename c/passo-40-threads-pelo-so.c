/* ============================================================================
 * STEP 40 - a thread, from the operating system's side.
 *
 * Step 33 showed the memory map of a process. Step 34 showed the boundary
 * between your code and the kernel. This one puts them together: what does
 * the kernel actually DO when you call pthread_create?
 *
 * The short answer is that Linux has no threads. It has tasks, and a thread
 * is a task that shares its address space with another task. One flag in one
 * system call is the whole difference.
 *
 *     make 40
 *     strace -f -e trace=clone3,clone,mmap,futex ./passo-40-threads-pelo-so
 * ========================================================================= */

#define _GNU_SOURCE         /* for gettid() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>         /* opendir, readdir */
#include <sys/types.h>

#define THREADS 3

static void show_ids(const char *who)
{
    /* getpid() is the PROCESS id, shared by every thread.
     * gettid() is the THREAD id, unique per thread.
     * pthread_self() is a library handle, not a kernel id, and is only
     * meaningful inside the program (pthreads step 11). */
    printf("  %-10s pid=%d  tid=%d  pthread_self=%lu\n",
           who, (int) getpid(), (int) gettid(),
           (unsigned long) pthread_self());
}

static void count_tasks(const char *when)
{
    /* /proc/self/task/ has one directory per thread of THIS process, and
     * that listing is the kernel's own answer to "how many threads do I
     * have". It needs no library support at all.
     *
     * Read it IN PROCESS with opendir. The obvious shortcut,
     *
     *     popen("ls /proc/self/task | wc -l", "r")
     *
     * always answers 1, and the reason is the point of this whole file:
     * popen forks a shell, and inside that child `/proc/self` means the
     * CHILD. You cannot shell out to inspect yourself. (It is also a bad
     * idea in a threaded program for unrelated reasons: fork only carries
     * the calling thread across.) */
    DIR *d = opendir("/proc/self/task");
    if (d == NULL) {
        perror("/proc/self/task");
        return;
    }

    int tasks = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL)
        if (e->d_name[0] != '.')        /* skip . and .. */
            tasks++;
    closedir(d);

    printf("  tasks in /proc/self/task %-14s %d\n", when, tasks);
}

static void show_thread_stacks(void)
{
    /* Each thread's stack is an ordinary anonymous mapping. Only the main
     * thread's gets the [stack] label; the others are just rw-p regions the
     * pthread library mmap'd. Their size is the default stack limit. */
    printf("\n  large rw-p mappings (candidate thread stacks):\n");
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps == NULL)
        return;

    char line[512];
    while (fgets(line, sizeof line, maps) != NULL) {
        unsigned long start, end;
        char perms[8] = "";
        if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) != 3)
            continue;
        unsigned long size = end - start;
        int is_main = strstr(line, "[stack]") != NULL;

        /* The main thread's stack starts small and grows on demand, so it
         * would fail a size filter. Always include the labelled one. */
        if (strcmp(perms, "rw-p") == 0 && (is_main || size >= 512 * 1024))
            printf("    %lx-%lx  %6lu KB%s\n", start, end, size / 1024,
                   is_main ? "  <- main thread, grows on demand" : "");
    }
    fclose(maps);
}

static void *worker(void *arg)
{
    int id = *(int *) arg;
    char label[16];
    snprintf(label, sizeof label, "thread %d", id);
    show_ids(label);

    /* A local, so its address is on THIS thread's stack. Compare the three
     * and you are looking at three separate stack regions. */
    int local = id;
    printf("  %-10s &local = %p\n", label, (void *) &local);

    /* Stay alive long enough for main to look at us. Without this the
     * workers finish before main gets to count, and /proc/self/task reports
     * 1 the whole way through: the threads existed, just never at the
     * moment anybody checked. Timing you did not think about is the entire
     * subject of concurrency. */
    usleep(400000);
    return NULL;
}

int main(void)
{
    printf("one process, several tasks:\n");
    show_ids("main");
    printf("  main: pid == tid, because the first thread IS the process\n\n");

    count_tasks("at start");

    pthread_t t[THREADS];
    int ids[THREADS];               /* one slot per thread, pthreads step 04 */

    printf("\ncreating %d threads:\n", THREADS);
    for (int i = 0; i < THREADS; i++) {
        ids[i] = i;
        int rc = pthread_create(&t[i], NULL, worker, &ids[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }

    /* Sleep briefly so the workers are alive while we count. Never do this
     * for synchronisation in real code: it is a race, not a guarantee. Here
     * it is a deliberate way to observe a transient state. */
    usleep(200000);
    printf("\n");
    count_tasks("while running");
    show_thread_stacks();

    for (int i = 0; i < THREADS; i++)
        pthread_join(t[i], NULL);

    printf("\n");
    count_tasks("after join");

    return 0;
}

/* ============================================================================
 * WHAT strace SHOWS
 *
 *     strace -f -e trace=clone3,mmap,futex,write ./passo-40-threads-pelo-so
 *
 * -f is what makes the WORKERS visible. Be precise about what it does:
 * clone3 is called BY main, so you see the thread creations either way. What
 * you lose without -f is everything the new threads then do. Measured on
 * this program with -e trace=write:
 *
 *     without -f   2 write lines, one task
 *     with -f      5 write lines, four tasks (19419, 19420, 19421, 19422)
 *
 * So the rule is not "clone disappears", it is "the threads disappear". For
 * anything threaded, -f.
 *
 * The three things you will see, in order, for every pthread_create:
 *
 *   1. mmap(NULL, 8392704, PROT_NONE,
 *           MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0x72d8af9ff000
 *      mprotect(0x72d8afa00000, 8388608, PROT_READ|PROT_WRITE) = 0
 *
 *      Two calls, and the arithmetic between them is the guard page:
 *
 *          mapped      8392704 bytes   PROT_NONE, nothing may touch it
 *          unprotected 8388608 bytes   read/write, the usable stack
 *          difference     4096 bytes   exactly one page, left PROT_NONE
 *
 *      and the mprotect base is 0x1000 above the mmap base, so the
 *      inaccessible page is at the BOTTOM: the end the stack grows towards
 *      (step 27). Run off the end of your stack and you hit a page the
 *      kernel refuses, giving a clean fault instead of one thread quietly
 *      scribbling into another thread's locals.
 *
 *      Note the library asks for the stack BEFORE creating the thread. This
 *      is the 8 MB region step 33 predicted, now watched being made.
 *
 *   2. clone3({flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND
 *              |CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS
 *              |CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, ...})
 *
 *      There is no "create thread" system call. There is clone, and the
 *      flags decide how much the new task shares with the old one:
 *
 *        CLONE_VM       same address space. THIS is what makes it a thread.
 *        CLONE_FILES    same file descriptors
 *        CLONE_FS       same working directory
 *        CLONE_SIGHAND  same signal handlers
 *        CLONE_THREAD   same process id, appears in /proc/PID/task
 *
 *      fork() is the same system call with none of those flags: a new task
 *      with a COPY of the address space. Thread and process are the same
 *      kernel object with different sharing. That is the whole story, and it
 *      is why everything in memoria.md about "threads share the heap" is
 *      literally true rather than a metaphor: CLONE_VM means one page table.
 *
 *   3. futex(..., FUTEX_WAIT, ...)   when main calls pthread_join
 *
 *      futex is "fast userspace mutex". The uncontended case never enters
 *      the kernel at all; only when a thread actually has to WAIT does it
 *      make this call. That is why an uncontended pthread_mutex_lock costs
 *      about as much as an atomic (step 38) and a contended one costs
 *      thousands of times more: the second one is a system call and a
 *      context switch.
 *
 * USEFUL VARIATIONS
 *
 *     strace -f -c ./prog              summary: which syscalls, how many
 *     strace -f -e trace=%%memory ./prog   just mmap/munmap/brk
 *     strace -f -T ./prog              time spent inside each syscall
 *     strace -f -o /tmp/trace.txt ./prog   to a file, keeping stdout clean
 *
 * `-c` on this program shows futex and clone3 counts that track exactly with
 * the number of threads, which is a quick sanity check that your program
 * creates what you think it creates.
 *
 * THE PROCESS AND ITS TASKS
 *
 *     ls /proc/self/task            one directory per thread
 *     ps -L -p <pid>                the same thing, from ps
 *     top -H                        top, but per thread
 *     cat /proc/<pid>/status        Threads: N, plus memory totals
 *
 * The program counts /proc/self/task itself, before, during and after, and
 * the number goes 1, 4, 1. Note that it uses opendir rather than shelling
 * out: see the comment on count_tasks for why `popen("ls /proc/self/task")`
 * always answers 1.
 *
 * WHY pid AND tid MATTER
 *
 *     getpid()        the process. Every thread returns the same value.
 *     gettid()        the task. Unique per thread. This is what strace,
 *                     top -H and gdb's "info threads" show you.
 *     pthread_self()  a library handle. Opaque, not a kernel id, and only
 *                     comparable with pthread_equal.
 *
 * When a crash report or a log line has a number in it, knowing which of the
 * three it is saves you from chasing the wrong thread.
 *
 * EXPERIMENTE:
 *
 *  1. Run it under strace without -f and then with, tracing writes:
 *
 *         strace    -e trace=write ./passo-40-threads-pelo-so 2>&1 | wc -l
 *         strace -f -e trace=write ./passo-40-threads-pelo-so 2>&1 | wc -l
 *
 *     Then do the same for clone3 and notice it does NOT change. Being able
 *     to say exactly what -f adds is worth more than remembering to type it.
 *
 *  2. `strace -f -c ./passo-40-threads-pelo-so`. Find clone3, mmap, futex,
 *     and check the counts against THREADS.
 *
 *  3. Compare the &local addresses the three workers print with the mapping
 *     list above them. Each local should sit inside one of the 8 MB regions,
 *     and no two in the same one.
 *
 *  4. Ask for a smaller stack and watch the mmap size change:
 *
 *         pthread_attr_t a;
 *         pthread_attr_init(&a);
 *         pthread_attr_setstacksize(&a, 128 * 1024);
 *         pthread_create(&t[i], &a, worker, &ids[i]);
 *
 *     Then reread step 35, where a size below PTHREAD_STACK_MIN was the
 *     EINVAL example. 1000 threads at 8 MB each is 8 GB of reservations;
 *     this is the knob that makes that survivable.
 *
 *  5. Write the fork() version: call fork() instead, and have both sides
 *     modify the same global. The child's change does not reach the parent,
 *     because there was no CLONE_VM. Same syscall, one flag, and the entire
 *     difference between concurrency models.
 *
 * -> back to "00 - COMECE AQUI.md" and the pthreads tutorial
 * ========================================================================= */
