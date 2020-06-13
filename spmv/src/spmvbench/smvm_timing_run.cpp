/**
 * @file smvm_timing_run.c
 * @author mfh
 * @date 26 Aug 2005
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "smvm_timing_run.h"
#include "smvm_timing_results.h"
/*#include "smvm_get_output_file.h"*/
#include "smvm_verify_result.h"
#include "timing.h"

#include <smvm_util.h>
#include <smvm_malloc.h>
#include "block_smvm_code.h"

#ifdef __cplusplus
}
#endif

#include "lime.h"

#define __WITH_DEBUG2( OP )  do {if(b_dbg) { OP; }} while(0)

extern unsigned long long tsetup, treorg, toper, tcache;

#if defined(USE_ACC)
#include "IndexArray.hpp"
typedef int index_t; // int used in bcsr_matrix_t
extern IndexArray<index_t> dre; // Data Reorganization Engine
#endif


/************************************************************************/
double 
flops (const int r, const int c, const int nnzb)
{
  return 2.0 * (double) (r * c * nnzb);
}


/************************************************************************/
long
loads (const int r, const int c, const int nnzb, const int num_block_rows)
{
  /*
   * Loads due to the floating-point numbers only.
   * Load c of source vector, r of destination vector, and r*c of matrix. 
   */
  long flpts_only = sizeof(double) * (c + r + r * c);
  /*
   * Loads due to the col_idx[] array.
   * Each entry is read once for each nonzero block.  
   */
  long col_idx_only = sizeof(int) * nnzb;
  /*
   * Loads due to the row_start[] array.
   * If the compiler is clever, each element need only be read once.
   */
  long row_start_only = sizeof(int) * (num_block_rows + 1);

  return flpts_only + col_idx_only + row_start_only;
}


/************************************************************************/
long
stores (const int r, const int c, const int nnzb)
{
  /* 
   * For each nonzero block, store r values in the destination vector.
   */
  return sizeof(double) * r * nnzb;
}



