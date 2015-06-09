#ifndef _smvm_util_h
#define _smvm_util_h
/****************************************************************************
 * @file smvm_util.h
 * @author Mark Hoemmen
 * @date 23 Feb 2006
 * 
 * Declares some utility functions.
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
 ****************************************************************************/
#include <stdlib.h>  /* exit */
#include <stdio.h>   /* fprintf */


extern int SPMV_DEBUG_LEVEL;


/** 
 * Reads the value of SPMV_DEBUG_LEVEL from the environment and sets the
 * SPMV_DEBUG_LEVEL variable accordingly.  If the debug level is > 0,
 * then WITH_DEBUG is operative.  If the debug level is > 1, then
 * WITH_DEBUG2 is operative.
 */
void
smvm_set_debug_level_from_environment ();

/**
 * Returns the debug level.
 */
int
smvm_debug_level ();

/**
 * Sets the debug level.
 */
void 
smvm_set_debug_level (const int level);


/**
 * Special wrapper for exit(int) that automatically calls dmalloc_shutdown 
 * if necessary.
 */
void
smvm_exit (const int errcode);



#define WITH_DEBUG( OP )  do { if (smvm_debug_level() > 0) { OP; } } while(0)

#define WITH_DEBUG2( OP )  do { if (smvm_debug_level() > 1) { OP; } } while(0)

#define WITH_DEBUG3( OP )  do { if (smvm_debug_level() > 2) { OP; } } while(0)

#define WITH_DEBUG4( OP )  do { if (smvm_debug_level() > 3) { OP; } } while(0)

/* mfh 05 Apr 2005: Although these macros don't seem to violate ANSI C 
 * standards, I want to make sure to eliminate them before trying to 
 * compile on non-GNU platforms. */
/*
#define dbgfnstart( fnname )  do { \
  if (smvm_debug_level() > 1) { \
    fprintf(stderr, "=== %s ===\n", fnname); \
  } \
} while(0)

#define dbgfnend( fnname )  do { \
  if (smvm_debug_level() > 1) { \
    fprintf(stderr, "=== Done with %s ===\n", fnname); \
  } \
} while(0)
*/

/** 
 * If the debug level is sufficiently high, does fprintf(stderr,args).
 * This macro uses GNU-specific features of the C preprocessor, in particular
 * the indefinite-length macro argument list feature.
 */
/*
#define dbgprint( args... )  do { \
  if (debug_level() > 1) { \
    fprintf(stderr, args); \
  } \
} while(0)
*/


#ifdef WARN
#  define WITH_WARN( OP )   do { OP; } while(0)
#else
#  define WITH_WARN( OP )  
#endif /* WARN */




#ifndef MIN
/**
 * The standard macro for finding the minimum of two things. 
 *
 * \bug GCC supports the "typeof" operator that lets us define
 * MIN in a typesafe way and only evaluate the inputs once.  Figure
 * out how to play with the #defines so that "typeof" is used when
 * we are using GCC.
 */
#  define MIN(a,b)  (((a)<(b))? (a) : (b))
#endif /* NOT MIN */

#ifndef MAX
/**
 * The standard macro for finding the maximum of two things. 
 *
 * \bug GCC supports the "typeof" operator that lets us define
 * MIN in a typesafe way and only evaluate the inputs once.  Figure
 * out how to play with the #defines so that "typeof" is used when
 * we are using GCC.
 */
#  define MAX(a,b)  (((a)>(b))? (a) : (b))
#endif /* NOT MAX */


/**
 * A comparison function to pass into qsort; compares two doubles.
 *
 * @bug May not handle the NaN case; I need to check that (mfh).
 *
 * @return -1 if *d1 < *d2, 1 if >, 0 if ==.
 */
int 
smvm_compare_double (const void *d1, const void *d2);


/**
 * Finds the median, min and max of the given array.  
 * Sorts the array as a side effect.  Results undefined if n < 1.
 *
 * @param a [IN,OUT]     Array of doubles with n elements.
 * @param n [IN]         Number of elements in a[].
 * @param p_median [OUT] Median of the given array is written here.
 * @param p_min [OUT]    Minimum of the given array is written here.
 * @param p_max [OUT]    Maximum of the given array is written here.
 */
void 
smvm_get_median_min_max (double a[], int n, double *p_median, double *p_min, double *p_max);

/**
 * Returns the median of a given array of doubles.  If the 
 * array is empty, returns 0.0 just to do something.
 *
 * @param values     array of doubles
 * @param n          length of values[] array
 */
double 
smvm_median (double* values, int n);

/*
 * Initialize x[] (of length n) with random doubles in [-1,1].
 * No memory allocation here!
 */
void 
smvm_init_vector_rand (int n, double* x);

/*
 * Initialize x[] (of length n), setting each value to val.
 * No memory allocation here!
 */
void 
smvm_init_vector_val (int n, double* x, double val);


/*
 * Prints msg to stderr (appending an endline), along with the current file name
 * and line number, and calls `exit' with the given exitval.  If msg is NULL,
 * prints nothing.  When calling this macro, append it with a semicolon.  I know
 * macros are icky, but the only way to get the file name and line number in it
 * is to use a macro.
 */
#define die_with_message( msg, exitval )   do { \
  fprintf (stderr, "%s at %s line %d\n", msg, __FILE__, __LINE__);  \
  smvm_exit (exitval); } while(0)


/**
 * Prints the current file name and line number to stderr, and calls `exit' 
 * with the given exitval.  When calling this macro, append it with a 
 * semicolon.
 */
#define die( exitval )   do { \
  fprintf (stderr, "Aborting in %s at line %d\n", __FILE__, __LINE__); \
  smvm_exit (exitval); } while(0)

/**
 * Equivalent to fprintf(stderr, args).  Uses a special feature of the GNU C
 * preprocessor, namely macros with variable numbers of arguments.
 */
/*
#define ERRMSG( args... )  do {fprintf(stderr, args);} while(0)
*/

/**
 * Returns the number of bits in the binary representation of x.
 */
int
smvm_num_bits (const int x);


/**
 * @brief Positive integer product with overflow check.
 *
 * Positive integer product with overflow check.  If x*y overflows, returns 0,
 * without changing *result.  Otherwise returns nonzero, and sets *result = x*y.  
 *
 * @param x        [IN] positive integer
 * @param y        [IN] positive integer
 * @param result   [OUT] the product of x and y, if no overflow.
 *
 * @return         Zero if overflow, otherwise nonzero.
 */
int 
smvm_checked_positive_product (int* result, const int x, const int y);

/**
 * Separates a complete pathname for a file into a parent directory, a 
 * namestem (with the .* extension removed) and an extension.  If there is no 
 * extension, *extn==NULL.  The returned char* are copies, allocated using 
 * malloc.
 *
 * @return Nonzero if error, else zero.
 */
void
split_pathname (char** parentdir, char** namestem, char** extn, const char* const path);

/**
 * Returns nonzero if path is a directory, else returns zero. 
 */
int
directory_p (const char* const path);

/**
 * Returns nonzero if path is a regular file, else returns zero.
 */
int
regular_file_p (const char* const path);


#endif /* #ifndef _smvm_util_h */
