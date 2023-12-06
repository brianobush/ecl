#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"
#include "ecl.h"

#define BASE36 36

void ecl_reset(ecl_t *ecl)
{
    int i;
    if (ecl)
    {
        for (i = 0; i < ecl->memsz; i++)
        {
            ecl->mem[i] = '.';
        }
        for (i = 0; i < BASE36; i++)
        {
            ecl->vars[i] = '.';
        }
        memset(ecl->state, 0, ecl->memsz);
        ecl->clock = 0;
    }
}

ecl_t *ecl_new(int x, int y, unsigned long seed)
{
    ecl_t *ecl = calloc(1, sizeof(ecl_t));
    ecl->width = x;
    ecl->height = y;
    ecl->memsz = ecl->width * ecl->height;
    ecl->mem = calloc(ecl->memsz, sizeof(char));
    ecl->state = calloc(ecl->memsz, sizeof(int));
    ecl->rng = rng_new(seed);
    ecl_reset(ecl);
    return ecl;
}

void ecl_free(ecl_t *ecl)
{
    if (ecl)
    {
        free(ecl->mem);
        free(ecl->state);
        rng_free(ecl->rng);
        free(ecl);
    }
}

void ecl_set_output(ecl_t *ecl,
                    void (*output_fn)(int channel, int note, int octave, int velocity, int length, void *ctx),
                    void *ctx)
{
    ecl->output_fn = output_fn;
    ecl->output_ctx = ctx;
}

int char2int(char c)
{
    if (c == '.')
    {
        return 0;
    }
    else if ((c >= '0') && (c <= '9'))
    {
        return c - '0';
    }
    else if ((c >= 'a') && (c <= 'z'))
    {
        return c - 'a' + 10;
    }
    return 0;
}

char int2char(int v)
{
    v %= BASE36; /* modulo, sice we are in base 36 */
    if (v < 0)
    {
        v *= -1; /* positive only */
    }
    if (v >= 0 && v <= 9)
    {
        return '0' + v;
    }
    else
    {
        return 'a' + (v - 10);
    }
}

int is_number(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'a') && (c <= 'z')));
}

int is_command(char c)
{
    return (c >= 'A' && c <= 'Z') || c == '<' || c == '>' || c == '$';
}

int is_special(char c)
{
    return c == '?';
}

int is_empty(int c)
{
    return c == '.';
}

int valid_char(char c)
{
    return is_number(c) || is_command(c) || is_special(c);
}

/* Get the value at memory position x */
char ecl_get(ecl_t *ecl, int x)
{
    if (ecl)
    {
        x = abs(x);
        return ecl->mem[x % ecl->memsz];
    }
    return '.';
}

/* Get the state at memory position x */
int ecl_get_state(ecl_t *ecl, int x)
{
    if (ecl)
    {
        x = abs(x);
        return ecl->state[x % ecl->memsz];
    }
    return STATE_ERR;
}

void ecl_set(ecl_t *ecl, int x, char val)
{
    if (ecl)
    {
        //printf("inserting in ecl %d val %c\n", x, val);
        ecl->mem[x % ecl->memsz] = valid_char(val) ? val : '.';
    }
}

/* Set the state at memory position x */
void ecl_set_state(ecl_t *ecl, int x, int val)
{
    if (ecl)
    {
        ecl->state[x % ecl->memsz] = val;
    }
}

/* Dump memory to stdout */
void ecl_dump(ecl_t *ecl)
{
    int x;
    if (ecl)
    {
        printf("0123456789abcdef\n");
        for (x = 0; x < ecl->memsz; x++)
        {
            printf("%c", ecl->mem[x]);
        }
        printf("\n\n");
    }
}

typedef struct
{
    char cmd;
    int pure,    /* always bangs, no bang input required */
        varargs, /* argument length specifed by first argument */
        bangs,   /* number of inputs required for a bang event; default value if varbangs true */
        args;    /* arg counts */
} arg_t;

