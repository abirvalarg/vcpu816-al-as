#include "instr.h"
#include "al_obj.h"
#include <stdio.h>
#include <string.h>

typedef struct Instruction
{
    const char *name;
    int (*func)(AsmState *state, const Arg *args, unsigned num_args);
} Instruction;

// misc
static int nop(AsmState *state, const Arg *args, unsigned num_args);

// memory
static int ldi(AsmState *state, const Arg *args, unsigned num_args);
static int lda(AsmState *state, const Arg *args, unsigned num_args);
static int sta(AsmState *state, const Arg *args, unsigned num_args);

// control flow
static int jmp(AsmState *state, const Arg *args, unsigned num_args);

// ALU
static int addi(AsmState *state, const Arg *args, unsigned num_args);

static const Instruction INSTR[] = {
    { "nop", nop },
    { "ldi", ldi },
    { "lda", lda },
    { "sta", sta },
    { "jmp", jmp },
    { "addi", addi },
    { 0, 0 }
};

int parse_instr(AsmState *state, const char *opcode, Arg *args, unsigned num_args)
{
    unsigned i = 0;
    while(INSTR[i].name)
    {
        if(!strcmp(INSTR[i].name, opcode))
            return INSTR[i].func(state, args, num_args);
        i++;
    }
    fprintf(stderr, "%s:%u: unknown instruction\n", state->path, state->line);
    return 1;
}

static int nop(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args)
        fprintf(stderr, "%s:%u: NOP instruction has no arguments, they will be ignored\n", state->path, state->line);
    AlSection_append(state->section, 0);
    return 0;
}

static int ldi(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args != 1 || args[0].label)
    {
        fprintf(stderr, "%s:%u: LDI instruction has 1 argument that has to be a number\n", state->path, state->line);
        return 1;
    }
    AlSection_append(state->section, 1);
    AlSection_append(state->section, args[0].offset & 0xff);
    return 0;
}

static int lda(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args != 1)
    {
        fprintf(stderr, "%s:%u: LDA instruction has exactly 1 argument\n", state->path, state->line);
        return 1;
    }
    if(args[0].label)
        AlSection_set_reloc(state->section, state->section->len, args[0].label, AL_RELOC_INSTR);
    AlSection_append(state->section, 2);
    AlSection_append(state->section, args[0].offset & 0xff);
    AlSection_append(state->section, args[0].offset >> 8);
    return 0;
}

static int sta(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args != 1)
    {
        fprintf(stderr, "%s:%u: STA instruction has exactly 1 argument\n", state->path, state->line);
        return 1;
    }
    if(args[0].label)
        AlSection_set_reloc(state->section, state->section->len, args[0].label, AL_RELOC_INSTR);
    AlSection_append(state->section, 3);
    AlSection_append(state->section, args[0].offset & 0xff);
    AlSection_append(state->section, args[0].offset >> 8);
    return 0;
}

static int jmp(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args != 1 || !args[0].label)
    {
        fprintf(stderr, "%s:%u: JMP instruction requires 1 label\n", state->path, state->line);
        return 1;
    }
    AlSection_set_reloc(state->section, state->section->len, args[0].label, AL_RELOC_INSTR);
    AlSection_append(state->section, 4);
    AlSection_append(state->section, args[0].offset & 0xff);
    AlSection_append(state->section, args[0].offset >> 8);
    return 0;
}

static int addi(AsmState *state, const Arg *args, unsigned num_args)
{
    if(num_args != 1 || args[0].label)
    {
        fprintf(stderr, "%s:%u: ADDI instruction has 1 argument that has to be a number\n", state->path, state->line);
        return 1;
    }
    AlSection_append(state->section, 0x50);
    AlSection_append(state->section, args[0].offset & 0xff);
    return 0;
}
