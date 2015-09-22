/**
 * @file smvm_util.c
 * @author mfh
 * @date 27 Jul 2005
 *
 * Implementation of functions declared in smvm_util.h.
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
 *************************************************************************/
#include <smvm_util.h>
#include <smvm_malloc.h>
#include <random_number.h>

#define HAVE_ERRNO_H 1
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#else
#  error "Sorry, you need to have the <errno.h> POSIX standard header file in order to compile smvm_util.c."
#endif
#ifdef HAVE_LIBGEN_H
#  include <libgen.h>
/*#else*/
/*#  error "Talk to Mark Hoemmen about implementing replacement routines for the POSIX standard functions dirname and basename, or talk to your OS vendor about implementing the POSIX standard properly."*/
#endif /* HAVE_LIBGEN_H */
#include <limits.h>    /* INT_MAX */
#include <stdio.h>     /* the usual output stuff */
#include <stdlib.h>    /* rand(), srand(unsigned int), qsort */
#include <string.h>
#include <time.h>      /* time_t time(time_t*) */
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>  /* for stats on files, like whether or not it's a directory */
#endif


/* should be in string.h */
//extern char* strdup (const char* s);

int SPMV_DEBUG_LEVEL = 0;


void
smvm_exit (const int errcode)
{
#ifdef DMALLOC
  extern void dmalloc_shutdown(void);

  dmalloc_shutdown ();
#endif /* DMALLOC */

  exit (errcode);
}


void
smvm_set_debug_level_from_environment ()
{
  /* getenv is a POSIX function */ 
  char *valstring = getenv ("SPMV_DEBUG_LEVEL");

  if (valstring == NULL) 
    {
      /* default */
      smvm_set_debug_level (0);
      return;
    }
  else
    {
      int value;
#ifdef HAVE_ERRNO_H
      errno = 0; /* Must always reset errno before checking it */

      /* Convert the "value" part of the "name=value" pair to an
       * int */

      value = strtol (valstring, NULL, 10);
      if (errno)
        {
          fprintf (stderr, "*** smvm_set_debug_level_from_environment:"
                   " SPMV_DEBUG_LEVEL is set to %s, which is not an "
                   "integer ***\n", valstring);
          smvm_set_debug_level (0);
          return;
        }
      else
        {
          if (value < 0)
            {
              fprintf (stderr, "*** smvm_set_debug_level_from_environment: "
                       "SPMV_DEBUG_LEVEL is set to %d, which is negative and"
                       " therefore invalid ***\n", value);
              smvm_set_debug_level (0);
              return;
            }
          else
            smvm_set_debug_level (value);
        }
#else /* No errno.h */
      char *endptr = NULL;

      value = strtol (valstring, &endptr, 10);
      /* Check out the GNU man page for strtol.   "If endptr is not NULL, 
       * strtol() stores the address of the first invalid character in 
       * *endptr.  If there were no digits at all, strtol() stores the 
       * original value of nptr in *endptr (and returns 0).  In particular, 
       * if *nptr is not '\0' but **endptr is '\0' on return, the entire 
       * string is valid."  */
      if (*valstring != '\0' && endptr != NULL && *endptr == '\0')
        {
          /* We got value successfully. */
          if (value < 0)
            {
              fprintf (stderr, "*** smvm_set_debug_level_from_environment: "
                       "SPMV_DEBUG_LEVEL is set to %d, which is negative and"
                       " therefore invalid ***\n", value);
              smvm_set_debug_level (0);
              return;
            }
          else
            smvm_set_debug_level (value);
        }
      else
        {
          fprintf (stderr, "*** smvm_set_debug_level_from_environment:"
                   " SPMV_DEBUG_LEVEL is set to %s, which is not an "
                   "integer ***\n", valstring);
          smvm_set_debug_level (0);
          return;
        }
#endif /* HAVE_ERRNO_H */
    }
}


int 
smvm_compare_double (const void *d1, const void *d2)
{
  double v_d1 = *((double *)d1);
  double v_d2 = *((double *)d2);

  if (v_d1 < v_d2) return -1;
  if (v_d1 > v_d2) return 1;
  return 0;
}


void 
smvm_get_median_min_max (double a[], int n, double *p_median, double *p_min, double *p_max)
{
  if (n < 1) return;
    
  qsort(a, n, sizeof(double), smvm_compare_double);
  *p_min = a[0];
  *p_max = a[n-1];
  *p_median = (n%2)? a[n/2] : (a[n/2-1] + a[n/2])/2;
}


