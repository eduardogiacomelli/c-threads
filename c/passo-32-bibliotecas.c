/* ============================================================================
 * STEP 32 - static and shared libraries, and what `-lm` has been doing.
 *
 * A library is just a bag of object files with an index. There are two kinds
 * and the difference is WHEN the code joins your program:
 *
 *     libfoo.a    static.  Copied into your binary at link time.
 *     libfoo.so   shared.  Loaded from disk at program START, every run.
 *
 * That single difference explains file sizes, deployment, LD_LIBRARY_PATH,
 * and why upgrading a system library can fix a bug in a program you did not
 * rebuild.
 *
 *     make 32
 *
 * The program is a two-line driver. The step is the footer: you build both
 * kinds from contas.c and watch them behave differently.
 * ========================================================================= */

#include <stdio.h>
#include "contas.h"

int main(void)
{
    printf("sum_to(10)      = %ld\n", sum_to(10));
    printf("is_even(7)         = %d\n", is_even(7));
    printf("call_count  = %d\n", call_count());

    /* Right now contas.c is compiled and linked in directly, the way the
     * Makefile has always done it. The footer turns it into a real library
     * twice over, and nothing in this file changes. */
    return 0;
}

/* ============================================================================
 * BUILD BOTH, IN A SCRATCH DIRECTORY
 *
 *     mkdir -p /tmp/lib32 && cd /tmp/lib32
 *     cp ~/c-playground/c-do-zero/contas.c ~/c-playground/c-do-zero/contas.h .
 *     cp ~/c-playground/c-do-zero/passo-32-bibliotecas.c main.c
 *
 * STATIC (.a). `ar` is a plain archiver, older than C itself. rcs means
 * replace, create, write an index.
 *
 *     gcc -c contas.c -o contas.o
 *     ar rcs libcontas.a contas.o
 *     ar t libcontas.a                 list what is inside
 *     gcc main.c -L. -lcontas -o app_static
 *     ./app_static
 *
 * Read the link line carefully, because the shape is always the same:
 *
 *     -L.          look for libraries in this directory
 *     -lcontas     link libcontas.a or libcontas.so  (lib + name + suffix)
 *
 * That is what -lm has always been: link libm.so, the maths library. And
 * "cannot find -lfoo" means the linker looked and there was no libfoo
 * anywhere on its search path.
 *
 * SHARED (.so). Needs -fPIC, position independent code, because the library
 * can be mapped at a different address in every process that uses it, so it
 * cannot contain hard-coded absolute addresses.
 *
 *     gcc -fPIC -c contas.c -o contas_pic.o
 *     gcc -shared -o libcontas.so contas_pic.o
 *     gcc main.c -L. -lcontas -o app_shared -Wl,-rpath,'$ORIGIN'
 *     ./app_shared
 *
 * NOW COMPARE THEM
 *
 *     ls -l app_static app_shared
 *     ldd app_static
 *     ldd app_shared
 *
 * Measured here: 16120 bytes static against 15992 shared, and `ldd` tells
 * the real story. The static one lists only libc and the loader. The shared
 * one also lists libcontas.so, with the path it will load at run time.
 *
 * The size gap is tiny because contas.c is tiny. With a real library it is
 * the difference between a 20 MB binary and a 200 KB one.
 *
 * THE ERROR EVERY BEGINNER HITS WITH .so
 *
 *     gcc main.c -L. -lcontas -o app_norpath
 *     ./app_norpath
 *
 *     ./app_norpath: error while loading shared libraries: libcontas.so:
 *     cannot open shared object file: No such file or directory
 *
 * It LINKED fine and it will not START. -L is a compile-time search path;
 * it is not remembered. At run time the dynamic loader searches its own
 * list, and the current directory is not on it. Three fixes:
 *
 *     LD_LIBRARY_PATH=. ./app_norpath          for one run
 *     gcc ... -Wl,-rpath,'$ORIGIN'             bake a path into the binary
 *     sudo cp libcontas.so /usr/local/lib && sudo ldconfig    install it
 *
 * $ORIGIN means "the directory the executable is in", and the single quotes
 * stop the shell expanding it. That is how a program ships with its own
 * libraries in the same folder.
 *
 * WHEN BOTH EXIST
 *
 * With libcontas.a and libcontas.so in the same directory, -lcontas picks
 * the SHARED one. Force the other with -static, and check with ldd.
 *
 * ORDER MATTERS FOR STATIC LIBRARIES
 *
 *     gcc -lcontas main.c        may fail
 *     gcc main.c -lcontas        works
 *
 * The linker processes its arguments left to right, and when it reaches a
 * static library it takes only the members that resolve symbols it already
 * knows are missing. Put the library first and there are no missing symbols
 * yet, so it takes nothing. Libraries go last. This is the single most
 * confusing linker behaviour there is, and it is why -lm goes at the end.
 *
 * WHAT THE SYSTEM ALREADY DOES THIS WAY
 *
 *     ldd /bin/ls
 *
 * Every program on the machine shares one copy of libc.so.6, mapped into
 * each process. That is why a libc security fix protects programs you never
 * recompiled, and why a static binary does not benefit.
 *
 * EXPERIMENTE:
 *
 *  1. Build both, run ldd on each, and note which lists libcontas.
 *
 *  2. With app_shared built and working, change contas.c (make sum_to
 *     return 0), rebuild ONLY the .so, and run app_shared again without
 *     relinking it. The behaviour changes. Do the same with the static one:
 *     nothing changes until you relink. That is the whole trade in one
 *     experiment.
 *
 *  3. `nm -D libcontas.so` lists the dynamic symbols the library exports.
 *     Compare with `nm libcontas.a`. Then confirm that `calls`, the
 *     static variable inside contas.c, is exported by neither.
 *
 *  4. Put -lcontas before main.c and read the failure. Then move it back.
 *
 *  5. `ldd /bin/ls` and `ldd $(which gcc)`. Everything on a Linux system is
 *     built this way.
 *
 * -> passo-33
 * ========================================================================= */
