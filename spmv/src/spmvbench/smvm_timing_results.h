#ifndef _smvm_timing_results_h
#define _smvm_timing_results_h
/**
 * @file smvm_timing_results.h
 * @author mfh
 * @date Time-stamp: <2004-03-24 19:43:11 mhoemmen>
 ****************************************************************************/

/**
 * @brief Complete set of results from SMVM benchmark run.
 *
 * A complete set of results from a run (including all trials in the run) of
 * the SMVM benchmark.
 */
struct SMVM_timing_results
{
  /** Number of rows in the sparse matrix */
  int m;
  /** Number of columns in the sparse matrix */
  int n;
  /** Dimension (r x c) of the register blocks in the sparse matrix */
  int r;
  /** Dimension (r x c) of the register blocks in the sparse matrix */
  int c;
  /** Number of nonzero blocks in the matrix */
  int nnzb;
  /** How many repetitions in each benchmark run */
  int num_trials;
  /** Median completion time of the repetitions */
  double t_median;
  /** Minimum completion time of the repetitions */
  double t_min;
  /** Maximum completion time of the repetitions */
  double t_max;
  /** Millions of floating-point operations executed in a single repetition */
  double mflops; 
  /** Number of loads in a single repetition */
  int num_loads;
  /** Number of stores in a single repetition */
  int num_stores;
};


/**
 * @brief Compares two SMVM_timing_results structs by their t_min values.  
 *
 * Used for qsort'ing arrays of these structs, in order to find statistical
 * info on them (min, max, median over all trials).
 */
int
compare_timing_results_by_t_min (const void* r1, const void* r2);


/**
 * @brief Compares two SMVM_timing_results structs by their t_median values.
 * 
 * Used for qsort'ing arrays of these structs, in order to find statistical
 * info on them (min, max, median over all trials).
 */
int
compare_timing_results_by_t_median (const void* r1, const void* r2);


/**
 * Save timing results to the given SMVM_timing_results struct.
 *
 * @param p_results [OUT]  Results are stored here.  The other parameters are 
 *                    documented in the documentation for SMVM_timing_results.
 * @see SMVM_timing_results
 */
void
smvm_save_timing_results (struct SMVM_timing_results* p_results, 
			  const int m, const int n, const int r, const int c, 
			  const int nnzb, const int num_trials, 
			  const double t_median, const double t_min, 
			  const double t_max, const double mflops, 
			  const int num_loads, const int num_stores);

#endif /* _smvm_timing_results_h */

