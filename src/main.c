#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "al_obj.h"
#include "args_parser.h"
#include "misc.h"
#include "instr.h"

typedef enum NextToken
{
    NT_FLAG,
    NT_INPUT,
    NT_OUTPUT
} NextToken;

static const char *const DEFAULT_OUTPUT = "vcpu816-al.o";
static const unsigned INIT_BUF_CAPACITY = 64;

static void print_help(FILE *fp, const char *exec);
static int process_file(AsmState *state, const char *path);
static int process_line(AsmState *state, const char *line);
static void set_global(AsmState *state, const char *name, u16 value);

static int process_line(AsmState *state, const char *line)
{
    char *buffer = malloc(strlen(line) + 1);
    if(!buffer)
        out_of_memory();
    size_t buffer_pos = 0;
    size_t last_char = 0;
    int exit_code = 0;

    char ch;
    while((ch = *(line++)))
    {
        if(ch == ':')
        {
            buffer[buffer_pos] = 0;
            if(state->section)
                AlSection_set_label(state->section, buffer);
            else
            {
                fprintf(stderr, "%s:%u: no section is declared\n", state->path, state->line);
                free(buffer);
                return 1;
            }
            buffer_pos = 0;
        }
        else if(ch == ';')
            break;
        else if(!(buffer_pos == 0 && isspace(ch)))
        {
            buffer[buffer_pos++] = ch;
            if(!isspace(ch))
                last_char = buffer_pos - 1;
        }
    }
    size_t len = size_t_min(last_char + 1, buffer_pos);
    buffer[len] = 0;
    if(buffer[0])
    {
        const char *opcode = buffer;
        const char *args = 0;
        for(size_t i = 0; i < len; i++)
        {
            if(isspace(buffer[i]))
            {
                buffer[i] = 0;
                args = &buffer[i + 1];
                while(isspace(args[0]))
                    args++;
                break;
            }
        }

        if(opcode[0] == '.')
        {
            if(!strcmp(opcode, ".section"))
            {
                state->section = AlObj_add_section(&state->obj, args);
                if(!state->section)
                {
                    free(buffer);
                    return 1;
                }
            }
            else if(!strcmp(opcode, ".equ"))
            {
                ParserResult res = parse_args(state->global_names, state->global_values, state->num_globals, args);
                switch(res.status)
                {
                case PS_OK:
                    if(res.num_args == 2 && res.args[0].label && res.args[0].offset == 0 && res.args[1].label == 0)
                    {
                        if(is_reg(res.args[0].label))
                        {
                            fprintf(stderr, "Invalid use of register at %s:%u\n", state->path, state->line);
                            exit_code = 1;
                        }
                        else
                            set_global(state, res.args[0].label, res.args[1].offset);
                        free(res.args[0].label);
                    }
                    else
                    {
                        if(res.num_args == 2 && !res.args[0].label)
                            fprintf(stderr, "%s:%u: syntax error or symbol is already defined\n", state->path, state->line);
                        else
                            fprintf(stderr, "Syntax error at %s:%u\n", state->path, state->line);
                        for(unsigned arg = 0; arg < res.num_args; arg++)
                            if(res.args[arg].label)
                                free(res.args[arg].label);
                        exit_code = 1;
                    }
                    free(res.args);
                    break;
                
                case PS_ERR_SYNTAX:
                    fprintf(stderr, "Syntax error at %s:%u\n", state->path, state->line);
                    exit_code = 1;
                
                case PS_ERR_BAD_ARITH:
                    fprintf(stderr, "Bad expression at %s:%u\n", state->path, state->line);
                    exit_code = 1;
                }
            }
            else if(!strcmp(opcode, ".byte"))
            {
                if(state->section)
                {
                    ParserResult res = parse_args(state->global_names, state->global_values, state->num_globals, args);
                    switch(res.status)
                    {
                    case PS_OK:
                        for(unsigned i = 0; i < res.num_args; i++)
                        {
                            if(res.args[i].label)
                            {
                                if(is_reg(res.args[1].label))
                                {
                                    fprintf(stderr, "Invalid use of register at %s:%u\n", state->path, state->line);
                                    exit_code = 1;
                                }
                                if(!AlSection_set_reloc(state->section, state->section->len, res.args[i].label, AL_RELOC_BYTE))
                                {
                                    fprintf(stderr, "%s:%u: Too many relocations in section\n", state->path, state->line);;
                                    exit_code = 1;
                                }
                                free(res.args[i].label);
                            }
                            if(!AlSection_append(state->section, res.args[i].offset & 0xff))
                            {
                                fprintf(stderr, "%s:%u: Section is too long\n", state->path, state->line);
                                return 1;
                            }
                        }
                        free(res.args);
                        break;
                    
                    case PS_ERR_SYNTAX:
                        fprintf(stderr, "Syntax error at %s:%u\n", state->path, state->line);
                        free(buffer);
                        return 1;
                    
                    case PS_ERR_BAD_ARITH:
                        fprintf(stderr, "Bad expression at %s:%u\n", state->path, state->line);
                        free(buffer);
                        return 1;
                    }
                }
                else
                {
                    fprintf(stderr, "%s:%u: No section declared\n", state->path, state->line);
                    free(buffer);
                    return 1;
                }
            }
            else if(!strcmp(opcode, ".short"))
            {
                if(state->section)
                {
                    ParserResult res = parse_args(state->global_names, state->global_values, state->num_globals, args);
                    switch(res.status)
                    {
                    case PS_OK:
                        for(unsigned i = 0; i < res.num_args; i++)
                        {
                            if(res.args[i].label)
                            {
                                if(!AlSection_set_reloc(state->section, state->section->len, res.args[i].label, AL_RELOC_SHORT))
                                {
                                    fprintf(stderr, "%s:%u: Too many relocations in section\n", state->path, state->line);;
                                    return 1;
                                }
                                free(res.args[i].label);
                            }
                            if(!AlSection_append(state->section, res.args[i].offset & 0xff) ||
                                !AlSection_append(state->section, res.args[i].offset >> 8))
                            {
                                fprintf(stderr, "%s:%u: Section is too long\n", state->path, state->line);
                                return 1;
                            }
                        }
                        free(res.args);
                        break;
                    
                    case PS_ERR_SYNTAX:
                        fprintf(stderr, "Syntax error at %s:%u\n", state->path, state->line);
                        free(buffer);
                        return 1;
                    
                    case PS_ERR_BAD_ARITH:
                        fprintf(stderr, "Bad expression at %s:%u\n", state->path, state->line);
                        free(buffer);
                        return 1;
                    }
                }
                else
                {
                    fprintf(stderr, "%s:%u: No section declared\n", state->path, state->line);
                    free(buffer);
                    return 1;
                }
            }
            else
            {
                fprintf(stderr, "%s:%u: Unknown directive %s\n", state->path, state->line, opcode);
                free(buffer);
                return 1;
            }
        }
        else
        {
            ParserResult args_res = parse_args(state->global_names, state->global_values, state->num_globals, args);
            switch(args_res.status)
            {
            case PS_OK:
                exit_code = parse_instr(state, opcode, args_res.args, args_res.num_args);
                for(unsigned i = 0; i < args_res.num_args; i++)
                    if(args_res.args[i].label)
                        free(args_res.args[i].label);
                if(args_res.args)
                    free(args_res.args);
                break;
            
            case PS_ERR_SYNTAX:
                fprintf(stderr, "%s:%u: syntax error in instruction arguments\n", state->path, state->line);
                break;
            
            case PS_ERR_BAD_ARITH:
                fprintf(stderr, "%s:%u: bad label arithmetics in instruction arguments\n", state->path, state->line);
                break;
            }
        }
    }

    free(buffer);
    return exit_code;
}

