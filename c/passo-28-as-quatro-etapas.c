/* ============================================================================
 * STEP 28 - what `gcc file.c -o prog` actually runs.
 *
 * Four separate programs, in order. Every error message you have ever seen
 * from a C build comes from exactly one of them, and knowing which one cuts
 * the search in half before you start reading.
 *
 *     preprocessor  cpp    text in, text out. No C is understood yet.
 *     compiler      cc1    C in, assembly out. This is where warnings live.
 *     assembler     as     assembly in, object file out.
 *     linker        ld     object files in, executable out.
 *
 *     Ctrl+Shift+B      (or: make 28)
 *
 * Run it, then work through the commands in the footer. That is the step.
 * ========================================================================= */

#include <stdio.h>

/* The preprocessor defines these for you. They are substituted as text
 * before the compiler sees the line, which is why __LINE__ is correct on
 * every line it appears on. */
int main(void)
{
    printf("what the preprocessor knew about this file:\n");
    printf("  __FILE__          %s\n", __FILE__);
    printf("  __LINE__          %d\n", __LINE__);
    printf("  __func__          %s\n", __func__);
    printf("  __DATE__ __TIME__ %s %s\n", __DATE__, __TIME__);

    /* __STDC_VERSION__ tells you which standard you are compiling as.
     * 201710L is C17, which is what -std=gnu17 selects. */
    printf("  __STDC_VERSION__  %ldL\n", __STDC_VERSION__);

    /* Compiler and platform. These are not in the C standard, they come
     * from gcc, and they are how portable code adapts. */
#ifdef __GNUC__
    printf("  __GNUC__          %d.%d\n", __GNUC__, __GNUC_MINOR__);
#endif
#ifdef __linux__
    printf("  __linux__         defined\n");
#endif
#ifdef __x86_64__
    printf("  __x86_64__        defined\n");
#endif

    /* Sanitizers announce themselves too, which is how a program can behave
     * differently when instrumented (passo-27 measured exactly that). */
#ifdef __SANITIZE_ADDRESS__
    printf("  __SANITIZE_ADDRESS__ defined  <- built with -fsanitize=address\n");
#else
    printf("  __SANITIZE_ADDRESS__ not defined\n");
#endif

    printf("\n__LINE__ again, further down: %d\n", __LINE__);
    return 0;
}

/* ============================================================================
 * RUN THE FOUR STAGES BY HAND
 *
 * Do this once. It takes two minutes and it makes the rest of your C life
 * easier.
 *
 * 1. PREPROCESSOR. Text substitution only: #include is replaced by the whole
 *    contents of the file, macros are expanded, #ifdef branches are deleted.
 *    Nothing here understands C.
 *
 *        cpp passo-28-as-quatro-etapas.c > /tmp/stage1.i
 *        wc -l passo-28-as-quatro-etapas.c /tmp/stage1.i
 *
 *    About 60 lines become about 800. All of that came from stdio.h. Open
 *    /tmp/stage1.i and scroll to the bottom: your code is at the end, with
 *    every macro already replaced.
 *
 *        grep -n "__FILE__\|__LINE__" /tmp/stage1.i
 *
 *    They are gone. The strings are already there instead.
 *
 * 2. COMPILER. C in, assembly out. Every warning you get comes from here.
 *
 *        gcc -S -masm=intel -o /tmp/stage2.s /tmp/stage1.i
 *        grep -v "^\s*\." /tmp/stage2.s | head -30
 *
 *    That is your main(), as instructions. The playground has a task for
 *    this: "C: ver o assembly gerado".
 *
 * 3. ASSEMBLER. Assembly text in, machine code out, wrapped in an ELF object
 *    file. Not runnable yet: calls to printf are still blank.
 *
 *        as -o /tmp/stage3.o /tmp/stage2.s
 *        file /tmp/stage3.o
 *        nm /tmp/stage3.o
 *
 *    In the nm output, `T main` means main is defined here (Text section).
 *    `U printf` means Undefined: this object needs it and does not have it.
 *
 * 4. LINKER. Resolves every U against the other objects and the libraries,
 *    then writes an executable. Easiest to let gcc drive it, because it knows
 *    the startup files and default libraries:
 *
 *        gcc -o /tmp/stage4 /tmp/stage3.o
 *        /tmp/stage4
 *        file /tmp/stage4
 *
 * WHICH STAGE FAILED?
 *
 *     "No such file or directory: foo.h"        preprocessor
 *     "expected ';' before ..."                 compiler
 *     "implicit declaration of function 'f'"    compiler (missing #include)
 *     "warning: unused variable"                compiler
 *     "undefined reference to 'f'"              LINKER (missing .c or -l)
 *     "multiple definition of 'f'"              LINKER
 *     "cannot find -lm"                         linker (library not found)
 *
 * The two that trip people up are the last three, because they mention a
 * function name and look like compiler errors. They are not. No #include
 * will ever fix "undefined reference", because the declaration was found;
 * the code was not. passo-19 has the full story.
 *
 * WHY -c AND -o EXIST
 *
 *     gcc -c a.c        compile only, produce a.o, do not link
 *     gcc a.o b.o -o p  link only
 *
 * That split is the entire reason `make` is worth using: change one .c file
 * and only that one is recompiled, then everything is relinked. On a large
 * project the difference is minutes.
 *
 * EXPERIMENTE:
 *
 *  1. Count the lines: `cpp file.c | wc -l`. Then add #include <string.h>
 *     and count again. Every include is a paste, and it costs build time,
 *     which is why headers should be as small as they can be.
 *
 *  2. Define a macro from the command line, without touching the file:
 *
 *         gcc -DGREETING='"hi"' ...
 *
 *     and add `#ifdef GREETING` to the program. This is how build systems
 *     configure code, and how -DNDEBUG turns off assert (passo-30).
 *
 *  3. `gcc -E -dM - < /dev/null | sort | head -40` lists every macro gcc
 *     defines before it has read a single line of your program. There are
 *     around 400.
 *
 *  4. Break it on purpose at each stage: misspell an #include, drop a
 *     semicolon, call an undeclared function, call a declared but undefined
 *     function. Four errors, four different stages, four different shapes of
 *     message. Learn the shapes.
 *
 * -> passo-29
 * ========================================================================= */
