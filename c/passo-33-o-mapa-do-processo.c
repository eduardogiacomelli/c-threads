/* ============================================================================
 * STEP 33 - the diagram in memoria.md, verified against the kernel.
 *
 * Everything you have been told about text, rodata, data, bss, heap and
 * stack is checkable. Linux publishes the real memory map of every running
 * process at /proc/<pid>/maps, and a process can read its own through the
 * alias /proc/self/maps.
 *
 * This program takes one address from each region, looks it up in that
 * table, and prints which mapping it landed in along with the permissions
 * the kernel enforces on it.
 *
 *     Ctrl+Shift+B      (or: make 33)
 *
 * Nothing here is a claim. It is a measurement.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One variable per region, so we have an address to look up. */
int    global_initialised = 42;     /* .data : has a value, so it is stored */
int    global_zero;                 /* .bss  : zero, so only its size is stored */
static const char *literal = "a string literal";   /* .rodata */

/* Find the line of /proc/self/maps whose range contains `addr`, and print
 * it. The format of each line is:
 *
 *     start-end perms offset dev inode   pathname
 *     55a1c2f0-55a1c310 r-xp 00001000 103:02 58720299 /tmp/prog
 *
 * perms is read, write, execute, and p for private (copy on write). */
static void where(const char *label, const void *addr)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps == NULL) {
        perror("/proc/self/maps");
        return;
    }

    unsigned long target = (unsigned long) addr;
    char line[512];
    int found = 0;

    while (fgets(line, sizeof(line), maps) != NULL) {
        unsigned long start, end;
        char perms[8] = "";
        char path[256] = "";

        /* %255[^\n] soaks up the rest of the line, path included, and the
         * width limit is what stops it overflowing (passo-13). */
        int n = sscanf(line, "%lx-%lx %7s %*s %*s %*s %255[^\n]",
                       &start, &end, perms, path);
        if (n < 3)
            continue;

        if (target >= start && target < end) {
            /* trim leading spaces the %[^\n] picked up */
            char *p = path;
            while (*p == ' ') p++;
            printf("  %-18s %14p  %s  %s\n", label, addr, perms,
                   *p ? p : "(anonymous)");
            found = 1;
            break;
        }
    }

    if (!found)
        printf("  %-18s %14p  not in any mapping?\n", label, addr);

    fclose(maps);
}

int main(void)
{
    int local = 1;
    int *heap = malloc(64);
    if (heap == NULL)
        return 1;

    printf("where each kind of thing actually lives:\n\n");
    printf("  %-18s %14s  %s  %s\n", "what", "address", "perm", "mapping");
    printf("  %-18s %14s  %s  %s\n", "----", "-------", "----", "-------");

    where("main (code)",        (void *) main);
    where("string literal",     (void *) literal);
    where("global = 42",        (void *) &global_initialised);
    where("global (zero)",      (void *) &global_zero);
    where("malloc(64)",         (void *) heap);
    where("local variable",     (void *) &local);

    printf("\nread the permission column:\n");
    printf("  r-xp  readable, EXECUTABLE, not writable  -> your code\n");
    printf("  r--p  read only                           -> string literals\n");
    printf("  rw-p  readable and writable, NOT executable\n");
    printf("        -> globals, heap and stack\n");

    printf("\nThat r-- on the literal is why `char *s = \"hi\"; s[0] = 'H';`\n");
    printf("dies (passo-11). It is not a C rule being enforced by the\n");
    printf("compiler, it is the MMU refusing the write.\n");

    printf("\nAnd rw- without x on the stack is why you cannot execute data\n");
    printf("there. That protection has a name, NX, and it exists because of\n");
    printf("exactly the overflow in passo-12.\n");

    free(heap);

    printf("\nfull map (%s):\n", "cat /proc/self/maps");
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps != NULL) {
        char line[512];
        int shown = 0;
        while (fgets(line, sizeof(line), maps) && shown < 12) {
            /* skip the sanitizer's enormous reservations, if present */
            if (strstr(line, "---p") == NULL) {
                fputs("  ", stdout);
                fputs(line, stdout);
                shown++;
            }
        }
        fclose(maps);
    }

    return 0;
}

