/**
 * @file random_number.c
 * @author Mark Hoemmen
 * @date 23 Feb 2006
 * 
 * Copyright (c) 2006, Regents of the University of California 
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the
 * following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the 
 *   distribution.
 *
 * * Neither the name of the University of California, Berkeley, nor
 *   the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior
 *   written permission.  
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************/
#include <random_number.h>
#include <smvm_malloc.h>
#include <smvm_util.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>  /* srand fn, rand fn */
#include <time.h>    /* time fn           */


/* Initializes the Mersenne Twister PRNG. */
extern void
init_genrand (unsigned long s);

/* generates a random number on [0,0xffffffff]-interval */
unsigned long 
genrand_int32 (void);

/* generates a random number on [0,0x7fffffff]-interval */
long 
genrand_int31 (void);

/* generates a random number on [0,1) with 53-bit resolution*/
double 
genrand_res53 (void);

#if 0
/**
 * CHECK:  This rounds -0.5 to 0 -- is that correct???
 */
static int
round_to_nearest_int (double t)
{
  if (t >= -0.5)
    return (int) (t + 0.5);

  return (int) (t - 0.5);
}
#endif


/***********************************************************************/
void 
smvm_init_randomizer (unsigned int seed)
{
  WITH_DEBUG2(fprintf(stderr, "=== smvm_init_randomizer ===\n"); fprintf(stderr, "seed = %x\n", seed));

  /* srand ((int) (time(0))); */
  init_genrand (seed);
  WITH_DEBUG2(fprintf(stderr, "=== Done with smvm_init_randomizer ===\n"));
}

/**
 * Returns a random integer in the range 0 .. N-1, inclusive.
 */
static int
smvm_random_integer_in_range_zero_to_n_minus_one (int N)
{
#if 0
  /* This approach doesn't address rounding error. */
  double d = genrand_res53 ();
  d = ((double) N) * d;
  return (int) d;
#endif

  int num_trials = 0;
  long attempt = 0;
  int n;

  /* WITH_DEBUG2(fprintf(stderr, "=== smvm_random_integer_in_range_zero_to_n_minus_one ===\n"); fprintf(stderr, "N = %d\n", N)); */

  assert (N > 0);

  /* 
   * See:  http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/efaq.html
   *
   * 1. Find the min int n such that N <= 2^n.  INT_MAX is 2^31 - 1, so we 
   *    don't need any special cases in the algorithm below (these are signed
   *    ints).
   * 2. Take the most significant n bits of as many integer random numbers as
   *    we need to get n bits.  Since INT_MAX is 2^31 - 1, n will be at most
   *    32, so we only need 32 bits.
   * 3. If the random number is >= N, discard it and try again.  (Just in case
   *    the random number generator is broken, I have a loop that stops trying
   *    again if we don't have success after INT_MAX trials.)
   */

  /* Step 1:  Find n. */
  if (N >= INT_MAX)
    n = 31;
  else
    {
      register int D = 1;
      n = 0;
      while (N > D)
	{
	  D = D << 1;
	  n++;
	}
      /* WITH_DEBUG2(fprintf(stderr, ">>> D = %d <<<\n", D)); */
    }

  assert (n < 32);
  assert (n >= 0);

  while (num_trials < INT_MAX)
    {
      /* Step 2:  get 32 bits of randomness */
      unsigned long random_bits = genrand_int32 ();
      /* WITH_DEBUG2(fprintf(stderr, ">>> random_bits = %x, 32-n = %d <<<\n", (unsigned int) random_bits, 32 - n)); */

      /* Right-shift random_bits by (32 - n).  This fills in zeros for the 
       * sign bit, so we know that attempt >= 0. */
      if (n == 0)
	attempt = 0; /* the only possibility */
      else
        attempt = (long) (random_bits >> (32 - n));

      /* WITH_DEBUG2(fprintf(stderr, ">>> attempt = %x <<<\n", (unsigned int) attempt)); */
      assert (attempt >= 0);

      /* Return attempt if it is in range. */
      if (attempt < N)
	{
	  /* WITH_DEBUG2(fprintf(stderr, "=== Done with smvm_random_integer_in_range_zero_to_n_minus_one ===\n")); */
	  return attempt;
	}

      num_trials++;
    }

  fprintf (stderr, "*** smvm_random_integer_in_range_zero_to_n_minus_one:  Yo"
	   "ur random number generator is probably broken, since even after I"
	   "NT_MAX trials, we were unable to get a random integer within the "
	   "range [0,%d) ***\n", N);
  exit (EXIT_FAILURE);
  return 0; /* to pacify the compiler */
}


