#pragma once
#include "types.h"

typedef enum AlRelocKind
{
    AL_RELOC_INSTR = 0,
    AL_RELOC_BYTE = 1,
    AL_RELOC_SHORT = 2
} AlRelocKind;

typedef struct AlSymbol
{
    char *name;
    u16 value;
} AlSymbol;

typedef struct AlRelocEntry
{
    u16 offset;
    AlRelocKind kind;
    char *symbol;
} AlRelocEntry;

typedef struct AlSection
{
    AlSymbol *symbols;
    AlRelocEntry *reloc;
    u8 *data;
    u16 num_sym;
    u16 sym_capacity;
    u16 num_reloc;
    u16 reloc_capacity;
    u16 len;
    u16 capacity;
} AlSection;

typedef struct AlSectionEntry
{
    char *name;
    AlSection *section;
} AlSectionEntry;

typedef struct AlObj
{
    char title[4];
    u16 version;
    u16 num_sections;
    AlSectionEntry *sections;
    u16 sections_capacity;
} AlObj;

void AlObj_cleanup(AlObj *self);
