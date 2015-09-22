/**
 * @file timing.c
 * @author mfh
 * @date 02 Oct 2005
 *
 * Implementation of the timing interface.  Compile-time options for numerous
 * timers; not all of them may be available on your system.  FIXME:  Some of 
 * the options that should work on a POSIX system might be missing the 
 * #include(s) they need.  However, we restrict config.h to options that are
 * known to build and run.
 *****************************************************************************/

#include "timing.h"
#include <stdio.h>
#include <stdlib.h> /* exit */

/*
 * Prints msg to stderr (appending an endline), along with the current file name
 * and line number, and calls `exit' with the given exitval.  If msg is NULL,
 * prints nothing.  When calling this macro, append it with a semicolon.  I know
 * macros are icky, but the only way to get the file name and line number in it
 * is to use a macro.
 */
#define die_with_message( msg, exitval )   { \
  if (msg != NULL) \
    fprintf (stderr, "%s at %s line %d\n", msg, __FILE__, __LINE__); \
  exit (exitval); \
}

/*
 * Prints the current file name and line number to stderr, and calls `exit' with
 * the given exitval.  When calling this macro, append it with a semicolon.
 */
#define die( exitval )   { \
  fprintf (stderr, "Aborting in %s at line %d\n", __FILE__, __LINE__); \
  exit (exitval); \
}


double 
timer_resolution()
{
  const int num_trials = 100;
  double trials[100];
  double tv, tv2;
  int i;
  double diff, mindiff;

  /* max_trials_on_overflow:  
   *   Max # times we should repeat test, if elapsed time overflows each time.
   * repeat_count:
   *   How many times we repeat the test.
   */
  int max_trials_on_overflow = 10;
  int repeat_count = 0;
  
  for (i = 0; i < num_trials; i++)
    {

      /* Note: We have to check for the rare case in which the elapsed time
       * overflows and goes back to zero within this loop.  We can only tell
       * this if the difference of the two time values is negative.  It can only
       * be positive if the maximum elapsed time is very small, or if the
       * processor is extremely slow, both of which are unlikely.
       */
     
      tv = get_seconds();
      while ((tv2 = get_seconds()) == tv); /* Loop until there is a change. */
      diff = tv2 - tv;
      
      repeat_count = 0;
      while (diff < 0.0 && repeat_count < max_trials_on_overflow)
        {
#ifdef WARN
          fprintf (stderr, "Warning in determination of clock resolution:  "
		   "Elapsed time overflowed!  Result %f is negative.\n"
		   "Retrying loop for %d-th time", diff, repeat_count);
#endif /* WARN */
          tv = get_seconds();
          while ((tv2 = get_seconds()) == tv);  /* Loop until there is a change.*/
          diff = tv2 - tv;
          repeat_count++;
        }
      if (diff < 0.0)
        {
          /* Didn't manage to avoid elapsed time overflows.  This is an
	     abort-worthy error.
	   */
	  die_with_message ("*** ERROR: timer_resolution: Time calculation "
			    "overflowed several times in sequence!!! ***\n", -1);
        }
      
      trials[i] = diff;
    }

  /* Find the min of all the trials.  We know that each trial's result is positive. */

  mindiff = trials[0];
  for (i = 1; i < num_trials; i++)
    mindiff = (trials[i] < mindiff) ? trials[i] : mindiff;

  /* Sanity check:  Timer resolution had better be positive!!! */
  if (mindiff <= 0.0)
    {
      fprintf (stderr, "*** ERRORR: timer_resolution: minimum difference mindiff "
	       "= %f is negative! ***\n", mindiff);
      die (-1);
    }

  return mindiff;
}


/*---------------------------------------------------------------------------*/
#ifdef POSIX_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_IN_SYS_TIME_H
#  include <sys/time.h>
#else
#  ifdef GETTIMEOFDAY_IN_TIME_H
#    include <time.h>
#  else
#    error "gettimeofday() apparently neither in <sys/time.h> nor <time.h> !!!"
#  endif /* GETTIMEOFDAY_IN_TIME_H */
#endif /* GETTIMEOFDAY_IN_SYS_TIME_H */
/*
 * MFH 2003 Nov 15
 *
 * Uses the POSIX function "gettimeofday(...)" to achieve near-microsecond
 * (10^{-6}) clock resolution.  For the POSIX standard, please refer to:  
 * The Open Group Base Specifications Issue 6 (IEEE Std 1003.1, 2003 Edition):
 * http://www.opengroup.org/onlinepubs/007904975/toc.htm
 *
 * gettimeofday is the method used by John D. McCalpin's STREAM memory 
 * bandwidth benchmark, for example.  ANSI C's clock() function has by 
 * comparison a very low resolution (0.01s on my Pentium M (Celeron) 1300 
 * MHz laptop, for example).  The POSIX standard does define a nanosecond
 * resolution clock in <time.h> (not <sys/time.h>).
 */