/************************************************************************/
void
smvm_timing_run_with_results (struct SMVM_timing_results* p_results,
                              const int m, const int n, const int r, 
                              const int c, const int row_start[], 
                              const int col_idx[], const double values[], 
                              const int nnzb, const double src[], 
                              double dest[], const int num_trials, 
                              const int b_warn, const int b_dbg)
{
  const int b_dryrun = (num_trials <= 0) ? 1 : 0; 
  const int num_block_rows = m/r;  /* assumes r divides m evenly */

  /* Temporary variables in which to store results of SMVM runs. */
  double *timings;
  double t_median, t_min, t_max;
  double mflops;
  long num_loads, num_stores;

  /* Will be set to the appropriate SpMV function for the register blocking
   * scheme we use.  Recall that there is a matrix of function pointers, each
   * of which refers to a different register blocking scheme and whether there
   * are multiple right hand sides.  We only have one right hand side in these
   * experiments, for now at least.
   */
  SMVM_FP f;

  /*==========================================================================*/
  /* BEGIN CODE                                                               */
  /*==========================================================================*/
  __WITH_DEBUG2( fprintf (stderr, ">>> smvm_timing_run_with_results:\n") );

  /* Initialize whatever timer we happen to be using. */
  init_timer();

  if (b_dryrun)
    {
      __WITH_DEBUG2( fprintf(stderr, "Dry run: matrix created but no SpMV performed\n") );
    }
  else /* NOT b_dryrun */
    {
      __WITH_DEBUG2( fprintf (stderr, "Actual run (num_trials = %d)\n", num_trials) );
      __WITH_DEBUG2( fprintf (stderr, "\tAllocating timings array\n") );

      timings = (double *) smvm_calloc (num_trials, sizeof (double));

      __WITH_DEBUG2( fprintf (stderr, "\tGetting routine for register-blocked SpMV\n") );

      /*
       * Get the routine for register-blocked SpMV (r,c) with a single right
       * hand side.  This works with Eun-Jin's version of block CSR format
       * (INCOMPATIBLE with that of the Sparse BLAS).
       *
       * Here is a typical function for register-blocked SpMV (here, r,c = 2,2):
       *
       * void bsmvm_2x2_1 (int start_row, int end_row, int bm,
       *                   int *row_start, int *col_idx, double *value,
       *                   double *src, double *dest)
       *
       * "bm" doesn't actually do anything in _this_ particular function
       * (guessing that bm only serves a purpose for multiple right-hand sides
       * (which we aren't using)).  So it can be set to 0 or whatever.  Here
       * is a typical use of the function pointer:
       *
       * f(0, num_block_rows, 0, row_start, col_idx, values, x, yy0);
       *
       * The r and c parameters have already been range-checked.
       */
      f = bsmvm_routines[r-1][c-1][0];
  
      /*
       * We now define the block of code to be measured: register-blocked
       * SpMV.  f is the function pointer to that routine.
       */
#define SMVM_REGISTER_BLOCKED   f(0, num_block_rows-1, 0, row_start, col_idx, values, src, dest) 

      /* First we run the code block once, to warm up the cache.
       * Then we run the main timing loop with the code block. 
       */
      __WITH_DEBUG2( fprintf (stderr, "\tWarming up the cache\n") );
      SMVM_REGISTER_BLOCKED;

      __WITH_DEBUG2( fprintf (stderr, "\tTiming loop\n") );
      TIMING_LOOP( num_trials, timings, SMVM_REGISTER_BLOCKED );  
      __WITH_DEBUG2( fprintf (stderr, "\tFinished timing runs.\n") );

      smvm_get_median_min_max (timings, num_trials, &t_median, &t_min, &t_max);

      mflops = (flops (r, c, nnzb)) / 1.0E6;
      num_loads  = loads (r, c, nnzb, num_block_rows);
      num_stores = stores (r, c, nnzb);

      /* This information is usually only useful if we are in debug mode. */
      if (b_dbg)
        {
          fprintf (stderr, "\tTiming results:\n");
          fprintf (stderr, "\tMedian time:                              %.10E sec\n", t_median);
          fprintf (stderr, "\tMin time:                                 %.10E sec\n", t_min);
          fprintf (stderr, "\tMax time:                                 %.10E sec\n", t_max);
          fprintf (stderr, "\tMflops (floating-point ops, not per sec): %.10E\n", mflops);
          fprintf (stderr, "\tnum_loads:                                %ld\n", num_loads);
          fprintf (stderr, "\tnum_stores:                               %ld\n", num_stores);
        }

      smvm_save_timing_results (p_results, m, n, r, c, nnzb, num_trials, 
                                t_median, t_min, t_max, mflops, num_loads, 
                                num_stores);
     
      __WITH_DEBUG2( fprintf (stderr, "Deallocating space for timing data...") );
      smvm_free (timings);
      __WITH_DEBUG2( fprintf (stderr, "done.\n") );
    } /* the non-dryrun case */
}


/* smvm_timing_run_with_results2 is like smvm_timing_run_with_results except
   that it times every trial together and obtains the time per trial by
   dividing the total time by the number of trials. */
