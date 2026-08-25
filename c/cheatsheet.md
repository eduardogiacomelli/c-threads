# C cheat sheet

Shapes you write, and the trap inside each one. Not a parameter reference:
that is what `man` is for, and the last section says how to read it.

Step numbers point at the runnable file that explains the thing.

## Build and run

```bash
# in VS Code: open any .c file and press Ctrl+Shift+B
# in the terminal, from ~/c-playground/c-do-zero:
make 05                 # build and run one tutorial step
make 17 ARGS="1e9 4"    # with command line arguments
make limpar             # remove the binaries

# anything of your own:
gcc -std=gnu17 -Wall -Wextra -g -fsanitize=address,undefined -pthread f.c -o /tmp/f && /tmp/f
```

Steps 09, 12, 14 and 36 are supposed to abort. That is the lesson, not a
broken build.

## printf and scanf

The one mapping worth memorising. Everything else is `man 3 printf`.

| you have | printf | scanf |
|---|---|---|
| `int` | `%d` | `%d` |
| `long` | `%ld` | `%ld` |
| `size_t` (what `sizeof` gives) | `%zu` | `%zu` |
| `double` | `%f`, `%.2f` | `%lf` |
| `float` | `%f` (promoted) | `%f` |
| `char` | `%c` | `%c` |
| string (`char *`) | `%s` | `%s` |
| any pointer | `%p` with `(void *)` | |
| a literal `%` | `%%` | |

- printf does not add a newline. Python's print does. (step 01)
- The template is a promise. Break it and you read the wrong register, not a
  corrupted value. (step 03)
- `scanf` needs `&`: `scanf("%d", &n)`. Without it you pass the value as an
  address. (step 07)
- `%f` in scanf reads a `float`, `%lf` reads a `double`. printf does not care;
  scanf does.

## Loops over an array

```c
int v[5] = {1, 2, 3, 4, 5};
size_t n = sizeof(v) / sizeof(v[0]);      /* only in the scope that declared it */

for (size_t i = 0; i < n; i++)   ...      /* < , never <=          (step 09) */
for (size_t i = n; i-- > 0; )    ...      /* backwards, no wrap    (step 23) */
```

`sizeof(v)/sizeof(v[0])` gives 2 inside a function that took `int v[]`,
because the parameter is a pointer. Pass the length. (step 10)

## Strings

```c
char buf[64];

snprintf(buf, sizeof buf, "%s is %d", name, age);   /* never sprintf/strcpy */
int wanted = snprintf(buf, sizeof buf, "%s", src);
if (wanted >= (int) sizeof buf) { /* truncated */ }

size_t len = strlen(buf);            /* O(n), it walks to the '\0' */
if (strcmp(a, b) == 0)               /* 0 means EQUAL */
fgets(buf, sizeof buf, stdin);       /* never gets() */
snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " more");  /* append */
```

- N characters need N+1 bytes. (step 11)
- `==` on strings compares addresses. (step 11)
- `char *s = "hi"` points at read-only memory. Write `const char *`. (step 33)
- strncpy does not guarantee a terminator. snprintf does. (step 13)

## Memory

```c
int *p = malloc(n * sizeof *p);      /* sizeof the THING, times n */
if (p == NULL) { perror("malloc"); return 1; }
...
free(p);
p = NULL;                            /* so a later use crashes loudly */

int *q = calloc(n, sizeof *q);       /* same, but zeroed */
```

`sizeof *p` rather than `sizeof(int)` stays correct when the type changes.

- One malloc, one free. (step 15)
- Never return the address of a local. (step 14)
- Say in a comment who frees. The compiler will not.

## Pointers and structs

```c
int  x = 5;
int *p = &x;      /* p holds the address */
*p = 7;           /* writes through it: x is now 7      */
p  = &y;          /* re-aims the pointer: x is untouched */

typedef struct { int id; double score; } Item;

Item  a;   a.id = 1;          /* I have the struct  -> dot   */
Item *b;   b->id = 1;         /* I have the address -> arrow */
```

