#include "args_parser.h"
#include "misc.h"
#include <ctype.h>
#include <malloc.h>
#include <string.h>

static const size_t INIT_TOKEN_CAP = 64;

typedef struct ParserState
{
    const char *src;
    char *token;
    size_t token_len;
    size_t token_capacity;
    char **global_names;
    const u16 *global_vals;
    enum {
        TOK_NONE,
        TOK_IDENT,
        TOK_NUMBER,
        TOK_OP,
        TOK_CHAR,
        TOK_STRING
    } token_kind;
    unsigned num_globals;
} ParserState;

typedef struct SubparserResult
{
    Arg result;
    ParserStatus status;
} SubparserResult;

static SubparserResult parse_or_expr(ParserState *state);
static SubparserResult parse_xor_expr(ParserState *state);
static SubparserResult parse_and_expr(ParserState *state);
static SubparserResult parse_shift_expr(ParserState *state);
static SubparserResult parse_sum_expr(ParserState *state);
static SubparserResult parse_mul_expr(ParserState *state);
static SubparserResult parse_neg_expr(ParserState *state);
static SubparserResult parse_atom(ParserState *state);
static ParserStatus next_token(ParserState *state);
static void append_token_from_src(ParserState *state);

ParserResult parse_args(
    char **global_names,
    const u16 *global_vals,
    unsigned num_globals,
    const char *src
)
{
    if(!src)
    {
        return (ParserResult){
            .args = 0,
            .num_args = 0,
            .status = PS_OK
        };
    }
    Arg *args = malloc(sizeof(Arg));
    unsigned args_cap = 1;
    unsigned num_args = 0;

    ParserState state = {
        .src = src,
        .token = malloc(INIT_TOKEN_CAP),
        .token_capacity = INIT_TOKEN_CAP,
        .token_len = 0,
        .global_names = global_names,
        .global_vals = global_vals,
        .token_kind = TOK_NONE,
        .num_globals = num_globals
    };
    if(!state.token)
        out_of_memory();

    ParserStatus status = next_token(&state);
    if(status != PS_OK)
    {
        free(args);
        return (ParserResult){
            .status = status,
            .args = 0,
            .num_args = 0
        };
    }

    while(state.token_kind != TOK_NONE)
    {
        SubparserResult res = parse_or_expr(&state);
        if(res.status != PS_OK)
        {
            for(unsigned i = i; i < num_args; i++)
                if(args[i].label)
                    free(args[i].label);
            free(args);
            return (ParserResult){
                .status = status,
                .args = 0,
                .num_args = 0
            };
        }
        if(num_args == args_cap)
        {
            unsigned new_cap = args_cap * 2;
            Arg *new_args = malloc(sizeof(Arg) * new_cap);
            if(!new_args)
                out_of_memory();
            memcpy(new_args, args, sizeof(Arg) * num_args);
            free(args);
            args = new_args;
            args_cap = new_cap;
        }
        args[num_args++] = res.result;

        if(state.token_kind != TOK_NONE)
        {
            if(state.token_kind != TOK_OP || strcmp(state.token, ","))
            {
                free(args);
                return (ParserResult){
                    .status = PS_ERR_SYNTAX,
                    .args = 0,
                    .num_args = 0
                };
            }
            status = next_token(&state);
            if(status != PS_OK)
            {
                free(args);
                return (ParserResult){
                    .status = status,
                    .args = 0,
                    .num_args = 0
                };
            }
        }
    }

    free(state.token);
    return (ParserResult){
        .status = PS_OK,
        .args = args,
        .num_args = num_args
    };
}

