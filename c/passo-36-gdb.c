/* ============================================================================
 * STEP 36 - gdb: six commands that pay for themselves.
 *
 * This program crashes on purpose, three frames deep, the way real code
 * does: a list walk that assumes a terminator that is not there.
 *
 *     make 36           runs it and it dies
 *
 * Then work through the gdb session in the footer. Every command and every
 * line of output there was captured from this exact program.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>

struct Node {
    int          value;
    struct Node *next;
};

static struct Node *make_node(int value)
{
    struct Node *n = malloc(sizeof *n);
    if (n == NULL) {
        perror("malloc");
        exit(1);
    }
    n->value = value;
    n->next  = NULL;
    return n;
}

/* THE BUG IS HERE.
 *
 * The loop tests node->value, as if the list ended with a node holding 0.
 * It does not: it ends with next == NULL. So on the last real node the body
 * runs, node becomes NULL, and the next test dereferences it.
 *
 * Written this way on purpose. It is a plausible mistake, it compiles clean,
 * and it crashes in a function that is nowhere near where the list was
 * built. */
static long sum_list(struct Node *node)
{
    long total = 0;

    while (node->value != 0) {          /* <- crashes here on the last pass */
        total += node->value;
        node = node->next;
    }
    return total;
}

static void report(struct Node *head, const char *label)
{
    long total = sum_list(head);
    printf("%s: %ld\n", label, total);
}

int main(void)
{
    struct Node *head = make_node(10);
    head->next = make_node(20);
    head->next->next = make_node(30);

    printf("list built: %d -> %d -> %d -> NULL\n",
           head->value, head->next->value, head->next->next->value);
    printf("about to walk it\n");

    report(head, "sum");            /* never returns */

    free(head->next->next);
    free(head->next);
    free(head);
    return 0;
}