Pass by pointer when the function must change it, by value when it only
reads. Mark read-only pointer parameters `const`. (steps 05, 06, 16)

## Command line arguments

```c
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <n> <threads>\n", argv[0]);
        return 1;
    }

    char *end;
    errno = 0;
    long n = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || errno == ERANGE || n <= 0) {
        fprintf(stderr, "error: bad n: \"%s\"\n", argv[1]);
        return 1;
    }
}
```

`atoi("abc")` is 0 and `atoi("1e9")` is 1, both silently. Use `strtol`, or
`strtod` when scientific notation is allowed. (steps 17, 18)

## pthreads skeleton

The shape the whole course is built on.

```c
#include <pthread.h>

typedef struct { int id; int *data; size_t from, to; long out; } Task;

void *worker(void *arg)
{
    Task *t = (Task *) arg;          /* cast the void * back */
    long sum = 0;
    for (size_t i = t->from; i < t->to; i++)
        sum += t->data[i];
    t->out = sum;                    /* my own slot, nobody else's */
    return NULL;
}

int main(void)
{
    pthread_t th[N];
    Task      task[N];               /* ONE PER THREAD, never one shared */

    for (int k = 0; k < N; k++) {
        task[k] = (Task){ k, data, k * chunk, (k + 1) * chunk, 0 };
        int rc = pthread_create(&th[k], NULL, worker, &task[k]);
        if (rc != 0) { fprintf(stderr, "create: %s\n", strerror(rc)); return 1; }
    }

    long total = 0;
    for (int k = 0; k < N; k++) {
        pthread_join(th[k], NULL);
        total += task[k].out;        /* combine AFTER the join */
    }
}
```

- The signature is exactly `void *name(void *arg)`. It cannot vary. (step 20)
- One argument slot per thread. `&i` of a loop variable is the classic bug.
- pthreads returns the error code; it does not set errno. `strerror(rc)`,
  never `perror`. (step 35)
- Compile with `-pthread`, at the end of the line.
- Private output slot plus combine after join needs no mutex at all.

## Timing

```c
#include <time.h>
struct timespec t0, t1;
clock_gettime(CLOCK_MONOTONIC, &t0);
...
clock_gettime(CLOCK_MONOTONIC, &t1);
double secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
```

Never `time()`: one second resolution, and it can jump backwards. Benchmark
at `-O2`, not with sanitizers. (step 39)

## Traps, one line each

| symptom | cause | step |
|---|---|---|
| `7 / 2` is 3 | int division, cast one side | 03 |
| garbage from printf | format does not match the argument | 03 |
| function changed nothing | C copies arguments, pass the address | 06 |
| corruption in an untouched variable | wrote past an array | 09 |
| `sizeof` gives 8 in a function | the array decayed to a pointer | 10 |
| string prints past its end | the `'\0'` was overwritten | 12 |
| right value, then wrong later | pointer to a dead local | 14 |
| `undefined reference` | a link error, no `#include` will fix it | 19, 31 |
| loop never ends counting down | `size_t` is unsigned, it wraps | 23 |
| `SQUARE(1+2)` is 5 | macros are text, parenthesise everything | 29 |
| printf output lost in a crash | stdout buffer never flushed | 34 |
| works at `-O0`, hangs at `-O2` | missing `volatile` on a signal flag | 37 |
| threads lose increments | `x++` is three operations | 38 (race) |
| 8 threads no faster than 4 | memory bound, not compute bound | 39 |

## Reading the manual

```bash
man 3 printf        # library function
man 2 write         # system call
man 7 pthreads      # the whole API on one page
man -k socket       # search by topic
```

The section number is the layer: 2 is the kernel, 3 is the C library. Knowing
which one you are reading is most of knowing where to look. (step 34)

For the tooling side, warning flags, sanitizers, `gdb`, `strace`, `perf`, see
[[tools]].
