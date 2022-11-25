#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "al_obj.h"

typedef enum NextToken
{
    NT_FLAG,
    NT_INPUT,
    NT_OUTPUT
} NextToken;

typedef struct AsmState
{
    AlObj obj;
    AlSection *section;
} AsmState;

static const char *const DEFAULT_OUTPUT = "vcpu816-al.o";
static const unsigned INIT_BUF_CAPACITY = 64;

static void print_help(FILE *fp, const char *exec);
static int process_file(AsmState *state, const char *path);
static int process_line(AsmState *state, const char *line);

static int process_line(AsmState *state, const char *line)
{
    printf("line: %s\n", line);
    return 0;
}

static int process_file(AsmState *state, const char *path)
{
    FILE *fp = fopen(path, "r");
    if(fp)
    {
        char *buf = malloc(INIT_BUF_CAPACITY);
        unsigned buf_capacity = INIT_BUF_CAPACITY;
        unsigned buf_pos = 0;

        while(!feof(fp))
        {
            char ch = fgetc(fp);
            if(ch == '\n')
            {
                buf[buf_pos] = 0;
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
                    .sections_capacity = 1
                },
                .section = 0
            };

            for(unsigned inp_id = 0; inp_id < input_count; inp_id++)
            {
                exit_code = process_file(&state, input[inp_id]);
                if(exit_code)
                    break;
            }

            if(exit_code == 0)
            {
                fprintf(stderr, "Output is not implemented yet\n");
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
