#pragma once
#include "args_parser.h"
#include "misc.h"

int parse_instr(AsmState *state, const char *opcode, Arg *args, unsigned num_args);
