#ifndef _timing_h
#define _timing_h
/**
 * @file timing.h
 * @author mfh
 * @since 2004 Jan 24
 * @date 2004 Oct 02
 *
 * Macros and functions for using the timer.   
 ****************************************************************************/

#include <stdio.h>
#include <math.h>

#define GETTIMEOFDAY_IN_SYS_TIME_H 1
/*#define GETTIMEOFDAY_IN_TIME_H 1*/
#define CLOCK_GETTIME_IN_TIME_H 1
#define CLOCK_IN_TIME_H 1

/**
 * Initializes the timer.  What this actually does, if anything at all, depends
 * on the particular timer and its implementation. 
 */
void 
init_timer();

/**
 * Returns a number of seconds.  Take differences to find elapsed time.
 * Resolution is implementation-dependent (see Makefile and various
 * configuration files).
 */
double 
get_seconds();

/**
 * Returns the resolution of the timer (an estimate of the smallest measurable
 * time interval) in seconds.  
 */
double 
timer_resolution();

/*
 * TIMING_LOOP
 *
 * times[] is an array of length max_iter.  The code block OP is repeated
 * max_iter times.  For each repeat i, the length of time required for that
 * iteration of OP to execute is stored in times[i].
 *
 * If the clock resolution is too coarse-grained to capture the time of OP
 * accurately, #define COARSE_CLOCK.  This makes times[i] the mean of 
 * COARSE_CLOCK_REP repetitions of OP.  COARSE_CLOCK_REP should be #define'd
 * to a value suited to the coarseness of the clock.
 *
 * TODO:  Instead of relying on COARSE_CLOCK_REP, determine clock resolution
 * automatically.
 */
#ifdef COARSE_CLOCK
  #define TIMING_LOOP( max_iter, times, OP ) { \
    int ii_; \
    for( ii_ = 0; ii_ < max_iter; ii_++ ) \
    { \
      int jj_; \
      double dt_ = get_seconds(); \
      for( jj_ = 0; jj_ < COARSE_CLOCK_REP; jj_++ ) \
      { \
        OP; \
      } \
      dt_ = (get_seconds() - dt_) / COARSE_CLOCK_REP; \
      times[ii_] = dt_; \
    } \
  }
#else
#ifdef DEBUG 
/* MFH 2004 Jan 24:  For some reason, the first run is always NaN.  On my 
 * system, this is apparently negative infinity.
 * So now we redo runs that produce "bad" results, but we don't allow
 * any single run to be redone more than 3 times.  This is an arbitrary
 * but reasonable figure. */
  #define TIMING_LOOP( max_iter, times, OP ) { \
    int redo_count = 0; \
    int max_redos  = 3; \
    int ii_; \
    for (ii_ = 0; ii_ < max_iter; ii_++) \
    { \
      double dt_ = get_seconds(); \
      { \
        fprintf (stderr, ">>> Trial %d of %d:  ", ii_ + 1, max_iter); \
        OP; \
      } \
      dt_ = get_seconds() - dt_; \
      if (dt_ >= 0) \
        { \
          times[ii_] = dt_; \
          fprintf (stderr, "%.10E seconds <<<\n", dt_); \
          redo_count = 0; \
        } \
      else \
        { \
          if (redo_count > max_redos) \
	    { \
	      fprintf (stderr, "*** ERROR: TIMING_LOOP: exceeded max redo count " \
		       "due to invalid results!!!"); \
	      fprintf (stderr, " at line %d of %s ***\n", __LINE__, __FILE__); \
              exit (-1); \
	    } \
          fprintf (stderr, "Bad timing run:  %.10E, redoing <<<\n", dt_); \
          ii_--; \
          redo_count++; \
	} \
    } \
  }
#else /* NOT DEBUG */
/* MFH 2004 Jan 24:  For some reason, the first run is always NaN.  
 * So now we redo runs that produce "bad" results, but we don't allow
 * any single run to be redone more than 3 times.  This is an arbitrary
 * but reasonable figure. */
  #define TIMING_LOOP( max_iter, times, OP ) { \
    int redo_count = 0; \
    int max_redos  = 3; \
    int ii_; \
    for (ii_ = 0; ii_ < max_iter; ii_++) \
    { \
      double dt_ = get_seconds(); \
      { \
        OP; \
      } \
      dt_ = get_seconds() - dt_; \
      if (dt_ >= 0) \
        { \
          times[ii_] = dt_; \
          redo_count = 0;   \
        } \
      else \
        { \
          if (redo_count > max_redos) \
	    { \
	      fprintf (stderr, "*** ERROR: TIMING_LOOP: exceeded max redo count " \
		       "due to invalid results!!!"); \
	      fprintf (stderr, " at line %d of %s ***\n", __LINE__, __FILE__); \
              exit (-1); \
	    } \
          fprintf (stderr, "Bad timing run:  %.10E, redoing <<<\n", dt_); \
          ii_--; \
          redo_count++; \
	} \
    } \
  }
#endif /* WARN */
#endif /* COARSE_CLOCK */


#endif /* #ifndef _timing_h */