static SubparserResult parse_or_expr(ParserState *state)
{
    SubparserResult res = parse_xor_expr(state);
    if(res.status != PS_OK)
        return res;
    Arg value = res.result;
    while(state->token_kind == TOK_OP && !strcmp(state->token, "|"))
    {
        if(value.label)
        {
            free(value.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_xor_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(rhs_res.result.label)
        {
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        value.offset |= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = value
    };
}

static SubparserResult parse_xor_expr(ParserState *state)
{
    SubparserResult res = parse_and_expr(state);
    if(res.status != PS_OK)
        return res;
    Arg value = res.result;
    while(state->token_kind == TOK_OP && !strcmp(state->token, "^"))
    {
        if(value.label)
        {
            free(value.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_and_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(rhs_res.result.label)
        {
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        value.offset ^= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = value
    };
}

static SubparserResult parse_and_expr(ParserState *state)
{
    SubparserResult res = parse_shift_expr(state);
    if(res.status != PS_OK)
        return res;
    Arg value = res.result;
    while(state->token_kind == TOK_OP && !strcmp(state->token, "&"))
    {
        if(value.label)
        {
            free(value.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_shift_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(rhs_res.result.label)
        {
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        value.offset |= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = value
    };
}

static SubparserResult parse_shift_expr(ParserState *state)
{
    SubparserResult res = parse_sum_expr(state);
    if(res.status != PS_OK)
        return res;
    Arg value = res.result;
    while(state->token_kind == TOK_OP && (!strcmp(state->token, "<<") || !strcmp(state->token, ">>")))
    {
        char op = state->token[0];
        if(value.label)
        {
            free(value.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_sum_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(rhs_res.result.label)
        {
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        if(op == '<')
            value.offset <<= rhs_res.result.offset;
        else
            value.offset >>= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = value
    };
}

static SubparserResult parse_sum_expr(ParserState *state)
{
    SubparserResult lhs_res = parse_mul_expr(state);
    if(lhs_res.status != PS_OK)
        return lhs_res;
    Arg val = lhs_res.result;
    while(state->token_kind == TOK_OP && (!strcmp(state->token, "+") || !strcmp(state->token, "+")))
    {
        char op = state->token[0];
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            if(val.label)
                free(val.label);
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_mul_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(val.label && rhs_res.result.label)
        {
            free(val.label);
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        if(!val.label)
            val.label = rhs_res.result.label;
        if(op == '+')
            val.offset += rhs_res.result.offset;
        else
            val.offset -= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = val
    };
}

static SubparserResult parse_mul_expr(ParserState *state)
{
    SubparserResult res = parse_neg_expr(state);
    if(res.status != PS_OK)
        return res;
    Arg value = res.result;
    while(state->token_kind == TOK_OP && (!strcmp(state->token, "*") || !strcmp(state->token, "/") || !strcmp(state->token, "%")))
    {
        char op = state->token[0];
        if(value.label)
        {
            free(value.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult rhs_res = parse_neg_expr(state);
        if(rhs_res.status != PS_OK)
            return rhs_res;
        if(rhs_res.result.label)
        {
            free(rhs_res.result.label);
            return (SubparserResult){
                .status = PS_ERR_BAD_ARITH,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        if(op == '*')
            value.offset *= rhs_res.result.offset;
        else if(op == '/')
            value.offset /= rhs_res.result.offset;
        else
            value.offset %= rhs_res.result.offset;
    }
    return (SubparserResult){
        .status = PS_OK,
        .result = value
    };
}

static SubparserResult parse_neg_expr(ParserState *state)
{
    char op = 0;
    if(state->token_kind == TOK_OP && (!strcmp(state->token, "-") || !strcmp(state->token, "!")))
    {
        op = state->token[0];
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
    }
    SubparserResult res = parse_atom(state);
    if(res.status != PS_OK)
        return res;
    if(op && res.result.label)
    {
        free(res.result.label);
        return (SubparserResult){
            .status = PS_ERR_BAD_ARITH,
            .result = {
                .label = 0,
                .offset = 0
            }
        };
    }
    if(op == '-')
        res.result.offset *= -1;
    else if(op == '!')
        res.result.offset ^= -1;
    return res;
}

static SubparserResult parse_atom(ParserState *state)
{
    if(state->token_kind == TOK_IDENT)
    {
        for(unsigned i = 0; i < state->num_globals; i++)
        {
            if(!strcmp(state->global_names[i], state->token))
            {
                ParserStatus st = next_token(state);
                if(st != PS_OK)
                {
                    return (SubparserResult){
                        .status = st,
                        .result = {
                            .label = 0,
                            .offset = 0
                        }
                    };
                }
                return (SubparserResult){
                    .status = PS_OK,
                    .result = {
                        .label = 0,
                        .offset = state->global_vals[i]
                    }
                };
            }
        }

        char *label = malloc(state->token_len + 1);
        strcpy(label, state->token);
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            free(label);
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        return (SubparserResult){
            .status = PS_OK,
            .result = {
                .label = label,
                .offset = 0
            }
        };
    }
    else if(state->token_kind == TOK_NUMBER)
    {
        unsigned radix = 10;
        u16 res = 0;
        for(unsigned pos = 0; pos < state->token_len; pos++)
        {
            char ch = state->token[pos];
            if(pos == 1 && res == 0 && strchr("box", ch))
            {
                switch(state->token[pos])
                {
                case 'b':
                    radix = 2;
                    break;
                
                case 'o':
                    radix = 8;
                    break;
                
                case 'x':
                    radix = 16;
                    break;
                }
            }
            else if(ch != '_')
            {
                char digit_good;
                switch(radix)
                {
                case 2:
                    digit_good = ch == '0' || ch == '1';
                    break;
                
                case 8:
                    digit_good = ch >= '0' && ch < '8';
                    break;
                
                case 10:
                    digit_good = !!isdigit(ch);
                    break;
                
                case 16:
                    digit_good = !!isxdigit(ch);
                    break;
                }
                if(digit_good)
                {
                    res *= radix;
                    if(ch <= '9')
                        res += ch - '0';
                    else
                        res += (ch | ' ') - 'a' + 10;
                }
                else
                {
                    return (SubparserResult){
                        .status = PS_ERR_SYNTAX,
                        .result = {
                            .label = 0,
                            .offset = 0
                        }
                    };
                }
            }
        }
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        return (SubparserResult){
            .status = PS_OK,
            .result = {
                .label = 0,
                .offset = res
            }
        };
    }
    else if(state->token_kind == TOK_OP && !strcmp(state->token, "("))
    {
        ParserStatus st = next_token(state);
        if(st != PS_OK)
        {
            return (SubparserResult){
                .status = st,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
        SubparserResult res = parse_or_expr(state);
        if(state->token_kind == TOK_OP && !strcmp(state->token, ")"))
        {
            st = next_token(state);
            if(st != PS_OK)
            {
                if(res.result.label)
                    free(res.result.label);
                return (SubparserResult){
                    .status = st,
                    .result = {
                        .label = 0,
                        .offset = 0
                    }
                };
            }
            return res;
        }
        else
        {
            return (SubparserResult){
                .status = PS_ERR_SYNTAX,
                .result = {
                    .label = 0,
                    .offset = 0
                }
            };
        }
    }
    else
    {
        return (SubparserResult){
            .status = PS_ERR_SYNTAX,
            .result = {
                .label = 0,
                .offset = 0
            }
        };
    }
}

static ParserStatus next_token(ParserState *state)
{
    while(isspace(state->src[0]))
        state->src++;
    
    state->token[0] = 0;
    state->token_len = 0;
    if(!state->src[0])
        state->token_kind = TOK_NONE;
    else if(isdigit(state->src[0]))
    {
        state->token_kind = TOK_NUMBER;
        while(isalnum(state->src[0]) || state->src[0] == '_')
        {
            if(state->src[0] == '_')
                state->src++;
            else
                append_token_from_src(state);
        }
    }
    else if(isalpha(state->src[0]) || state->src[0] == '_' || state->src[0] == '.')
    {
        state->token_kind = TOK_IDENT;
        while(isalnum(state->src[0]) || state->src[0] == '_' || state->src[0] == '.')
            append_token_from_src(state);
    }
    else
    {
        state->token_kind = TOK_OP;
        append_token_from_src(state);
        if((state->token[0] == '<' || state->token[0] == '>') && state->src[0] == state->token[0])
            append_token_from_src(state);
    }

    return PS_OK;
}

static void append_token_from_src(ParserState *state)
{
    if(state->token_len == state->token_capacity - 1)
    {
        size_t new_cap = state->token_capacity * 2;
        char *new_token = malloc(new_cap);
        if(!new_token)
            out_of_memory();
        memcpy(new_token, state->token, state->token_len);
        free(state->token);
        state->token = new_token;
        state->token_capacity = new_cap;
    }
    state->token[state->token_len++] = state->src[0];
    state->token[state->token_len] = 0;
    state->src++;
}