void
smvm_timing_run_with_results2 (struct SMVM_timing_results* p_results,
                               const int m, const int n, const int r, 
                               const int c, int row_start[],
                               int col_idx[], double values[], 
                               const int nnzb, double src[], 
                               double dest[], const int num_trials, 
                               const int b_warn, const int b_dbg,
                               const int b_verify, const double tol)
{
  const int b_dryrun = (num_trials <= 0) ? 1 : 0; 
  const int num_block_rows = m/r;  /* assumes r divides m evenly */
  tick_t start, finish;

  /* Temporary variables in which to store results of SMVM runs. */
  double secs, t_median, t_min, t_max, mflops;
  long num_loads, num_stores;

  /* Will be set to the appropriate SpMV function for the register blocking
   * scheme we use.  Recall that there is a matrix of function pointers, each
   * of which refers to a different register blocking scheme and whether there
   * are multiple right hand sides.  We only have one right hand side in these
   * experiments, for now at least.
   */
  SMVM_FP f;

  /*==========================================================================*/
  /* BEGIN CODE                                                               */
  /*==========================================================================*/
  __WITH_DEBUG2( fprintf (stderr, ">>> smvm_timing_run_with_results:\n") );

  /* Initialize whatever timer we happen to be using. */
  init_timer();

  if (b_dryrun)
    {
      __WITH_DEBUG2( fprintf(stderr, "Dry run: matrix created but no SpMV performed\n") );
    }
  else /* NOT b_dryrun */
    {
      //int i;

      __WITH_DEBUG2( fprintf (stderr, "Actual run (num_trials = %d)\n", num_trials) );

      __WITH_DEBUG2( fprintf (stderr, "\tGetting routine for register-blocked SpMV\n") );

      /*
       * Get the routine for register-blocked SpMV (r,c) with a single right
       * hand side.  This works with Eun-Jin's version of block CSR format
       * (INCOMPATIBLE with that of the Sparse BLAS).
       *
       * Here is a typical function for register-blocked SpMV (here, r,c = 2,2):
       *
       * void bsmvm_2x2_1 (int start_row, int end_row, int bm,
       *                   int *row_start, int *col_idx, double *value,
       *                   double *src, double *dest)
       *
       * "bm" doesn't actually do anything in _this_ particular function
       * (guessing that bm only serves a purpose for multiple right-hand sides
       * (which we aren't using)).  So it can be set to 0 or whatever.  Here
       * is a typical use of the function pointer:
       *
       * f(0, num_block_rows, 0, row_start, col_idx, values, x, yy0);
       *
       * The r and c parameters have already been range-checked.
       */
      f = bsmvm_routines[r-1][c-1][0];
  
      /*
       * We now define the block of code to be measured: register-blocked
       * SpMV.  f is the function pointer to that routine.
       */
#ifdef SMVM_REGISTER_BLOCKED
#undef SMVM_REGISTER_BLOCKED
#define SMVM_REGISTER_BLOCKED   f(0, num_block_rows-1, 0, row_start, col_idx, values, src, dest) 
#endif /* SMVM_REGISTER_BLOCKED */

      /* run once to warm up cache */
      //__WITH_DEBUG2( fprintf (stderr, "\tWarming up the cache\n") );
      //SMVM_REGISTER_BLOCKED;
      //{double *tmp = src; src = dest; dest = tmp;} /* pointer swap */

      __WITH_DEBUG2( fprintf (stderr, "\tTiming loop\n") );

#if 0
		fprintf(stdout, "Rows (blocks): %d\n", num_block_rows);
		fprintf(stdout, "Row index: %p\n", row_start);
		fprintf(stdout, "Col index: %p\n", col_idx);
		fprintf(stdout, "CSR values: %p\n", values);
		fprintf(stdout, "Src vector: %p\n", src);
		fprintf(stdout, "Dst vector: %p\n", dest);
#endif
		tsetup = treorg = tcache = 0;
		CLOCKS_EMULATE
		CACHE_BARRIER(dre)
		TRACE_START
		STATS_START

		// assume input data is in memory and not cached (flushed and invalidated)
		tget(start);
		/* actual timing runs */
		//for (i = 0; i < num_trials; i++) {
			SMVM_REGISTER_BLOCKED;
			//{double *tmp = src; src = dest; dest = tmp;} /* pointer swap */
		//}
		// assume output data is in memory (flushed)
		tget(finish);

		CACHE_BARRIER(dre)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		toper = tdiff(finish,start)-tsetup-treorg-tcache;
		secs = tesec(finish, start)/num_trials;
		if (secs == 0.0) secs = 1.0/TICKS_ESEC;
		fprintf(stdout, "SpMV time: %f sec\n", secs);
#if defined(USE_ACC)
		fprintf(stdout, "Setup time: %f sec\n", tsetup/(double)TICKS_ESEC);
		fprintf(stdout, "Reorg time: %f sec\n", treorg/(double)TICKS_ESEC);
#endif
		fprintf(stdout, "Oper. time: %f sec\n", toper/(double)TICKS_ESEC);
		fprintf(stdout, "Cache time: %f sec\n", tcache/(double)TICKS_ESEC);
		STATS_PRINT

      __WITH_DEBUG2( fprintf (stderr, "\tFinished timing runs.\n") );

      /* verification run: make sure the routine works */
      if (b_verify) {
        /* reverse src <-> dest if swap used */
        smvm_verify_result(m, n, r, c, row_start, col_idx, values, src,
                           dest, tol, 0, 0);
      }

      t_min = t_median = t_max = secs;
      mflops = (flops (r, c, nnzb)) / 1.0E6;
      num_loads  = loads (r, c, nnzb, num_block_rows);
      num_stores = stores (r, c, nnzb);

      /* This information is usually only useful if we are in debug mode. */
      if (b_dbg)
        {
          fprintf (stderr, "\tTiming results:\n");
          fprintf (stderr, "\tMedian time:                              %.10E sec\n", t_median);
          fprintf (stderr, "\tMin time:                                 %.10E sec\n", t_min);
          fprintf (stderr, "\tMax time:                                 %.10E sec\n", t_max);
          fprintf (stderr, "\tMflops (floating-point ops, not per sec): %.10E\n", mflops);
          fprintf (stderr, "\tnum_loads:                                %ld\n", num_loads);
          fprintf (stderr, "\tnum_stores:                               %ld\n", num_stores);
        }

      smvm_save_timing_results (p_results, m, n, r, c, nnzb, num_trials, 
                                t_median, t_min, t_max, mflops, num_loads, 
                                num_stores);
     
      __WITH_DEBUG2( fprintf (stderr, "done.\n") );
    } /* the non-dryrun case */
}