arg_t ARGS[] = {
    /* Ideas: 
    Accumulate (or decrease) values in a register
    Conditional: Jump if register is zero
    Unnamed counter/decrementer 
    */

    {'A', 0, 0, 1, 1}, /* accumulate values; argument is register storage */
    // Burst
    {'C', 0, 0, 1, 1}, /* produce a constant value on bang */
    {'D', 0, 0, 1, 1}, /* decrement value of bang by arg (def 1) on output */
    {'E', 0, 0, 1, 3}, /* Eucliden clock, args: pulses, steps, current */
    {'F', 0, 0, 1, 1}, /* if bang value matches argument, allow value to pass otherwise block */
    {'G', 1, 0, 0, 2}, /* pure generator;  pure creators of bangs, args: rate, max */
    // H
    {'I', 0, 0, 1, 1}, /* increment value of bang by arg (def 1) on output */
    {'J', 0, 0, 1, 1}, /* jump bang value a specified number of cells  */
    // K
    // L   limit?
    {'M', 0, 0, 1, 1}, /* mod; bang with x, arg is y, output x%y */
    // N
    {'O', 0, 0, 1, 5}, /* Output to a device (midi); channel, octave, note, velocity, length */  
    // Add repeat to output?
    {'P', 0, 0, 1, 1}, /* continue bang probabilistically */
    {'Q', 0, 0, 1, 1}, /* query a register on bang */
    {'R', 0, 0, 1, 2}, /* randomize; no args -> binary */
    {'S', 0, 1, 1, 1}, /* store a specified length (sequence) of numbers */
    {'T', 0, 0, 1, 1}, /* teleport a bang to a channel */
    // U
    {'V', 0, 0, 1, 2}, /* Store bang value into a named register */
    // W
    {'X', 0, 0, 1, 0}, /* Kill a bang */
    // Y
    // Z
    {'Z', 0, 0, 1, 1}, /* Jump unless zero to address specified  */
    {'O', 0, 0, 1, 5}, /* output to midi; channel, octave, note, velocity, length */
    {'<', 0, 0, 1, 1}, /* redirect to left n cols */
    {'>', 0, 0, 1, 1}, /* redirect to right n cols */
    {'$', 0, 0, 1, 1}, /* duplicate bang value with optional offset */
    {0, 0, 0, 0, 0},
};

/* TODO: optimize this with generated tables that use 127 value table */
arg_t *find_arg(char c)
{
    int i;
    for (i = 0; ARGS[i].cmd; i++)
    {
        if (c == ARGS[i].cmd)
        {
            return &ARGS[i];
        }
    }
    return 0;
}

int can_bang(ecl_t *ecl, int x, int req)
{
    int i;
    for (i = 1; i <= req; i++)
    {
        if (ecl_get_state(ecl, x - i) != STATE_NUM)
        {
            return 0;
        }
    }
    return 1;
}

static void op_teleport_read(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int v = (arg == '?') ? 0 : char2int(arg); /* prevent ? args */
    ecl->channels[v] = char2int(bang);
    printf("setting var(%d) = %d\n", v, ecl->channels[v]);
}

static void do_teleport(ecl_t *ecl)
{
    int x;
    int arg;
    char v;

    for (x = 0; x < ecl->memsz; x++)
    {
        v = ecl->mem[x];
        if (v == 'T')
        {
            arg = char2int(ecl_get(ecl, x + 1));
            if (ecl->channels[arg] > 0)
            {
                ecl_set(ecl, x + 2, int2char(ecl->channels[arg]));
                ecl_set_state(ecl, x + 2, STATE_NUM);
            }
        }
    }
    for (x = 0; x < BASE36; x++)
    {
        ecl->channels[x] = 0;
    }
}

/* Write bang to an address at X,Y (X*WIDTH+Y) offset from v */
// static void op_write(ecl_t *ecl, int v)
// {
//     char xarg, yarg, bang;
//     int x, y;

//     xarg = ecl_get(ecl, v + 1);
//     yarg = ecl_get(ecl, v + 2);
//     bang = ecl_get(ecl, v - 1);
//     x = char2int((xarg == '?') ? bang : xarg);
//     y = char2int((yarg == '?') ? bang : yarg);
//     printf("x %d y %d\n", x, y);
//     v += (x * ecl->height) + y + 1;
//     ecl_set(ecl, v, bang);
//     ecl_set_state(ecl, v, STATE_NUM);
// }

