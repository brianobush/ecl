
#include <stdlib.h>
#include <stdio.h>


#include "rng.h"

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  rng_t* rng = rng_new(34583);
  int i;
  for(i=0; i<100; i++) {
    printf("%1.3f\n", rng_double(rng));
  }
  printf("gaussian\n");
  for(i=0; i<100; i++) {
    printf("%1.3f\n", rng_gaussian(rng, 2, 0.1));
  }
  printf("choice\n");
  for(i=0; i<100; i++) {
    printf("%d\n", rng_choice(rng, 2));
  }

  rng_free(rng);
  return 0;
}
