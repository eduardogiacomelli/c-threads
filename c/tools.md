# Tools: what to run, and what each one can actually tell you

Checked on this machine (Ubuntu 24.04, gcc 13.3.0, binutils 2.42, gdb 15.1).
Everything listed as installed was run to produce the output shown.

## The one thing to do first

Turn warnings on and read them. In this playground they already are:

```
-std=gnu17 -Wall -Wextra -g -fsanitize=address,undefined
```

Across steps 01 to 34 the compiler found, on its own, the bugs in steps 03,
10, 12, 13, 14, 23 and 29. That is a better hit rate than any other tool
here, and it costs nothing.

Worth adding when you want more pressure:

| flag | catches |
|---|---|
| `-Wshadow` | a local hiding an outer variable of the same name |
| `-Wconversion` | silent narrowing, `long` into `int`, `double` into `float` |
| `-Wpedantic` | anything outside the standard you asked for |
| `-Wwrite-strings` | assigning a literal to a non-const `char *` (step 11) |
| `-Wvla` | variable length arrays, which live on the stack and can blow it |
| `-Werror` | stop treating warnings as suggestions |

Try them one at a time on code you already believe is clean. `-Wconversion`
in particular is loud the first time.

## Sanitizers: they run your program and watch it

Compiled in, so they only see code paths you actually execute. That is the
limitation: a sanitizer cannot find a bug in a branch you never took.

| build with | finds | steps |
|---|---|---|
| `-fsanitize=address` | out of bounds, use after free, use after return, leaks | 09, 12, 14, 15 |
| `-fsanitize=undefined` | signed overflow, bad shifts, null deref, misaligned access | 02, 04 |
| `-fsanitize=thread` | data races | pthreads 12 |
| `-fsanitize=leak` | leaks only, much cheaper than full ASan | 15 |

ASan and TSan cannot be combined; they need separate builds. The playground
has a task for each. TSan on Ubuntu 24.04 needs `setarch -R` to disable ASLR,
which the pthreads Makefile already does.

Two things worth knowing about ASan, both learned the hard way in this
tutorial:

- it relocates locals onto a fake stack, so frame addresses grow **up**
  instead of down (step 27);
- it replaces malloc, so `/proc/self/maps` shows `(anonymous)` where you
  would normally see `[heap]` and `[stack]` (step 33).

An instrumented build is not the program you ship. For anything about memory
layout, check both.

```bash
ASAN_OPTIONS=detect_stack_use_after_return=1 ./prog
ASAN_OPTIONS=detect_leaks=1 ./prog
UBSAN_OPTIONS=print_stacktrace=1 ./prog
```

## Reading a binary

All installed, all from binutils.

```bash
file prog                 # what kind of file, and dynamic or static
size prog                 # bytes of text, data and bss
nm prog                   # symbol table: T defined, U undefined, t static
nm -D lib.so              # dynamic symbols a shared library exports
objdump -d prog           # disassembly, with your symbol names
objdump -d -M intel prog  # intel syntax, easier if you are used to it
readelf -h prog           # ELF header, entry point, type
readelf -S prog           # every section: .text .rodata .data .bss
strings prog              # every string literal in the file
ldd prog                  # which shared libraries it needs at startup
addr2line -e prog 0x1234  # turn an address from a crash into file:line
c++filt _Z3fooi           # demangle a C++ symbol, useful when reading libs
```

`strings` on your own binary is worth doing once. Every message you print is
sitting there in plain text.

Steps 31 and 32 are built around `nm`, `ldd` and `ar`.

## Watching a running program

```bash
strace ./prog                      # every system call
strace -e trace=write ./prog       # just the ones you care about
strace -c ./prog                   # summary table, counts and time
strace -f ./prog                   # follow threads. Needed for pthreads.
```

`strace -f` is the one for PPD: without `-f` you only see the main thread.

`ltrace` (library calls rather than system calls) is **not installed**:
`sudo apt install ltrace`.

Step 34 is the strace step, and it explains why buffered `printf` output
disappears when a program crashes.

## gdb, the minimum that pays for itself

Installed. Compile with `-g`, which the playground already does.

```bash
gdb ./prog
```

| command | does |
|---|---|
| `run` / `r` | start it |
| `bt` | backtrace after a crash. **This is the one.** |
| `break file.c:42` / `b main` | breakpoint |
| `next` / `n`, `step` / `s` | over, into |
| `print x` / `p *p` / `p arr[3]` | inspect anything |
| `p/x x` | print in hex |
| `x/8xb &v` | dump 8 bytes of memory, hex. The byte view of step 25. |
| `info locals` | every local in this frame |
| `info threads` / `thread 2` | for pthreads |
| `finish` | run to the end of this function |

If you learn only one thing: run the program under gdb, let it crash, type
`bt`. That is a stack trace with line numbers, for free.

```bash
gdb -q --batch -ex run -ex bt ./prog    # crash, print backtrace, exit
```

Core dumps, if enabled with `ulimit -c unlimited`:

```bash
gdb ./prog core
```

## Measuring

```bash
time ./prog                        # wall, user, sys. Start here.
perf stat ./prog                   # instructions, cycles, cache misses
perf record ./prog && perf report  # where the time actually went
```

`perf` is installed. For the pthreads exercises, `perf stat` is the honest
way to compare sequential against threaded, because it shows you whether you
gained anything real or just moved work around.

Inside a program, use `clock_gettime(CLOCK_MONOTONIC, ...)`, never `time()`.

## Not installed, worth having

```bash
sudo apt install valgrind cppcheck clang clang-tidy clang-format ltrace
```

| tool | why |
|---|---|
| `valgrind` | finds memory errors with **no recompile**, catches things ASan misses (uninitialised reads, via memcheck) |
| `cppcheck` | static analysis, finds bugs without running the program at all |
| `clang` | a second compiler is a second opinion; its error messages are often clearer |
| `clang-tidy` | lint with fixes |
| `clang-format` | formatting, so you stop thinking about it |

Static analysis and sanitizers are complements, not alternatives: one reads
every path and never runs, the other runs one path and sees everything about
it.

```bash
valgrind --leak-check=full ./prog
cppcheck --enable=all --std=c17 .
```

## A build that checks itself

What a serious Makefile target looks like, using only what is here:

```make
CFLAGS := -std=gnu17 -Wall -Wextra -Wshadow -Wconversion -g

check:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o /tmp/t_asan $(SRC)
	/tmp/t_asan
	$(CC) $(CFLAGS) -fsanitize=thread -o /tmp/t_tsan $(SRC)
	setarch -R /tmp/t_tsan
	$(CC) $(CFLAGS) -Werror -c $(SRC) -o /dev/null
```

Three builds, three different questions. Run it before you hand anything in.

## The order to reach for them

1. **Compiler warnings.** Free, instant, highest hit rate.
2. **ASan and UBSan.** Cheap, and they turn "segfault" into a file and a line.
3. **`gdb` + `bt`.** When you have a crash and no idea where.
4. **`strace`.** When the program is not doing what you think it is asking
   the OS to do.
5. **TSan.** The moment there is more than one thread.
6. **`perf`.** Only after it is correct. A fast wrong answer is worthless.

Reading a warning carefully beats every tool below it on this list.

Back to [[00 - COMECE AQUI]] · [[memoria]]
