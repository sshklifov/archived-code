#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Disclaimer
 * This programme does not perform any freeing of
 * resources unless necessary for the normal operation
 * of the programme. The kernel will automatically free
 * any pages not explicitly freed by this programme.
 */

int main ()
{
    /* Allocate some memory */

    int* p[100];

    for (int i = 0; i < 100; i++)
    {
        p[i] = malloc (128 * sizeof(int));
    }

    /* Force malloc to change programme break */

    void* curr_brk = sbrk (0);
    void* last_block;

    while (curr_brk == sbrk(0))
    {
        last_block = malloc (128 * sizeof(int));
    }

    /*
     * Delete culprit (last block which caused malloc to increase the programme break),
     * It is assumed that malloc will set values which as multiples of a page size, which
     * is both reasonable and logical. It is also assumed that free will discard blocks at
     * the end of the heap (it may not call brk, however).
     */

    void* before_free_brk = sbrk(0);
    free (last_block);

    /* Check if programme break was changed */

    if (curr_brk == sbrk(0))
    {
        fprintf (stderr, "free() not implemented as expected\n");
        fprintf (stderr, "current brk: %p\nbefore free() brk: %p\nafter free() brk: %p\n", curr_brk, before_free_brk, sbrk(0));
        exit (EXIT_FAILURE);
    }

    else
    {
        printf ("current brk: %p\nbefore free() brk: %p\nafter free() brk: %p\n", curr_brk, before_free_brk, sbrk(0));
        curr_brk = sbrk (0); // update brk
    }

    /*
     * Free enough space so that a block of 512 ints can fit.
     * Notice that the middle blocks are removed (not at the end).
     */

    free (p[49]);
    free (p[50]);
    free (p[51]);

    /* Now allocate the memory... */

    void* addr = malloc (256 * sizeof(int));
    printf ("freed memory in region (roughly): %p-%p\n", p[49], p[51] + 128);
    printf ("malloc returned address: %p (with %p end)\n", addr, (int*)addr + 256);
    printf ("brk is: %p", sbrk (0));

    /* and check if malloc requesed a new page */

    printf ("defagmentation %simplemented\n", curr_brk == sbrk (0) ? ""  : "not ");

    return 0;
}