/* ============================================================================
 * WHAT YOU JUST MEASURED
 *
 * Compiled plainly, the six addresses land like this:
 *
 *   main               0x5..bd249   r-xp   the program file
 *   string literal     0x5..be004   r--p   the program file
 *   global = 42        0x5..c0010   rw-p   the program file
 *   global (zero)      0x5..c002c   rw-p   the program file
 *   malloc(64)         0x5..fd2a0   rw-p   [heap]
 *   local variable     0x7ffe..f9c  rw-p   [stack]
 *
 * Four different mappings, all from one executable, with different
 * permissions. The kernel loaded your ELF file section by section and gave
 * each the rights it declared. That is what `readelf -S` was listing in
 * passo-31, and this is the same information seen from the other side.
 *
 * Note how far the stack is from everything else, and that its address
 * changes on every run. That is ASLR, address space layout randomisation,
 * and it is why passo-05 told you never to memorise an address.
 *
 * data VERSUS bss, AND WHY THE DISTINCTION EXISTS
 *
 *   global_initialised = 42   the 42 must be stored in the file
 *   global_zero               only its SIZE is stored; the kernel hands you
 *                             a page that is already zero
 *
 * Check it:  size /tmp/prog   and add a big zeroed array, then look again.
 * A 10 MB zeroed global costs 10 MB of bss and almost nothing on disk. That
 * is also why an uninitialised GLOBAL is zero while an uninitialised LOCAL
 * is garbage (passo-08): the global gets a fresh zeroed page, the local gets
 * whatever the last function left on the stack.
 *
 * UNDER THE SANITIZER IT LOOKS DIFFERENT
 *
 * Ctrl+Shift+B builds with ASan, and the table changes in a specific way:
 * the three program regions still say the executable's name, but the last
 * two now read (anonymous) instead of [heap] and [stack]. ASan replaces
 * malloc with its own allocator and relocates locals onto a fake stack
 * (passo-27), and both of those are plain anonymous mmap regions, so the
 * kernel has no [heap] or [stack] label to give them.
 *
 * The permissions are unchanged, so the lesson holds either way. Same
 * situation as passo-27: run both builds and compare.
 *
 *     gcc -std=gnu17 -Wall -g passo-33-o-mapa-do-processo.c -o /tmp/map
 *     /tmp/map
 *
 * WHY THIS MATTERS FOR THREADS
 *
 * Run `cat /proc/self/maps | grep stack` in a threaded program and you will
 * find several stack mappings, one per thread, each a few megabytes,
 * separated by unmapped guard pages. The guard page is what turns a stack
 * overflow into a clean crash instead of one thread quietly writing into
 * another thread's locals. memoria.md draws this; now you can see it.
 *
 * EXPERIMENTE:
 *
 *  1. Run twice and compare the stack addresses. Then turn ASLR off for one
 *     command and compare again:
 *
 *         setarch -R ./passo-33-o-mapa-do-processo | head -12
 *
 *     Identical every time. That is the flag the playground's ThreadSanitizer
 *     task already uses, and now you know why it needs it.
 *
 *  2. Add `static int big[1024 * 1024];` and compare `size` output before
 *     and after, then `ls -l` the binary. 4 MB of bss, and the file barely
 *     grows.
 *
 *  3. Add `char *s = "hi"; s[0] = 'H';` and run. Segmentation fault, and the
 *     r-- in the table above is the reason. Change it to `char s[] = "hi";`
 *     and it works, because now it is a copy on the stack, which is rw-.
 *
 *  4. `cat /proc/self/maps` in your shell shows bash's own map. Then
 *     `pmap $$` for a friendlier layout, and `cat /proc/$$/status` for the
 *     memory totals.
 *
 * -> passo-34
 * ========================================================================= */
