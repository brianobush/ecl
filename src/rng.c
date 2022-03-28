/* REFERENCE                                                       */
/* M. Matsumoto and T. Nishimura,                                  */
/* "Mersenne Twister: A 623-Dimensionally Equidistributed Uniform  */
/* Pseudo-Random Number Generator",                                */
/* ACM Transactions on Modeling and Computer Simulation,           */
/* Vol. 8, No. 1, January 1998, pp 3--30.                          */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "rng.h"

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

#define MT_RAND_MAX 0xffffffff

struct rng_t {
  unsigned long mt[N]; /* the array for the state vector  */
  int mti, has_saved;
  double saved;
};

rng_t* rng_new(unsigned long seed) {
  rng_t* rng = calloc(1, sizeof(rng_t));
  /* setting initial seeds to mt[N] using         */
  /* the generator Line 25 of Table 1 in          */
  /* [KNUTH 1981, The Art of Computer Programming */
  /*    Vol. 2 (2nd Ed.), pp102]                  */
  rng->mt[0]= seed & 0xffffffff;
  for (rng->mti=1; rng->mti<N; rng->mti++) {
    rng->mt[rng->mti] = (69069 * rng->mt[rng->mti-1]) & 0xffffffff;
  }
  return rng;
}

void rng_free(rng_t* rng)
{
  free(rng);
}

unsigned long rng_next(rng_t* rng)
{
  int kk;
  unsigned long y;
  static unsigned long mag01[2]={0x0, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if(rng->mti >= N) { /* generate N words at one time */

    for(kk=0;kk<N-M;kk++) {
      y = (rng->mt[kk]&UPPER_MASK)|(rng->mt[kk+1]&LOWER_MASK);
      rng->mt[kk] = rng->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    for(;kk<N-1;kk++) {
      y = (rng->mt[kk]&UPPER_MASK)|(rng->mt[kk+1]&LOWER_MASK);
      rng->mt[kk] = rng->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    y = (rng->mt[N-1]&UPPER_MASK)|(rng->mt[0]&LOWER_MASK);
    rng->mt[N-1] = rng->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

    rng->mti = 0;
  }

  y = rng->mt[rng->mti++];
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  return y;
}

/* generates a random number with 53-bit resolution. NOTE: returns
   doubles in [0,1) -- excluding zero, but including 1. */
double rng_double(rng_t* rng)
{
  /* shifts : 67108864 = 0x4000000, or 2**26, 9007199254740992 =
     0x20000000000000, or 2**53. Basically, [a] contains 27 random
     bits shifted left 26, and [b] fills in the lower 26 bits of the
     53-bit numerator.
 */
  long a = rng_next(rng) >> 5, b = rng_next(rng) >> 6;
  return (a * 67108864.0 + b) / 9007199254740992.0;
}

/* see: http://www.taygeta.com/random/gaussian.html */
static double rng_normal(rng_t* rng)
{
  double u,v,r2,rsq;

  if(rng->has_saved) {
    rng->has_saved = 0;
    return rng->saved;
  } else {
    do {
      u = 2.0 * rng_double(rng) - 1.0;
      v = 2.0 * rng_double(rng) - 1.0;
      r2 = u * u + v * v;
    } while (r2 > 1 || r2 == 0);
    rsq = sqrt(-2.0 * log(r2) / r2);
    rng->has_saved = 1;
    rng->saved = v * rsq;
    return u * rsq;
  }
}

double rng_gaussian(rng_t* rng, double mu, double sigma)
{
  return mu + rng_normal(rng) * sigma;
}

int rng_choice(rng_t* rng, int range)
{
  /* See comp.lang.c FAQ 13.16 */
  unsigned int x = (MT_RAND_MAX) / range;
  unsigned int y = x * range;
  unsigned int r;
  do {
    r = rng_next(rng);
  } while(r >= y);

  return r / x;
}

#undef N
#undef M
#undef MATRIX_A
#undef UPPER_MASK
#undef LOWER_MASK