void 
init_timer(void) 
{
  int i;
  double s = 0.0;
  double t = 0.0;
  double x = 1.0;
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "POSIX gettimeofday used (near-microsecond resolution)\n");
#endif
  /*
   * MFH 2004 Jan 24: Sometimes gettimeofday doesn't work right on the
   * first call.  So I decided to put a "warmup run" here.  We time
   * some convergent operation which the compiler won't optimize away.
   */
  t = get_seconds();
  for (i = 0; i < 100000; i++)
    {
      s = s + x / 2;
      x = x / 2.0;
    }
  t = get_seconds() - t;
}


double 
get_seconds()
{
  struct timeval tp;
  int i;

  /* The POSIX standard (2003 edition) says that if the 2nd argument of 
   * gettimeofday is not NULL, the behavior of the function is unspecified.
   */
  i = gettimeofday(&tp,NULL);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}


/*---------------------------------------------------------------------------*/
#elif defined(POSIX_TIMER)
#ifdef CLOCK_GETTIME_IN_TIME_H
#  include <time.h>
#else
#  error "timing.c:  clock_gettime() is apparently not in <time.h> !!!"
#endif /* CLOCK_GETTIME_IN_TIME_H */
/*
 * Uses the POSIX real-time clock with "clock_gettime(...)".  The "TMR" (timer)
 * option is optional in POSIX, so clock_gettime may not exist.  If it does, the
 * resolution is accessible via the following function:
 *
 * int clock_getres(clockid_t clock_id, struct timespec *res);
 *
 * timespec stores time in nanoseconds, although the actual resolution may be
 * much coarser than 1 ns.  For the POSIX standard, please refer to: The Open
 * Group Base Specifications Issue 6 (IEEE Std 1003.1, 2003 Edition):
 * http://www.opengroup.org/onlinepubs/007904975/toc.htm
 */
void 
init_timer(void)
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "POSIX clock_gettime used.\n");
#endif
}
double get_seconds(){
  struct timespec ts;
  /*
   * The monotonic clock is preferred over the realtime clock (monotonicity 
   * is good), but according to the POSIX standard, the monotonic clock is
   * optional.  So we test whether it is defined.
   */
#ifdef CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif /* CLOCK_MONOTONIC */
  return (double) ts.tv_sec + ((double) ts.tv_nsec / (double) 1.E9);
}


/*---------------------------------------------------------------------------*/
#elif defined(ITIMER)

#define MAX_ETIME 86400   /* 24 hour timer period */
struct itimerval first_u; /* user time */

/* init the timer */
void init_timer(void) 
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, 
           "getitimer used (claims near-microsecond resolution).\n");
#endif
  first_u.it_interval.tv_sec = 0;
  first_u.it_interval.tv_usec = 0;
  first_u.it_value.tv_sec = MAX_ETIME;
  first_u.it_value.tv_usec = 0;
  setitimer(ITIMER_PROF, &first_u, NULL);
}
/* Returns elapsed user seconds since call to init_etime */
double get_seconds(void) 
{
  struct itimerval curr;

  getitimer(ITIMER_PROF, &curr);
  return (double) ((first_u.it_value.tv_sec - curr.it_value.tv_sec) +
                   (first_u.it_value.tv_usec - curr.it_value.tv_usec)*1e-6);
}


/*---------------------------------------------------------------------------*/
#elif defined(HR_TIMER)

void init_timer(void)
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "hr_timer used.\n");
#endif
}
double get_seconds()
{
  hrtime_t t = gethrtime();

  return ((double)t)/1.E9;
}


/*---------------------------------------------------------------------------*/
#elif defined(IPM_TIMER)

#include "IPM_timer.h"

void init_timer( void )
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "IPM timer used.\n");
#endif
}

double get_seconds()
{
  return IPM_timer_get_time();
}


/*---------------------------------------------------------------------------*/
#elif defined(ANSI_C_CLOCK)
/*
 * ANSI C's clock().  Resolution is on the order of 0.01s.  The value of
 * CLOCKS_PER_SEC is required to be 1 million on all XSI-conformant systems.
 * It's weird, but it's in the POSIX standard.  What that also means is that
 * we probably can't trust this value much.
 */
#ifdef CLOCK_IN_TIME_H
#  include <time.h>
#else
#  error "timing.c:  clock() apparently not found in <time.h> !!!"
#endif

void init_timer(void)
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "ANSI C standard function clock() used (resolution about 0.01s).\n");
#endif
}


double get_seconds()
{
  return ((double) clock()) / ((double)(CLOCKS_PER_SEC));
}

#else
/*# error "Please select a particular timing mechanism in the platform-specific config file."*/

void init_timer(void)
{
#ifdef REPORT_CLOCK_TYPE
  fprintf (stderr, "No clock used.\n");
#endif
}


double get_seconds()
{
  return (0.0);
}

#endif /* various timers */
