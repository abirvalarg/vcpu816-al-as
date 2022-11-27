#pragma once
#include "types.h"

typedef struct Arg
{
    char *label;
    u16 offset;
} Arg;

typedef enum ParserStatus
{
    PS_OK,
    PS_ERR_SYNTAX,
    PS_ERR_BAD_ARITH
} ParserStatus;

typedef struct ParserResult
{
    Arg *args;
    unsigned num_args;
    ParserStatus status;
} ParserResult;

ParserResult parse_args(
    const char *const *global_names,
    u16 *global_vals,
    unsigned num_globals,
    const char *src
);
