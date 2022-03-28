

#ifndef _ECL_H_
#define _ECL_H_

#include "rng.h"

#define BASE36 36

enum
{
  STATE_EMPTY = 0,
  STATE_CMD,
  STATE_NUM,
  STATE_ARG, /* These are numbers in the context of a command */
  STATE_SPECIAL,
  STATE_NEW,
  STATE_ERR
};

// typedef struct ecl_t
// {
//   int ip,
//       clock,
//       memsz;
//   char *mem;
//   int *state;
//   int *visual; /* visual state */
//   int *clocks;
//   char vars[BASE36]; /* teleport variable storage */
//   rng_t *rng;

//   //void (*output_fn)(int type, int command, void* ctx);
// } ecl_t;

typedef struct ecl_t
{
  int clock,
      memsz;
  char vars[BASE36];     /* variable storage */
  char channels[BASE36]; /* teleport storage */
  char *mem;
  int width, height;
  int *state;
  rng_t *rng;
  void (*output_fn)(int channel, int note, int octave, int velocity, int length, void *ctx); /* midi output fn */
  void *output_ctx;
} ecl_t;

/* Create an ECL memory; size is defined by width (x) and height (y); stored in linear array */
ecl_t *ecl_new(int x, int y, unsigned long seed);

void ecl_set_output(ecl_t *ecl,
                    void (*output_fn)(int channel, int note, int octave, int velocity, int length, void *ctx),
                    void *ctx);

/* TODO: rename */
int valid_char(char c);

/* Destroy an ECL memory and associated resources */
void ecl_free(ecl_t *ecl);

/* Load a saved memory into an ECL structure; returns true on success */
int ecl_load(ecl_t *ecl, FILE *file);

/* Load a buffer into an ECL structure; returns non-zero value on success. */
int ecl_load_buffer(ecl_t *ecl, const char *buffer, int len, int offset);

/* Save the ECL state to a file */
int ecl_save(ecl_t *ecl, FILE *file);

/* Dump memory to stdout */
void ecl_dump(ecl_t *ecl);

/* Evaluate a memory once; returns non-zero value on success. */
/* TODO: cannot really fail, change return to void? */
void ecl_eval(ecl_t *ecl);

/* Get the state at memory position x */
int ecl_get_state(ecl_t *ecl, int x);

/* Get the visual state at memory position x */
int ecl_get_visual(ecl_t *ecl, int x);

/* Get the value at memory position x */
char ecl_get(ecl_t *ecl, int x);

/* Set the value at memory position x with val */
void ecl_set(ecl_t *ecl, int x, char val);

/* Reset internal state */
void ecl_reset(ecl_t *ecl);

/* need to add:
 Type for each cell?
 Output handlers? (midi, udp, etc)
 seed for random number generator 
 */

#endif
