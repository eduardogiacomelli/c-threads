/* ============================================================================
 * STEP 16 - struct: several boxes under one name.
 *
 * This is the last step before threads, and that is no coincidence: the only
 * way to pass more than one piece of data to a thread is to put it all in a
 * struct and pass the address of that.
 *
 *     Ctrl+Shift+B      (or: make 16)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A struct is a RECIPE for a memory layout: these fields, in this order,
 * glued together. Declared outside any function, it is visible to the whole
 * file.
 *
 * The `typedef` is what lets you write `Student x` later instead of
 * `struct Student x`. Convention, not obligation. */
typedef struct {
    char   name[20];
    int    age;
    double grade;
} Student;

/* BY VALUE: receives a COPY of the whole struct, all 32 bytes. It works, and
 * it is safe (the original cannot be changed), but it copies everything on
 * every call. For reading a small struct, that is fine. */
void show(Student s)
{
    /* a dot, because `s` is a struct, not a pointer */
    printf("   %s, %d years old, grade %.1f\n", s.name, s.age, s.grade);
}

/* BY POINTER: receives only the address, 8 bytes, and reaches the original.
 * This is the default: by pointer nearly always, with `const` when the
 * function only reads. */
void raise_grade(Student *s, double amount)
{
    /* `s->grade` is shorthand for `(*s).grade`, read as "the grade field of
     * the struct pointed at by s". The arrow exists because the parenthesised
     * form is unbearable to type. The parentheses would be mandatory:
     * `*s.grade` means something else, since . binds tighter than *. */
    s->grade += amount;
    if (s->grade > 10.0)
        s->grade = 10.0;
}

int main(void)
{
    /* Initialising by field name: readable, and independent of the order. */
    Student ana = { .name = "Ana", .age = 20, .grade = 7.5 };

    printf("struct Student takes %zu bytes ", sizeof(Student));
    printf("(%zu name + %zu age + %zu grade, plus alignment)\n\n",
           sizeof(ana.name), sizeof(ana.age), sizeof(ana.grade));

    printf("ana:\n");
    show(ana);

    raise_grade(&ana, 2.0);
    printf("after +2.0:\n");
    show(ana);

    /* ASSIGNMENT BETWEEN STRUCTS COPIES EVERYTHING. This is the interesting
     * exception: a struct you can copy with `=`; an array you cannot (step
     * 10). Copying a struct copies the arrays INSIDE it. */
    Student copy = ana;
    strcpy(copy.name, "Clone");
    printf("\nafter `copy = ana` and renaming the copy:\n");
    printf("   original: "); show(ana);
    printf("   copy:     "); show(copy);
    printf("   -> independent boxes\n");

    /* An array of structs: exactly step 08, with a bigger type. */
    Student group[3] = {
        { .name = "Bruno", .age = 21, .grade = 6.0 },
        { .name = "Carla", .age = 19, .grade = 9.5 },
        { .name = "Davi",  .age = 22, .grade = 8.0 },
    };

    printf("\ngroup:\n");
    for (size_t i = 0; i < sizeof(group) / sizeof(group[0]); i++)
        show(group[i]);

    /* A STRUCT ON THE HEAP, the pattern threads will demand.
     * Put step 15 together with this one: one block per unit of work, each
     * with its own data, all independent. */
    Student *fresh = malloc(sizeof(Student));   /* sizeof the TYPE, not the
                                                   pointer */
    if (fresh == NULL)
        return 1;

    /* With a pointer, every access uses the arrow. */
    strcpy(fresh->name, "Elisa");
    fresh->age   = 23;
    fresh->grade = 8.8;

    printf("\nstudent allocated on the heap:\n");
    show(*fresh);          /* *fresh dereferences: passes the struct by value */
    printf("   (the same, through the pointer: %s, grade %.1f)\n",
           fresh->name, fresh->grade);

    free(fresh);

    return 0;
}

/* ============================================================================
 * THE DIAGRAM
 *
 *   Student ana = { "Ana", 20, 7.5 };   one contiguous block:
 *
 *     0x7ffd1000  [ 'A''n''a' \0 ... ]   name[20]    20 bytes
 *     0x7ffd1014  [ 20 ]                 age          4 bytes
 *     0x7ffd1018  [ 7.5 ]                grade        8 bytes
 *                                                    -----------
 *                                        sizeof(Student) = 32
 *
 *   The compiler may insert empty bytes between fields so each type starts
 *   at an address that is a multiple of its own size (padding). That is why
 *   sizeof(struct) is not always the sum of the fields: check the output.
 *   Step 31 and the visualiser's byte view show this directly.
 *
 * DOT OR ARROW?
 *
 *     I have the struct     ->  ana.grade
 *     I have the address    ->  p->grade      (which is (*p).grade)
 *
 *   Getting this wrong is a compile error, not a silent bug. gcc says
 *   "invalid type argument of '->'". A rare relief in C.
 *
 * THE BRIDGE TO THREADS
 *
 *   pthread_create passes ONE argument, of type void *. Need to send three
 *   things to a thread? A struct, and pass its address:
 *
 *       typedef struct { int id; int *data; size_t from, to; } Task;
 *
 *       Task *t = malloc(sizeof(Task));    // one per thread (step 15)
 *       t->id = i;  ...
 *       pthread_create(&threads[i], NULL, worker, t);
 *
 *   Reusing a single struct for every thread is step 14's bug in different
 *   clothing: they all read the same box, and whoever wrote last wins.
 *
 * EXPERIMENTE:
 *
 *  1. Change `void show(Student s)` to `void show(Student *s)`, turn the
 *     dots into arrows and the calls into `&ana`. Compare the two: 32 bytes
 *     copied against 8.
 *
 *  2. Inside show (the by-value version), write `s.grade = 0;`. Print again
 *     in main. Nothing changed: step 06 all over again, now with a struct.
 *     Then mark the parameter `const Student *s` in the pointer version and
 *     try to change it: the compiler stops you. Use const whenever the
 *     function only reads.
 *
 *  3. Reorder the fields (double first, then int, then the name) and print
 *     sizeof(Student). The number can change because of padding.
 *
 *  4. Make an array of 3 pointers to Student, each with its own malloc, fill
 *     them in one loop and free them in a second. That is, almost letter for
 *     letter, the skeleton of a program with 3 threads.
 *
 * -> End of the fundamentals. Back to "00 - COMECE AQUI.md", then on to the
 *    pthreads tutorial.
 * ========================================================================= */
