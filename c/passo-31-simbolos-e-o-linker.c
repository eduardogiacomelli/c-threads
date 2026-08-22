/* ============================================================================
 * STEP 31 - symbols: what the linker is actually matching up.
 *
 * "undefined reference" and "multiple definition" are the two errors that
 * stop being mysterious the moment you can list the symbol table yourself.
 * That is one command, and this step is about running it.
 *
 *     make 31           (links contas.c in, as always)
 *
 * Then work through the footer with nm. That is where the step lives.
 * ========================================================================= */

#include <stdio.h>
#include "contas.h"

/* EXTERNAL linkage: visible to every other object file in the program.
 * This is the default for functions and for globals declared outside any
 * function, and it is what puts a capital T in the nm output. */
int program_counter = 0;

void announce(const char *what)
{
    program_counter++;
    printf("  [%d] %s\n", program_counter, what);
}

/* INTERNAL linkage: `static` at file scope means "this name does not leave
 * this object file". The linker never sees it, so no other file can call it
 * and no other file can collide with it. nm shows it in lowercase.
 *
 * This is the closest thing C has to private, and it should be your default
 * for any helper that is not part of the file's interface. */
static int secret_helper(int x)
{
    return x * 3;
}

/* A static global: same rule, same reason. */
static int call_count = 0;

int main(void)
{
    announce("main starting");

    call_count++;
    printf("  secret_helper(4) = %d, called %d time(s)\n",
           secret_helper(4), call_count);

    /* soma_ate and quantas_chamadas come from contas.c, a different object
     * file. This file only ever saw their declarations in contas.h; the
     * linker is what connects the call to the code. */
    printf("  soma_ate(10) from contas.o = %ld\n", soma_ate(10));
    printf("  quantas_chamadas() = %d\n", quantas_chamadas());

    /* `chamadas` is static inside contas.c, so this file cannot name it.
     * Uncomment to see the compiler refuse (experiment 4). */
    /* printf("%d\n", chamadas); */

    announce("done");
    return 0;
}

/* ============================================================================
 * READ THE SYMBOL TABLE
 *
 * Build the objects without linking, then look:
 *
 *     gcc -c passo-31-simbolos-e-o-linker.c -o /tmp/s31.o
 *     gcc -c contas.c -o /tmp/contas.o
 *     nm /tmp/s31.o
 *
 * The letter in the middle column is the whole message:
 *
 *     T   defined here, in the Text (code) section, global
 *     t   defined here, in Text, but LOCAL (static)
 *     D   defined, initialised data, global      (program_counter = 0 may
 *         land in B instead, see below)
 *     d   defined, initialised data, local
 *     B   defined, uninitialised (bss), global
 *     b   defined, uninitialised (bss), local    (call_count)
 *     R   read-only data, global                 (string literals live near here)
 *     U   UNDEFINED: needed, not provided by this file
 *
 * Lowercase always means static. So:
 *
 *     nm /tmp/s31.o | grep -i helper     ->  t secret_helper
 *     nm /tmp/s31.o | grep -i announce   ->  T announce
 *     nm /tmp/s31.o | grep soma_ate      ->  U soma_ate
 *
 * That last line IS the error message you get when you forget contas.c. The
 * linker walks every U and looks for a matching T or D somewhere else. If it
 * finds none:
 *
 *     undefined reference to `soma_ate'
 *
 * And if it finds two, from two different files:
 *
 *     multiple definition of `soma_ate'
 *
 * Two errors, one table, no mystery.
 *
 * SEE IT FAIL, AND SEE IT WORK
 *
 *     gcc /tmp/s31.o -o /tmp/broken            <- undefined reference
 *     gcc /tmp/s31.o /tmp/contas.o -o /tmp/ok  <- fine
 *     nm /tmp/ok | grep -E "soma_ate|printf"
 *
 * In the linked executable soma_ate is now T. printf stays U, because it
 * lives in the shared C library and is resolved when the program starts, not
 * when it is linked. passo-32 is about that.
 *
 * OTHER THINGS WORTH RUNNING ON AN OBJECT FILE
 *
 *     file /tmp/s31.o          "ELF 64-bit LSB relocatable"
 *     size /tmp/s31.o          how many bytes of text, data and bss
 *     readelf -h /tmp/ok       the ELF header, including the entry point
 *     readelf -S /tmp/ok       every section: .text, .rodata, .data, .bss
 *     objdump -d /tmp/s31.o    disassembly, with your symbol names
 *     strings /tmp/ok | head   every string literal in the binary
 *
 * `strings` on your own binary is worth doing once. Every message you print
 * is sitting there in plain text, which is a useful thing to know before you
 * ever put something sensitive in a string literal.
 *
 * WHY static IS NOT OPTIONAL HYGIENE
 *
 * Every non-static name goes into a single flat namespace shared by the
 * whole program, including every library you link. Two files that both
 * define `helper` will not link. Mark everything static unless the header
 * declares it, and the problem cannot occur.
 *
 * EXPERIMENTE:
 *
 *  1. `nm /tmp/s31.o | sort -k2` and read every line. Match each letter to
 *     the declaration that produced it. Note where program_counter and
 *     call_count ended up, and why an initialiser of 0 does not put a
 *     variable in .data: zeroed data goes in .bss, which costs no space in
 *     the file because the kernel zeroes the page for you.
 *
 *  2. Delete `static` from secret_helper and rerun nm. Lowercase t becomes
 *     uppercase T: the function just became visible to the entire program.
 *
 *  3. Cause "multiple definition" on purpose: add `int soma_ate(int n)
 *     { return 0; }` to this file and run `make 31`. Read the error, then
 *     note that it names both files.
 *
 *  4. Uncomment the `chamadas` line in main. The COMPILER stops you, not the
 *     linker, because static also means the name is not declared here. Two
 *     different defences from one keyword.
 *
 *  5. `objdump -d /tmp/s31.o | grep -A6 "<announce>:"`. That is your
 *     function as instructions, with the call to printf left as a
 *     placeholder for the linker to fill in.
 *
 * -> passo-32
 * ========================================================================= */
