#include "al_obj.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "misc.h"

void AlObj_cleanup(AlObj *self)
{
    for(unsigned sec_id = 0; sec_id < self->num_sections; sec_id++)
    {
        free(self->sections[sec_id].name);
        AlSection *sec = self->sections[sec_id].section;

        for(unsigned sym_id = 0; sym_id < sec->num_sym; sym_id++)
            free(sec->symbols[sym_id].name);
        free(sec->symbols);

        for(unsigned reloc_id = 0; reloc_id < sec->num_reloc; reloc_id++)
            free(sec->reloc[reloc_id].symbol);
        free(sec->reloc);

        free(sec);
    }
}

u8 AlSection_symbol_exists(AlSection *section, const char *name)
{
    for(unsigned lbl_id = 0; lbl_id < section->num_sym; lbl_id++)
        if(!strcmp(section->symbols[lbl_id].name, name))
            return 1;
    return 0;
}

u8 AlSection_set_label(AlSection *section, const char *name)
{
    for(unsigned lbl_id = 0; lbl_id < section->num_sym; lbl_id++)
    {
        if(!strcmp(section->symbols[lbl_id].name, name))
        {
            section->symbols[lbl_id].value = section->len;
            return 1;
        }
    }

    if(section->num_sym == section->sym_capacity)
    {
        if(section->sym_capacity == 0xffff)
        {
            fprintf(stderr, "too many symbols\n");
            return 0;
        }
        u16 new_cap = section->sym_capacity * 2;
        if(new_cap < section->sym_capacity)
            new_cap = 0xffff;
        AlSymbol *new_syms = malloc(sizeof(AlSymbol) * new_cap);
        if(!new_syms)
            out_of_memory();
        memcpy(new_syms, section->symbols, sizeof(AlSymbol) * section->num_sym);
        free(section->symbols);
        section->symbols = new_syms;
        section->sym_capacity = new_cap;
    }
    section->symbols[section->num_sym].name = malloc(strlen(name) + 1);
    strcpy(section->symbols[section->num_sym].name, name);
    section->symbols[section->num_sym].value = section->len;
    section->num_sym++;
    return 1;
}

AlSection *AlObj_add_section(AlObj *self, const char *name)
{
    for(int i = 0; i < self->num_sections; i++)
    {
        if(!strcmp(self->sections[i].name, name))
            return self->sections[i].section;
    }
    AlSection *section = malloc(sizeof(AlSection));
    if(!section)
        out_of_memory();
    memset(section, 0, sizeof(AlSection));
    
    if(self->num_sections == self->sections_capacity)
    {
        if(self->num_sections == 0xffff)
        {
            fprintf(stderr, "error: Too many sections\n");
            return 0;
        }
        u16 new_cap = self->sections_capacity * 2;
        if(new_cap < self->sections_capacity)
            new_cap = 0xffff;
        AlSectionEntry *new_sections = malloc(sizeof(AlSectionEntry) * new_cap);
        if(!new_sections)
            out_of_memory();
        memcpy(new_sections, self->sections, sizeof(AlSectionEntry) * self->sections_capacity);
        free(self->sections);
        self->sections = new_sections;
        self->sections_capacity = new_cap;
    }
    self->sections[self->num_sections].section = section;
    self->sections[self->num_sections].name = malloc(strlen(name) + 1);
    strcpy(self->sections[self->num_sections].name, name);
    self->num_sections++;
    return section;
}