/* ============================================================================
 * THE SESSION, CAPTURED FROM THIS PROGRAM
 *
 * Build WITHOUT sanitizers for this. ASan is better at telling you what rule
 * you broke; gdb is better at letting you look around afterwards, and the
 * plain build is what you will debug in a real project.
 *
 *     gcc -std=gnu17 -Wall -Wextra -g passo-36-gdb.c -o /tmp/crash
 *
 * Every line of output below came from that binary. Your addresses will
 * differ (ASLR, step 33); the line numbers will not.
 *
 * THE ONE COMMAND. If you learn nothing else, learn this:
 *
 *     gdb -q --batch -ex run -ex bt /tmp/crash
 *
 *     Program received signal SIGSEGV, Segmentation fault.
 *     0x0000555555555273 in sum_list (node=0x0) at passo-36-gdb.c:46
 *     46          while (node->value != 0) {
 *     #0  0x0000555555555273 in sum_list (node=0x0) at passo-36-gdb.c:46
 *     #1  0x000055555555529f in report (head=0x5555555592a0,
 *                                       label=0x55555555604d "sum")
 *                                       at passo-36-gdb.c:55
 *     #2  0x0000555555555361 in main () at passo-36-gdb.c:69
 *
 * Read it top to bottom. Frame #0 is where it died, and gdb printed the
 * argument that killed it: node=0x0. Frames #1 and #2 are the callers, with
 * their arguments, including the string label spelled out. Three lines and
 * you know a null node reached sum_list.
 *
 * --batch runs the commands and exits, which makes it scriptable. Drop it to
 * stay in the session.
 *
 * (gdb may ask about debuginfod on first use. Answer n, or silence it with
 *  -ex 'set debuginfod enabled off'.)
 *
 * LOOKING AROUND, REAL OUTPUT
 *
 *     (gdb) frame 1
 *     #1  0x0000...29f in report (head=0x5555555592a0, ...) at ...:55
 *     55          long total = sum_list(head);
 *
 *     (gdb) print head
 *     $1 = (struct Node *) 0x5555555592a0
 *
 *     (gdb) print *head
 *     $2 = {value = 10, next = 0x5555555592c0}
 *
 *     (gdb) print head->next->value
 *     $3 = 20
 *
 *     (gdb) print *head->next->next->next
 *     Cannot access memory at address 0x0
 *
 * That last line is the bug, stated by gdb: the fourth node does not exist.
 *
 * gdb KNOWS YOUR TYPES
 *
 *     (gdb) ptype struct Node
 *     type = struct Node {
 *         int value;
 *         struct Node *next;
 *     }
 *
 *     (gdb) print sizeof(struct Node)
 *     $4 = 16
 *
 * Sixteen, not twelve. There it is again: 4 bytes of int, 4 bytes of
 * alignment padding, 8 bytes of pointer (step 31, and the byte view in the
 * app). And you can see the padding directly:
 *
 *     (gdb) x/16xb head
 *     0x5555555592a0: 0x0a 0x00 0x00 0x00  0x00 0x00 0x00 0x00
 *     0x5555555592a8: 0xc0 0x92 0x55 0x55  0x55 0x55 0x00 0x00
 *                     \_______________/    \____________________/
 *                     value = 10           padding, then next
 *
 * WATCHPOINTS: WHO IS CHANGING THIS?
 *
 *     (gdb) break sum_list
 *     (gdb) run
 *     (gdb) watch node
 *     Hardware watchpoint 2: node
 *     (gdb) continue
 *
 *     Hardware watchpoint 2: node
 *     Old value = (struct Node *) 0x5555555592a0
 *     New value = (struct Node *) 0x5555555592c0
 *     sum_list (node=0x5555555592c0) at passo-36-gdb.c:46
 *
 * Continue again and it steps to 0x...92e0, then to 0x0. It is hardware
 * assisted, so it costs nothing, and it is the fastest way to answer "who
 * wrote to this variable".
 *
 * A CONDITIONAL BREAKPOINT, AND WHY IT MISSED
 *
 * The obvious move is:
 *
 *     (gdb) break sum_list if node == 0
 *
 * It never fires: the condition is checked on ENTRY to the function, and
 * sum_list is only entered once, with a valid head. Fair enough. So try the
 * line the crash reported:
 *
 *     (gdb) break passo-36-gdb.c:46 if node == 0
 *
 * That misses too, and the reason is worth the whole step:
 *
 *     (gdb) info line passo-36-gdb.c:46
 *     Line 46 starts at address 0x1255 <sum_list+20>
 *              and ends at 0x1257 <sum_list+22>.
 *
 * One source line, several machine addresses. Here is the loop:
 *
 *     1255:  jmp  126f          <- gdb put the breakpoint HERE. Runs once.
 *     1257:  mov  -0x18(%rbp),%rax     loop body
 *     ...
 *     126f:  mov  -0x18(%rbp),%rax     the test
 *     1273:  mov  (%rax),%eax          <- the crash: reading node->value
 *     1277:  jne  1257
 *
 * `while` compiles to a jump forward to the test plus a jump back. Line 46
 * owns both the entry jump at 0x1255 and the test at 0x126f, and
 * `break file:46` picked the first. The condition was false there, once, and
 * never evaluated again.
 *
 * Lesson: a breakpoint is on an ADDRESS, not on a line, and `info line`
 * tells you which one you got. When a line breakpoint behaves oddly in a
 * loop, that is why. `watch` sidesteps the problem entirely, which is what
 * makes it the right tool here.
 *
 * THE COMMANDS, RANKED BY HOW OFTEN YOU WILL USE THEM
 *
 *     bt                  where did it die, and who called it
 *     print x / p *p      look at anything, including expressions
 *     break f / break f:n stop somewhere
 *     next / step         one line, over calls / into calls
 *     info locals         everything in this frame
 *     x/16xb p            the actual bytes
 *     watch v             stop when v changes
 *     finish              run to the end of this frame
 *
 * For threads, three more that matter for PPD:
 *
 *     info threads              list them, current one marked with *
 *     thread 3                  switch
 *     thread apply all bt       a backtrace of EVERY thread at once
 *
 * That last one diagnoses a deadlock: run it, let it hang, Ctrl+C in gdb,
 * and ask every thread what it is waiting for.
 *
 * SANITIZER OR gdb?
 *
 *     sanitizer   what rule you broke, file and line, automatically, and it
 *                 catches bugs that have not crashed yet
 *     gdb         everything around the crash: values, callers, memory,
 *                 and the ability to ask another question
 *
 * Complements, not alternatives. Reach for the sanitizer first because it is
 * automatic. See [[tools]].
 *
 * EXPERIMENTE:
 *
 *  1. Run the one-liner. Read all three frames and name the wrong line out
 *     loud before reading further.
 *
 *  2. Fix it: the condition should be `while (node != NULL)`. Rebuild, run,
 *     confirm it prints 60.
 *
 *  3. Put the bug back and reproduce the breakpoint miss yourself. Run
 *     `info line passo-36-gdb.c:46` and compare the address it reports with
 *     the crash address in the backtrace. They are different, and now you
 *     know why.
 *
 *  4. Use `watch node` and continue until node becomes 0x0. That is the last
 *     safe moment before the crash; `bt` there and look at the frame.
 *
 *  5. Run the SANITIZER build under gdb too:
 *
 *         make 36
 *         gdb -q --batch -ex run -ex bt ./passo-36-gdb
 *
 *     ASan prints its report, then gdb shows the stack. You can have both.
 *
 * -> passo-37
 * ========================================================================= */
