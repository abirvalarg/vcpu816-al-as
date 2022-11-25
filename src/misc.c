#include "misc.h"
#include <stdio.h>
#include <stdlib.h>

size_t size_t_min(size_t a, size_t b)
{
    if(a < b)
        return a;
    else
        return b;
}

void out_of_memory()
{
    fputs("out of memory\n", stderr);
    exit(255);
}