static int process_file(AsmState *state, const char *path)
{
    FILE *fp = fopen(path, "r");
    if(fp)
    {
        state->path = path;
        char *buf = malloc(INIT_BUF_CAPACITY);
        if(!buf)
            out_of_memory();
        unsigned buf_capacity = INIT_BUF_CAPACITY;
        unsigned buf_pos = 0;

        while(!feof(fp))
        {
            char ch = fgetc(fp);
            if(ch == '\n')
            {
                buf[buf_pos] = 0;
                state->line++;
                int code = process_line(state, buf);
                if(code)
                {
                    fclose(fp);
                    return code;
                }
                buf_pos = 0;
            }
            else
            {
                if(buf_pos == buf_capacity - 1)
                {
                    unsigned new_cap = buf_capacity * 2;
                    char *new_buf = malloc(new_cap);
                    if(!new_buf)
                        out_of_memory();
                    memcpy(new_buf, buf, buf_pos);
                    free(buf);
                    buf = new_buf;
                    buf_capacity = new_cap;
                }
                buf[buf_pos++] = ch;
            }
        }

        free(buf);
        fclose(fp);
        return 0;
    }
    else
    {
        fprintf(stderr, "can't open file %s\n", path);
        return 1;
    }
}

static void set_global(AsmState *state, const char *name, u16 value)
{
    if(state->num_globals == state->globals_cap)
    {
        unsigned new_cap = state->globals_cap * 2;
        char **new_names = malloc(sizeof(char*) * new_cap);
        if(!new_names)
            out_of_memory();
        u16 *new_vals = malloc(sizeof(u16) * new_cap);
        if(!new_vals)
            out_of_memory();
        memcpy(new_names, state->global_names, sizeof(char*) * state->num_globals);
        memcpy(new_vals, state->global_values, sizeof(u16) * state->num_globals);
        free(state->global_names);
        state->global_names = new_names;
        free(state->global_values);
        state->global_values = new_vals;
    }
    char *name_container = malloc(strlen(name) + 1);
    if(!name_container)
        out_of_memory();
    strcpy(name_container, name);
    state->global_names[state->num_globals] = name_container;
    state->global_values[state->num_globals] = value;
    state->num_globals++;
}

