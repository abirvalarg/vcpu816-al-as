#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

u8 is_reg(const char *token)
{
    char *buffer = malloc(strlen(token) + 1);
    unsigned pos = 0;
    while(token[pos])
    {
        buffer[pos] = token[pos] | ' ';
        pos++;
    }
    return !strcmp(buffer, "x") || !strcmp(buffer, "y") || !strcmp(token, "sp") || !strcmp(buffer, "pc")
        || !strcmp(buffer, "xh") || !strcmp(buffer, "yh") || !strcmp(token, "sh") || !strcmp(buffer, "ph")
        || !strcmp(buffer, "xl") || !strcmp(buffer, "yl") || !strcmp(token, "sl") || !strcmp(buffer, "pl");
}