void
smvm_init_vector_rand (int n, double* x)
{
  int i;
  for (i = 0; i < n; i++)
    x[i] = smvm_random_double (-1.0, 1.0);
}


void
smvm_init_vector_val (int n, double* x, double val)
{
  int i;
  for (i = 0; i < n; i++)
    x[i] = val;
}


double 
smvm_median (double* values, int n)
{
  if (n < 1)
    return 0.0;
  if (n < 2)
    return values[0];

  qsort (values, n, sizeof(double), smvm_compare_double);
  return (n%2) ? values[n/2] : (values[n/2-1] + values[n/2])/2;
}


int 
smvm_num_bits (const int x)
{
  int t = x;
  int L;
  for (L = 0; t; L++, t >>= 1)
    ; /* do nothing */

  return L;
}


int 
smvm_checked_positive_product (int* result, const int x, const int y)
{
  int lx = smvm_num_bits (x);
  int ly = smvm_num_bits (y);
  int lintmax = smvm_num_bits (INT_MAX);
  int p;

  /* x*y requires (lx + ly + 1) bits.  Since INT_MAX is $2^{lintmax + 1} - 1$,
     any positive integer requiring the same number of bits as INT_MAX does not
     overflow. */

  if (lx + ly + 1 > lintmax)
    return 0; /* overflowed */

  /* else */
  p = x * y;

  /* Sanity check to make sure that there was no overflow. */
  if (p < 0)
    return 0; /* overflowed */

  /* else */
  *result = p;
  return 1;
}


int
smvm_debug_level ()
{
  return SPMV_DEBUG_LEVEL;
}


void 
smvm_set_debug_level (const int level)
{
  SPMV_DEBUG_LEVEL = level;
}


void
split_pathname (char** parentdir, char** namestem, char** extn, const char* const path)
{
#ifdef HAVE_LIBGEN_H
  char* path_copy = strdup (path);
  char* s;
  int n, k;

  *extn = NULL; /* Will be filled in with at least the empty string */
  *parentdir = strdup (dirname (path_copy));
  smvm_free (path_copy);
  path_copy = strdup (path);
  s = strdup (basename (path_copy));
  smvm_free (path_copy);
  n = strlen (s);
  /* Extract the extension */
  for (k = n - 1; k >= 0; k--)
    {
      if (s[k] == '.')
        {
          s[k] = '\0';
          if (k < n - 1) 
            {
              /* Save the extension */
              *extn = strdup (&s[k + 1]);
            }
          break;
        }
    }
  *namestem = s;
  if (*extn == NULL)
    *extn = strdup ("");

#else
  *parentdir = *namestem = *extn = "";
  WITH_DEBUG(fprintf(stderr, "*** split_pathname: you need <libgen.h> ***\n"));
/*#  error "Sorry, you need the POSIX standard header file <libgen.h> to compile smvm_util.c."*/
#endif /* HAVE_LIBGEN_H */
}


int
directory_p (const char* const path)
{
#ifdef HAVE_SYS_STAT_H
  int saved_errno = 0;
  int status = 1;
  struct stat s;

  errno = 0;
  status = stat (path, &s);
  saved_errno = errno;

  if (status != 0)
    {
      WITH_DEBUG(fprintf(stderr, "*** directory_p: stat failed: errno = %d ***\n", saved_errno));
      return 0;
    }
  else
    {
      mode_t mode = s.st_mode;
      if (S_ISDIR (mode))
        return 1; /* is a directory */
      else
        return 0;
    }
#else
  WITH_DEBUG(fprintf(stderr, "*** directory_p: you need <stat.h> ***\n"));
  return 0;
#endif
}


int
regular_file_p (const char* const path)
{
#ifdef HAVE_SYS_STAT_H
  int saved_errno = 0;
  int status = 1;
  struct stat s;

  errno = 0;
  status = stat (path, &s);
  saved_errno = errno;

  if (status != 0)
    {
      WITH_DEBUG(fprintf(stderr, "*** regular_file_p: stat failed: errno = %d ***\n", saved_errno));
      return 0;
    }
  else
    {
      mode_t mode = s.st_mode;
      if (S_ISREG (mode))
        return 1; /* is a regular file */
      else
        return 0;
    }
#else
  WITH_DEBUG(fprintf(stderr, "*** regular_file_p: you need <stat.h> ***\n"));
  return 0;
#endif
}