int main(int argc, const char *const *argv)
{
    char show_version = 0,
        show_help = 0;
    unsigned input_count = 0,
        input_cap = 1;
    NextToken next_token = NT_FLAG;
    const char **input = malloc(sizeof(char*));
    char *output = malloc(strlen(DEFAULT_OUTPUT) + 1);
    strcpy(output, DEFAULT_OUTPUT);
    int exit_code = 0;

    for(int i = 1; i < argc; i++)
    {
        switch(next_token)
        {
        case NT_FLAG:
            if(argv[i][0] == '-')
            {
                if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
                    show_help = 1;
                else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
                    show_version = 1;
                else if(!strcmp(argv[i], "-o"))
                    next_token = NT_OUTPUT;
                else if(!strcmp(argv[i], "--"))
                    next_token = NT_INPUT;
            }
            else
            {
                if(input_count == input_cap)
                {
                    unsigned new_cap = input_cap * 2;
                    if(new_cap < input_cap)
                    {
                        fprintf(stderr, "Too many input files\n");
                        exit(1);
                    }
                    const char **new_input = malloc(sizeof(char*) * new_cap);
                    if(!new_input)
                        out_of_memory();
                    memcpy(new_input, input, sizeof(char*) * input_count);
                    free(input);
                    input = new_input;
                    input_cap = new_cap;
                }
                input[input_count++] = argv[i];
            }
            break;
        
        case NT_INPUT:
            if(input_count == input_cap)
            {
                unsigned new_cap = input_cap * 2;
                if(new_cap < input_cap)
                {
                    fprintf(stderr, "Too many input files\n");
                    exit(1);
                }
                const char **new_input = malloc(sizeof(char*) * new_cap);
                if(!new_input)
                    out_of_memory();
                memcpy(new_input, input, sizeof(char*) * input_count);
                free(input);
                input = new_input;
                input_cap = new_cap;
            }
            input[input_count++] = argv[i];
            break;

        case NT_OUTPUT:
            free(output);
            output = malloc(strlen(argv[i]) + 1);
            strcpy(output, argv[i]);
            break;
        }
    }

    if(show_version)
    {
        printf("%s (vcpu816-al-as) version " VERSION_STR 
            "\nMIT License\n\n"
            "Copyright (c) 2022 abirvalarg\n\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
            "of this software and associated documentation files (the \"Software\"), to deal\n"
            "in the Software without restriction, including without limitation the rights\n"
            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
            "copies of the Software, and to permit persons to whom the Software is\n"
            "furnished to do so, subject to the following conditions:\n"
            "\n"
            "The above copyright notice and this permission notice shall be included in all\n"
            "copies or substantial portions of the Software.\n"
            "\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
            "SOFTWARE.\n", argv[0]);
    }
    else if(show_help)
        print_help(stdout, argv[0]);
    else
    {
        if(input_count)
        {
            AsmState state = {
                .obj = {
                    .title = {0x7f, 0x41, 0x31, 0x36},
                    .version = 0,
                    .num_sections = 0,
                    .sections = malloc(sizeof(AlSectionEntry)),
                    .sections_capacity = 1,
                },
                .section = 0,
                .path = 0,
                .global_names = 0,
                .global_values = 0,
                .num_globals = 0,
                .globals_cap = 0,
                .line = 0
            };

            for(unsigned inp_id = 0; inp_id < input_count; inp_id++)
            {
                exit_code = process_file(&state, input[inp_id]);
                if(exit_code)
                    break;
            }

            if(exit_code == 0)
            {
                FILE *output_fp = fopen(output, "wb");
                if(output_fp)
                {
                    AlObj_write(&state.obj, output_fp);
                    fclose(output_fp);
                }
                else
                {
                    fprintf(stderr, "Failed to open output file\n");
                    exit_code = 1;
                }
            }

            AlObj_cleanup(&state.obj);
        }
        else
        {
            print_help(stderr, argv[0]);
            exit_code = 1;
        }
    }

    free(output);
    free(input);
    return exit_code;
}

static void print_help(FILE *fp, const char *exec)
{
    fprintf(fp, "Usage: %s [flags] <input files>\n"
        "flags:\n"
        "-h --help\tShow this message\n"
        "-v --version\tShow version and license\n"
        "-o\t\tSpecify output file, default is \"vcpu816-al.o\"\n"
        "--\t\tStop processing flags. Everything after this flag will be\n"
        "\t\tinterpreted as input file name\n", exec);
}