static void op_prob(ecl_t *ecl, int x)
{
    char arg, bang;
    int v, pass = 0;

    arg = ecl_get(ecl, x + 1);
    bang = ecl_get(ecl, x - 1);
    v = char2int((arg == '?') ? bang : arg);
    if (v > 0) /* value of zero does not pass */
    {
        if (v == BASE36 - 1) /* max value */
        {
            pass = 1;
        }
        else if (rng_double(ecl->rng) < (((double)v) / (double)BASE36))
        {
            pass = 1;
        }
    } /* else block bang, default with arg of 0 or empty */
    if (pass)
    {
        x += 2;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

static void op_inc(ecl_t *ecl, int x)
{
    int v = 1;
    int bang = char2int(ecl_get(ecl, x - 1));
    char arg = ecl_get(ecl, x + 1);

    if (arg == '?')
    {
        v = bang;
    }
    else
    {
        if (!is_empty(arg))
        {
            v = char2int(arg);
        }
    }
    v += bang;
    if (v > BASE36 - 1)
    {
        v = BASE36 - 1;
    }
    x += 2;
    ecl_set(ecl, x, int2char(v));
    ecl_set_state(ecl, x, STATE_NUM);
}

static void op_dec(ecl_t *ecl, int x)
{
    int v = 1;
    int bang = char2int(ecl_get(ecl, x - 1));
    char arg = ecl_get(ecl, x + 1);

    if (arg == '?')
    {
        v = bang;
    }
    else
    {
        if (!is_empty(arg))
        {
            v = char2int(arg);
        }
    }
    v = bang - v;
    if (v < 0) /* lower bound at zero */
    {
        v = 0;
    }
    x += 2;
    ecl_set(ecl, x, int2char(v));
    ecl_set_state(ecl, x, STATE_NUM);
}

/* To reset accumulator, just use Z8..J7.....A0... */
static void op_accumulate(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int sum = char2int(arg) + char2int(bang);

    if (char2int(bang) > 0)
    {

        /* write accumulated value to register */
        x += 1;
        ecl_set(ecl, x, int2char(sum));
        ecl_set_state(ecl, x, STATE_ARG);
        /* then to ouput */
        x += 1;
        ecl_set(ecl, x, int2char(sum));
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

static void op_if(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    
    if (arg == bang)
    {
        x += 2;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

/* Constant with no argument consumes all bang input values. Arg 
of ? will make this an identity function where bang value equals output
value */
static void op_const(ecl_t *ecl, int x)
{
    char bang, arg = ecl_get(ecl, x + 1);
    if (!is_empty(arg))
    {
        x += 2;
        if (arg == '?')
        {
            bang = ecl_get(ecl, x - 3);
            ecl_set(ecl, x, bang);
        }
        else
        { /* standard value as arg */
            ecl_set(ecl, x, arg);
        }
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

/* generate random value in [min, max] */
static void op_rand(ecl_t *ecl, int x)
{
    int v, min, max;
    char bang, min_arg, max_arg;

    min_arg = ecl_get(ecl, x + 1);
    max_arg = ecl_get(ecl, x + 2);
    bang = ecl_get(ecl, x - 1);
    min = char2int((min_arg == '?') ? bang : min_arg);
    max = char2int((max_arg == '?') ? bang : max_arg) + 1;

    if (max <= min)
    {
        max = min + 2;
    }
    printf("min %d max %d\n", min, max);
    v = rng_double(ecl->rng) * (max - min) + min;
    x += 3;
    ecl_set(ecl, x, int2char(v));
    ecl_set_state(ecl, x, STATE_NUM);
}

static void op_euclid(ecl_t *ecl, int x)
{
    int cur, pulses, steps;
    char bang = ecl_get(ecl, x - 1);
    char arg_pulses = ecl_get(ecl, x + 1);
    char arg_steps = ecl_get(ecl, x + 2);
    char arg_cur = ecl_get(ecl, x + 3);
    
    if (char2int(bang) > 0)
    {
        steps = (!is_empty(arg_steps)) ? char2int(arg_steps) : 3;
        if (steps < 1)
        {
            steps = 3;
        }
        pulses = (!is_empty(arg_pulses)) ? char2int(arg_pulses) : 1;
        if (pulses < 1)
        {
            pulses = 1;
        }
        else if (pulses > steps)
        {
            pulses = steps;
        }
        if(is_empty(arg_cur)) {
            cur = 1;   
        } else {
            cur = char2int(arg_cur) + 1;
        }

        int bucket = (pulses * (cur + steps - 1)) % steps + pulses;        
        printf("bucket %d\n", bucket);

        if (bucket >= steps)
        {
            ecl_set(ecl, x + 4, int2char(cur));
            ecl_set_state(ecl, x + 4, STATE_NUM);
            if (cur == steps) cur = 0;
        }
        ecl_set(ecl, x + 3, int2char(cur));
        ecl_set_state(ecl, x + 3, STATE_NUM);
    }
}


/* Clock generator defined by rate and length */
static void op_generate(ecl_t *ecl, int x)
{
    int v, rate, mod;
    char bang = ecl_get(ecl, x - 1);
    char arg_rate = ecl_get(ecl, x + 1);
    char arg_mod = ecl_get(ecl, x + 2);

    /* Can we run this cycle? */
    if (bang == '.' || char2int(bang) > 0)
    {

        rate = char2int((arg_rate == '?') ? bang : arg_rate);
        if (rate < 1)
        {
            rate = 8;
        }

        if (ecl->clock % rate == 0)
        {
            mod = char2int((arg_mod == '?') ? bang : arg_mod);
            if (mod < 1)
            {
                mod = 1;
            }
            v = ((ecl->clock + 1) / rate) % mod;
            ecl_set(ecl, x + 3, int2char(v + 1));
            ecl_set_state(ecl, x + 3, STATE_NUM);
        }
    }
    /* zero out any bang values iff more than one */
    if (ecl_get_state(ecl, x - 1) == STATE_NUM && ecl_get_state(ecl, x - 2) == STATE_NUM)
    {
        // printf("--- %d and %d\n",
        //        (abs(x - 1) % ecl->memsz), (abs(x - 2) % ecl->memsz));
        ecl_set(ecl, x - 1, '.');
        ecl_set_state(ecl, x - 1, STATE_EMPTY);
    }
}


/* Sequence controller; S <length> <sequence data>. If length is zero or empty, any bang 
is consumed with no action. Any bang on length > 0 will wrap around to a valid index value. 
An empty sequence will result in no action. 
Range of bang is 1-len(sequence). 
Bangs must be greater than zero.  */
static void op_seq(ecl_t *ecl, int x)
{
    char bang = char2int(ecl_get(ecl, x - 1));
    char len = char2int(ecl_get(ecl, x + 1));
    int src, tgt;

    if (len > 0 && bang > 0)
    {
        bang = (bang - 1) % len + 1; /* index into array of len size; force into 1-n */
        tgt = x + 2 + len;           /* output location */
        src = ecl_get(ecl, x + 1 + bang);
        if (src != '.')
        {
            ecl_set(ecl, tgt, src);
            ecl_set_state(ecl, tgt, STATE_NUM);
        }
    }
}

/* Jump bang value a specified number of cells; zero defaults to one. */
static void op_jump(ecl_t *ecl, int x)
{
    int v;
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    if (arg == '?')
    {
        v = x + char2int(bang) + 1;
    }
    else
    {
        if (is_empty(arg))
        { /* Just a naked J */
            v = x + 2;
        }
        else
        {
            v = char2int(arg);
            if (v < 1)
            {
                v = 1;
            }
            v += x + 1;
        }
    }
    ecl_set(ecl, v, bang);
    ecl_set_state(ecl, v, STATE_NUM);
}

static void op_var(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int v;

    if (!is_empty(arg))
    {
        v = char2int(arg);
        ecl->vars[v] = bang;
        x += 2;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_ARG);
        x += 1;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

static void op_query(ecl_t *ecl, int x)
{
    char arg = ecl_get(ecl, x + 1);
    char var;

    if (!is_empty(arg))
    {
        x += 2;
        var = ecl->vars[char2int(arg)];
        if (!is_empty(var))
        {
            ecl_set(ecl, x, var);
            ecl_set_state(ecl, x, STATE_NUM);
        }
    }
}

static void op_right(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int addr;

    if (char2int(bang) > 0)
    {
        addr = (arg == '?') ? char2int(bang) : (is_empty(arg) ? 1 : char2int(arg));
        /* computer address of where to write */
        addr = (addr == 0) ? 2 : (ecl->height * addr);
        addr += x;
        if (addr < ecl->memsz)
        {
            ecl_set(ecl, addr, bang);
            ecl_set_state(ecl, addr, STATE_NUM);
        }
    }
}

static void op_left(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int addr;

    if (char2int(bang) > 0)
    {
        addr = (arg == '?') ? char2int(bang) : (is_empty(arg) ? 1 : char2int(arg));
        /* computer address of where to write */
        addr = (addr == 0) ? 2 : (ecl->height * addr);
        addr = x - addr;

        if (addr > 0)
        {
            ecl_set(ecl, addr, bang);
            ecl_set_state(ecl, addr, STATE_NEW);
        }
    }
}

static void op_dup(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int offset;

    if (char2int(bang) > 0)
    {
        x += 2;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_NUM);
        offset = (arg == '?') ? char2int(bang) : (is_empty(arg) ? 1 : char2int(arg));
        x += (offset < 1) ? 1 : offset;
        ecl_set(ecl, x, bang);
        ecl_set_state(ecl, x, STATE_NUM);
    }
}

static void op_mod(ecl_t *ecl, int x)
{
    char bang = ecl_get(ecl, x - 1);
    char arg = ecl_get(ecl, x + 1);
    int v;

    if (!is_empty(arg))
    {
        v = char2int(bang) % char2int(arg);
        /* TODO: bounds check values */
        ecl_set(ecl, x+2, int2char(v));
        ecl_set_state(ecl, x+2, STATE_NUM);
    }
}

// Output args: channel, note, octave, velocity, length
static void op_output(ecl_t *ecl, int x)
{
    int i;
    char bang = ecl_get(ecl, x - 1);
    char arg;
    int vals[5];

    vals[0] = 1; /* def channel */
    vals[1] = 1; /* def note */
    vals[2] = 3; /* def octave */
    vals[3] = 1; /* def velocity */
    vals[4] = 1; /* def len */

    for (i = 0; i < 5; i++)
    {
        arg = ecl_get(ecl, x + i + 1);
        vals[i] = char2int((arg == '?') ? bang : arg);
    }
    /* Now bound values */
    if (vals[0] > 15)
    {
        vals[0] = 0;
    }
    if (ecl->output_fn)
    {
        ecl->output_fn(vals[0], vals[1], vals[2], vals[3], vals[4], ecl->output_ctx);
    }
}

static void call_cmd(ecl_t *ecl, int x)
{
    char v = ecl->mem[x];
    //printf("Calling %c from address %d\n", v, x);
    switch (v)
    {
    case 'A':
        op_accumulate(ecl, x);
        break;
    case 'C':
        op_const(ecl, x);
        break;
    case 'D':
        op_dec(ecl, x);
        break;
    case 'E':
        op_euclid(ecl, x);
        break;
    case 'F':
        op_if(ecl, x);
        break;
    case 'G':
        op_generate(ecl, x);
        break;
    case 'I':
        op_inc(ecl, x);
        break;
    case 'J':
        op_jump(ecl, x);
        break;
    case 'M':
        op_mod(ecl, x);
        break;
    case 'S':
        op_seq(ecl, x);
        break;
    case 'T':
        op_teleport_read(ecl, x);
        break;
    case 'O':
        op_output(ecl, x);
        break;
    case 'P':
        op_prob(ecl, x);
        break;
    case 'R':
        op_rand(ecl, x);
        break;
    case 'V':
        op_var(ecl, x);
        break;
    case 'Q':
        op_query(ecl, x);
        break;
    case 'X':
        //op_write(ecl, x);
        break;
    case '>':
        op_right(ecl, x);
        break;
    case '<':
        op_left(ecl, x);
        break;
    case '$':
        op_dup(ecl, x);
        break;
    default:
        printf("Warning: Invalid command %c", v);
        break;
    }
}

/* Evaluate a memory once; no possible error state to return */
void ecl_eval(ecl_t *ecl)
{
    int x;
    int y, args; /* arguments expected */
    char v;
    arg_t *arg;

    /* Determine current state of memory */
    for (x = 0, args = 0; x < ecl->memsz; x++)
    {
        //printf("here %d\n", x);
        if (args > 0) /* expecting args */
        {
            ecl->state[x] = STATE_ARG; /* not naked numbers or spaces */
            args--;
            continue;
        }
        v = ecl->mem[x];
        if (is_empty(v))
        { /* Most common case first */
            ecl->state[x] = STATE_EMPTY;
        }
        else if (is_number(v))
        {
            ecl->state[x] = STATE_NUM;
        }
        else if (is_command(v))
        {
            ecl->state[x] = STATE_CMD;
            arg = find_arg(v);
            if (arg)
            {
                if (arg->varargs)
                {
                    /* reusing v variable here */
                    v = ecl_get(ecl, x + 1);
                    args = (v == '.') ? 1 : (char2int(v) + 1);
                }
                else
                {
                    args = arg->args;
                }
                //printf("args for %c is %d\n", v, args);
            }
            else
            {
                printf("Invalid command %c at %d\n", v, x);
            }
        }
        /* Special case? */
        else
        {
            ecl->state[x] = STATE_ERR;
            printf("Invalid state at %d val %c\n", x, v);
        }
    }

    /* Second, we will evaluate from higher address to lower and exec (bang) all 
        commands that have valid triggers, and move numbers higher in memory if possible. 
        This pass will now know arguments from plain (and moveable) numbers.  */

    for (x = ecl->memsz - 1; x >= 0; x--)
    {
        if (ecl_get_state(ecl, x) == STATE_NUM)
        {
            if ((x + 1) % ecl->height == 0) /* delete number if at bottom */
            {
                ecl_set(ecl, x, '.');
                ecl_set_state(ecl, x, STATE_EMPTY);
            } /* move number if possible */
            else if (ecl_get_state(ecl, x + 1) == STATE_EMPTY)
            {
                ecl_set(ecl, x + 1, ecl_get(ecl, x));
                ecl_set_state(ecl, x + 1, STATE_NUM);
                ecl_set(ecl, x, '.');
                ecl_set_state(ecl, x, STATE_EMPTY);
            }
        }
        else if (ecl_get_state(ecl, x) == STATE_CMD)
        {
            arg = find_arg(ecl_get(ecl, x));
            if (arg)
            {
                if (can_bang(ecl, x, arg->bangs) || arg->pure)
                {
                    call_cmd(ecl, x);
                    for (y = 1; y <= arg->bangs; y++)
                    {
                        /* zero out all bang values */
                        ecl_set(ecl, x - y, '.');
                        ecl_set_state(ecl, x - y, STATE_EMPTY);
                    }
                }
            }
        }
    }
    do_teleport(ecl);
    ecl->clock++;
}

int ecl_load_buffer(ecl_t *ecl, const char *buffer,
                    int len, int offset)
{
    int i;
    if (ecl)
    {
        for (i = 0; i < len; i++)
        {
            //printf("examining %c from %d of len %d\n", buffer[i], i, len);
            if (buffer[i] == '.')
            {
                offset++;
            }
            else if (valid_char(buffer[i]))
            {
                ecl_set(ecl, offset, buffer[i]);
                offset++;
                printf("loaded %c from %d into %d\n", buffer[i], i, offset);
            }
        }
        return offset;
    }
    return 0;
}

#define MAX_LINE_SIZE 1024

int ecl_load(ecl_t *ecl, FILE *file)
{
    int len;
    int offset = 0;
    char buf[MAX_LINE_SIZE];

    if (file)
    {
        while (fgets(buf, MAX_LINE_SIZE, file))
        {
            len = ((int)strlen(buf) - 1);
            if (len > 0)
            {
                offset = ecl_load_buffer(ecl, buf, len, offset);
            }
        }
        return 1;
    }
    return 0;
}
#undef MAX_LINE_SIZE

int ecl_save(ecl_t *ecl, FILE *file)
{
    int i;
    if (ecl && file)
    {
        for (i = 0; i < ecl->memsz; i++)
        {
            if (i > 0 && i % 16 == 0)
            {
                fputc('\n', file);
            }
            fputc((int)ecl->mem[i], file);
        }
        return 1;
    }
    return 0;
}

#undef MIN
#undef MAX
#undef BASE36
