#pragma once
#include "types.h"
#include <stdio.h>

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

u8 AlSection_symbol_exists(AlSection *section, const char *name);
u8 AlSection_set_label(AlSection *section, const char *name);
u8 AlSection_append(AlSection *section, u8 value);
u8 AlSection_set_reloc(AlSection *section, u16 position, const char *symbol, AlRelocKind kind);

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
AlSection *AlObj_add_section(AlObj *self, const char *name);
void AlObj_write(AlObj *self, FILE *fp);
