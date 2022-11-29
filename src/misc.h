#pragma once
#include <stddef.h>
#include "types.h"
#include "al_obj.h"

typedef struct AsmState
{
    AlObj obj;
    AlSection *section;
    const char *path;
    char **global_names;
    u16 *global_values;
    unsigned num_globals;
    unsigned globals_cap;
    unsigned line;
} AsmState;

size_t size_t_min(size_t a, size_t b);

__attribute__((noreturn))
void out_of_memory();

u8 is_reg(const char *token);