/************************************************************************/
void
smvm_timing_run (int m, int n, int r, int c, 
                 int row_start[], int col_idx[], double values[], 
                 int nnzb,
                 double src[], double dest[],
                 int num_trials, FILE* outfile, 
                 int b_warn, int b_dbg)
{
  const int b_dryrun = (num_trials <= 0) ? 1 : 0; 
  /* const char col_headers[] = "m,n,r,c,num_nonzero_blocks,num_trials,"
    "median_time(sec),min_time(sec),max_time(sec),mflops,num_loads,"
    "num_stores\n"; */
  const int num_block_rows = m/r;  /* assumes r divides m evenly */

  /*
   * For timing results.
   *
   * timings
   *   Array in which to store timings for SMVM runs.
   * t_median, t_min, t_max
   *   The median, min and max timings, respectively.
   * mflops
   *   The m(ega)flops (not per second, just floating point ops) count.
   * num_loads
   *   Total number of loads done by the SpMV.
   * num_stores
   *   Total number of stores done by the SpMV.
   */
  double *timings;
  double t_median, t_min, t_max;
  double mflops;
  long num_loads, num_stores;

  /* f
   *   Will be set to the appropriate SpMV function for the register blocking
   *   scheme we use.  Recall that there is a matrix of function pointers, each
   *   of which refers to a different register blocking scheme and whether there
   *   are multiple right hand sides.  We only have one right hand side in these
   *   experiments, for now at least.
   */
  SMVM_FP f;

  __WITH_DEBUG2( fprintf (stderr, ">>> smvm_timing_run:\n") );
  /* Initialize whatever timer we happen to be using. */
  init_timer();

  if (b_dryrun)
    {
      __WITH_DEBUG2( fprintf(stderr, "Dry run: matrix created but no SpMV performed\n") );
    }
  else /* NOT b_dryrun */
    {
      int i;

      if (b_dbg) fprintf (stderr, "Actual run (num_trials = %d)\n", num_trials);
      if (b_dbg) fprintf (stderr, "\tAllocating timings array\n");

      timings = (double *) smvm_malloc (num_trials * sizeof (double));
      for (i = 0; i < num_trials; i++)
        timings[i] = 0.0;

      if (outfile == NULL)
        {
          die_with_message ("*** ERROR: smvm_timing_run: timer file "
                            "not initialized ***", -1);
        }

      if (b_dbg) fprintf (stderr, "\tGet routine for register-blocked SpMV\n");

      /*
       * Get the routine for register-blocked SpMV (r,c) with a single right
       * hand side.  This works with Eun-Jin's version of block CSR format
       * (INCOMPATIBLE with that of the Sparse BLAS).
       *
       * Here is a typical function for register-blocked SpMV (here, r,c = 2,2):
       *
       * void bsmvm_2x2_1 (int start_row, int end_row, int bm,
       *                   int *row_start, int *col_idx, double *value,
       *                   double *src, double *dest)
       *
       * "bm" doesn't actually do anything in _this_ particular function
       * (guessing that bm only serves a purpose for multiple right-hand sides
       * (which we aren't using)).  So it can be set to 0 or whatever.  Here
       * is a typical use of the function pointer:
       *
       * f(0, num_block_rows, 0, row_start, col_idx, values, x, yy0);
       *
       * The r and c parameters have already been range-checked.
       */
      f = bsmvm_routines[r-1][c-1][0];
  
      /*
       * We now define the block of code to be measured: register-blocked
       * SpMV.  f is the function pointer to that routine.
       */
#ifdef SMVM_REGISTER_BLOCKED
#  undef SMVM_REGISTER_BLOCKED
#  define SMVM_REGISTER_BLOCKED   f(0, num_block_rows-1, 0, row_start, col_idx, values, src, dest) 
#endif /* SMVM_REGISTER_BLOCKED */

      /* First we run the code block once, to warm up the cache.
       * Then we run the main timing loop with the code block. 
       */
      __WITH_DEBUG2( fprintf (stderr, "\tWarming up the cache\n") );
      SMVM_REGISTER_BLOCKED;

      __WITH_DEBUG2( fprintf (stderr, "\tTiming loop\n") );
      TIMING_LOOP( num_trials, timings, SMVM_REGISTER_BLOCKED );  

      __WITH_DEBUG2( fprintf (stderr, "\tFinished timing runs.\n") );

      smvm_get_median_min_max (timings, num_trials, &t_median, &t_min, &t_max);

      mflops = (flops (r, c, nnzb)) / 1.0E6;
      num_loads  = loads (r, c, nnzb, num_block_rows);
      num_stores = stores (r, c, nnzb);

      /* This information is usually only useful if we are in debug mode. */
      if (b_dbg)
      {
        fprintf (stdout, "\tMedian time:                              %.10E sec\n", t_median);
        fprintf (stdout, "\tMin time:                                 %.10E sec\n", t_min);
        fprintf (stdout, "\tMax time:                                 %.10E sec\n", t_max);
        fprintf (stdout, "\tMflops (floating-point ops, not per sec): %.10E\n", mflops);
        fprintf (stdout, "\tnum_loads:                                %ld\n", num_loads);
        fprintf (stdout, "\tnum_stores:                               %ld\n", num_stores);
      }

      /*
       * Output results to timer data file.
       *
       * FIXME:  For output to the data file, clock resolution is assumed to be
       *         no better than 10^{-10} s.  This is somewhat reasonable but 
       *         ideally we should account for any possible clock resolution.
       */
      fprintf (outfile,
               "%d,%d,%d,%d,%d,%d,"
               "%.10E,%.10E,%.10E,%.10E,"
               "%ld,%ld\n",
               m, n, r, c, nnzb, num_trials,
               t_median, t_min, t_max, mflops, 
               num_loads, num_stores);
      
      /* sleep(1); // Pause to let the file write proceed. */

      __WITH_DEBUG2( fprintf (stderr, "Deallocating space for timing data...") );
      smvm_free (timings);
      __WITH_DEBUG2( fprintf (stderr, "done.\n") );
    } /* the non-dryrun case */
}
