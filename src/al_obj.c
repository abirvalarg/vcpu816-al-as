#include "al_obj.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "misc.h"

static const u16 INIT_SECTION_CAPACITY = 64;
const char ARCH[8] = {'V', 'C', 'P', 'U', '8', '1', '6', ' '};

void AlObj_cleanup(AlObj *self)
{
    for(unsigned sec_id = 0; sec_id < self->num_sections; sec_id++)
    {
        free(self->sections[sec_id].name);
        AlSection *sec = self->sections[sec_id].section;

        for(unsigned sym_id = 0; sym_id < sec->num_sym; sym_id++)
            free(sec->symbols[sym_id].name);
        if(sec->symbols)
            free(sec->symbols);

        for(unsigned reloc_id = 0; reloc_id < sec->num_reloc; reloc_id++)
            free(sec->reloc[reloc_id].symbol);
        if(sec->reloc)
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
        u16 new_cap = section->sym_capacity ? section->sym_capacity * 2 : 1;
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

u8 AlSection_append(AlSection *section, u8 value)
{
    if(section->len == section->capacity)
    {
        if(section->capacity == 0xffff)
            return 0;
        u16 new_cap = section->capacity ? section->capacity * 2 : INIT_SECTION_CAPACITY;
        if(new_cap < section->capacity)
            new_cap = 0xffff;
        u8 *new_data = malloc(new_cap);
        if(!new_data)
            out_of_memory();
        memcpy(new_data, section->data, section->len);
        if(section->data)
            free(section->data);
        section->data = new_data;
        section->capacity = new_cap;
    }
    section->data[section->len++] = value;
    return 1;
}

u8 AlSection_set_reloc(AlSection *section, u16 position, const char *symbol, AlRelocKind kind)
{
    if(section->num_reloc == section->reloc_capacity)
    {
        if(section->reloc_capacity == 0xffff)
            return 0;
        u16 new_cap = section->reloc_capacity ? section->reloc_capacity * 2 : 1;
        if(new_cap < section->reloc_capacity)
            new_cap = 0xffff;
        AlRelocEntry *new_reloc = malloc(sizeof(AlRelocEntry) * new_cap);
        if(!new_reloc)
            out_of_memory();
        memcpy(new_reloc, section->reloc, sizeof(AlRelocEntry) * section->num_reloc);
        if(section->reloc)
            free(section->reloc);
        section->reloc = new_reloc;
        section->reloc_capacity = new_cap;
    }
    char *sym_buf = malloc(strlen(symbol) + 1);
    strcpy(sym_buf, symbol);
    section->reloc[section->num_reloc++] = (AlRelocEntry){
        .kind = kind,
        .offset = position,
        .symbol = sym_buf
    };
    return 1;
}

void AlObj_write(AlObj *self, FILE *fp)
{
    // title
    for(int i = 0; i < 4; i++)
        fputc(self->title[i], fp);
    
    // version
    fputc(0, fp);
    fputc(0, fp);

    // num_sections
    fputc(self->num_sections & 0xff, fp);
    fputc(self->num_sections >> 8, fp);

    // arch
    for(int i = 0; i < 8; i++)
        fputc(ARCH[i], fp);
    
    size_t pos = 16 + self->num_sections * 9;
    for(unsigned i = 0; i < self->num_sections; i++)
        pos += strlen(self->sections[i].name);
    
    // section list
    for(unsigned s = 0; s < self->num_sections; s++)
    {
        AlSection *sect = self->sections[s].section;

        fputc(pos & 0xff, fp);
        fputc((pos >> 8) & 0xff, fp);
        fputc((pos >> 16) & 0xff, fp);
        fputc((pos >> 24) & 0xff, fp);

        u32 sect_len = 6 + sect->num_sym * 3 + sect->num_reloc * 4 + sect->len;
        for(unsigned i = 0; i < sect->num_sym; i++)
            sect_len += strlen(sect->symbols[i].name);
        for(unsigned i = 0; i < sect->num_reloc; i++)
            sect_len += strlen(sect->reloc[i].symbol);

        fputc(sect_len & 0xff, fp);
        fputc((sect_len >> 8) & 0xff, fp);
        fputc((sect_len >> 16) & 0xff, fp);
        fputc((sect_len >> 24) & 0xff, fp);

        const char *name = self->sections[s].name;
        while(*name)
        {
            fputc(*name, fp);
            name++;
        }
        fputc(0, fp);

        pos += sect_len;
    }

    for(unsigned s = 0; s < self->num_sections; s++)
    {
        AlSection *sect = self->sections[s].section;

        fputc(sect->num_sym & 0xff, fp);
        fputc(sect->num_sym >> 8, fp);

        fputc(sect->num_reloc & 0xff, fp);
        fputc(sect->num_reloc >> 8, fp);

        fputc(sect->len & 0xff, fp);
        fputc(sect->len >> 8, fp);

        for(unsigned i = 0; i < sect->num_sym; i++)
        {
            fputc(sect->symbols[i].value & 0xff, fp);
            fputc(sect->symbols[i].value >> 8, fp);

            const char *name = sect->symbols[i].name;
            while(*name)
            {
                fputc(*name, fp);
                name++;
            }
            fputc(0, fp);
        }

        for(unsigned i = 0; i < sect->num_reloc; i++)
        {
            fputc(sect->reloc[i].offset & 0xff, fp);
            fputc(sect->reloc[i].offset >> 8, fp);

            fputc(sect->reloc[i].kind, fp);

            const char *name = sect->reloc[i].symbol;
            while(*name)
            {
                fputc(*name, fp);
                name++;
            }
            fputc(0, fp);
        }

        for(unsigned i = 0; i < sect->len; i++)
            fputc(sect->data[i], fp);
    }
}
