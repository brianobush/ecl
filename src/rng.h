
#ifndef _RNG_H_
#define _RNG_H_

/* The RNG itself containing current state */
typedef struct rng_t rng_t;

/* Create and initialize a RNG; a seed of zero uses current time */
rng_t* rng_new(unsigned long seed);

/* generates a random number with 53-bit resolution. NOTE: returns
   doubles in [0,1) -- excluding zero, but including 1. */
double rng_double(rng_t* rng);

/* range must be > 0; returns values in (0,range] -- including zero,
   but excluding range */
int rng_choice(rng_t* rng, int range);

/* Generate random values from a gaussian dist */
double rng_gaussian(rng_t* rng, double m, double s);

/* Free a RNG */
void rng_free(rng_t* r);

#endif /* _RNG_H_ */