/***********************************************************************/
int 
smvm_random_integer (int low, int high)
{
  int k = 0;
  assert (high >= low);
  k = low + smvm_random_integer_in_range_zero_to_n_minus_one (high - low + 1);

  assert (k >= low);
  assert (k <= high);
  return k;

#if 0
  /*
   * MFH 2004 Jan 15:  OK, this is too weird.  
   * double temp = 0.0; temp = (double) i / RAND_MAX;
   * causes temp = nan, but calculating (double) i / RAND_MAX again
   * after that makes everything work out.  You actually have to 
   * calculate (double) i / RAND_MAX _twice_  -- you can't just set
   * temp = 0.0.
   */
  /* rand() returns an int between 0 and RAND_MAX. */
  int i = rand ();
  /* double temp = (double) (i) / (double) (RAND_MAX); */
  /* double temp = (double) i / RAND_MAX; */
  double temp = 0.0;

  WITH_DEBUG4(fprintf(stderr, "### rand() = %d ###\n", i));

  temp = (double) i / RAND_MAX; 
  temp = (double) i / RAND_MAX; 

  /* Transform into interval [low,high], and round to nearest int.
   * This is essential -- if you just rely on the cast to int, the 
   * resulting truncation will throw off the probabilities.
   */
  return round_to_nearest_int ((high - low) * temp + low);
#endif
}


/***********************************************************************/
double
smvm_random_double (double low, double high)
{
  return low + (high - low) * genrand_res53 ();
#if 0
  /* rand() returns an int between 0 and RAND_MAX. */
  double temp = (double) (rand ()) / (double) (RAND_MAX);

  /* Transform into interval [low, high]. */
  return (high - low) * temp + low;
#endif 
}


struct random_integer_from_range_without_replacement_generator_t*
create_random_integer_from_range_without_replacement_generator (const int low, const int high)
{
  struct random_integer_from_range_without_replacement_generator_t* gen = NULL;
  int i;

  WITH_DEBUG2(fprintf(stderr, "=== create_random_integer_from_range_without_replacement_generator ===\n"));

  gen = smvm_calloc (sizeof (struct random_integer_from_range_without_replacement_generator_t), 1);
  gen->low = low;
  gen->high = high;

  /* It's legit for high < low -- that just means that there are no numbers 
   * left from which to pick. */
  if (high < low)
    gen->num_remaining = 0;
  else
    gen->num_remaining = high - low + 1;

  gen->remaining = smvm_malloc (gen->num_remaining * sizeof (int));
  for (i = 0; i < gen->num_remaining; i++)
    gen->remaining[i] = low + i;

  return gen;
}

void
destroy_random_integer_from_range_without_replacement_generator (struct random_integer_from_range_without_replacement_generator_t* gen)
{
  if (gen != NULL)
    {
      if (gen->remaining != NULL)
	smvm_free (gen->remaining);

      smvm_free (gen);
    }
}


int
return_random_integer_from_range_without_replacement (int* theint, struct random_integer_from_range_without_replacement_generator_t* gen)
{
  int p = 0;
  int i;

  if (gen->num_remaining < 1)
    return -1;

  if (smvm_debug_level() > 1)
    {
      int j;
      fprintf (stderr, "### ");
      for (j = 0; j < gen->num_remaining; j++)
	fprintf (stderr, "%d ", gen->remaining[j]);

      fprintf (stderr, "\n");
    }

  /* Pick a random position in the array of remaining elements, and return 
   * the element there. */
  p = smvm_random_integer (0, gen->num_remaining - 1);
  *theint = gen->remaining[p];

  /* Remove the selected element from the array of remaining elements, by 
   * left-shifting over all the elements after it and decrementing 
   * num_remaining. */

  for (i = p; i < gen->num_remaining - 1; i++)
    gen->remaining[i] = gen->remaining[i+1];

  gen->num_remaining = gen->num_remaining - 1;

  if (smvm_debug_level() > 1)
    {
      int j;
      fprintf (stderr, "### ");
      for (j = 0; j < gen->num_remaining; j++)
	fprintf (stderr, "%d ", gen->remaining[j]);

      fprintf (stderr, "\n");
    }

  return 0;
}
