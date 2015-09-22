#ifndef _smvm_benchmark_h
#define _smvm_benchmark_h
/**
 * @file smvm_benchmark.h
 * @author mfh
 * @date 23 Sep 2005
 ***********************************************************************/
#include <stdio.h> 
#include "smvm_timing_results.h"


/**
 * @brief Parameters that govern the SMVM benchmark.
 * 
 * Parameters that govern invocation of the core sparse matrix-vector 
 * multiplication benchmark.  
 */
struct SMVM_parameters
{
  /** Number of rows in the matrix.     */
  int   m; 

  /** Number of columns in the matrix.  */
  int   n; 

  /** Register block dimension (r x c). */
  int   r; 

  /** Register block dimension (r x c). */
  int   c; 

  /** Number of times to repeat timing run. */
  int   num_trials;    

  /** Percentage fill in the sparse matrix. */
  double percent_fill; 

  /** Pointer to output file where results go. */
  FILE* dataoutfile;

  /* new parameter: array of deciles for using statistics */
  double *interval_fracs;
};


/**
 * Computes NNZ as a function of m, n and percent_fill.  This is
 * trickier than it seems, because m*n is very large (in practical
 * cases, > 2^{31}), but percent_fill (or rather, fill itself) is
 * small.
 *
 * @param m             Number of rows there will be in sparse matrix.
 * @param n             Number of columns there will be in sparse matrix.
 * @param percent_fill  Percent of entries in matrix with nonzeros.
 *
 * @return              Number of nonzeros there should be in the matrix.
 */
int 
compute_NNZ (int m, int n, double percent_fill);


/**
 * Allocates memory for the sparse data structures.
 *
 * @param p_row_start     Pointer to row_start[] address.
 * @param p_col_idx       Pointer to col_idx[] address.
 * @param p_values        Pointer to values[] address.
 * @param NNZ             Number of nonzeros that will be in the matrix 
 *                        (caveat:  actual number could be NNZ - (r*c - 1)).
 * @param num_block_rows  Number of block rows that will be in the matrix.
 */
void 
smvm_allocate_sparse_data_structures (int** p_row_start, int** p_col_idx, 
				      double** p_values, int NNZ, 
				      int num_block_rows);

/**
 * Runs all cases of the benchmark, and sends raw results to output file.
 *
 * @param p_params [IN,OUT]    Parameters for the benchmark 
 *                             (including output file stream).
 */
int
smvm_benchmark (struct SMVM_parameters *p_params);


/**
 * Runs all cases of the benchmark and stores the raw results in *p_results.
 * The caller is responsible for allocating p_results.
 *
 * @param p_params [IN]    Parameters for the benchmark.
 * @param p_results [OUT]  Timing results stored here.
 */
int
smvm_benchmark_with_results (struct SMVM_parameters *p_params, 
			     struct SMVM_timing_results* p_results, const int b_verify);


#endif /* NOT _smvm_benchmark_h */
