#ifndef _smvm_timing_run_h
#define _smvm_timing_run_h
/**
 * @file smvm_timing_run.h
 * @author mfh
 * @date 2004 Oct 02
 *****************************************************************************/
#include <stdio.h>
#include "smvm_timing_results.h"

/**
 * @brief Number of flops required for BSR SpMV.
 *
 * Returns the number of flops required by a BSR sparse matrix-vector
 * multiplication with the given number of nonzero blocks.  Note that it
 * depends only upon the number of nonzeros:
 *
 *   For all (i,j) such that A(i,j) != 0:  dest(i) = dest(i) +  A(i,j) * src(j)
 *
 * We count each of these as two flops, even if there is a fused multiply-add
 * instruction, because a processor should be rewarded if it can complete two
 * operations in one.  The execution time will reflect whatever performance
 * gains the fused multiply-add may cause.
 *
 * @param r [IN]      Register block dimension (r x c)
 * @param c [IN]      Register block dimension (r x c)
 * @param nnzb [IN]   Number of nonzero register blocks in the matrix.
 *
 * @return Number of flops required for BSR SpMV (double since may be too
 * large for int).
 */
double
flops (const int r, const int c, const int nnzb);


/**
 * Returns the bytes of loads than a BCSR SpMV requires.  Both floating-point
 * values and index arrays are counted.
 *
 * @param r  Each register block in the matrix has dimensions r x c.
 * @param c
 * @param nnzb              Number of nonzero blocks in the matrix.
 * @param num_block_rows    Number of block rows in the matrix.
 */
long
loads (const int r, const int c, const int nnzb, const int num_block_rows);


/**
 * Returns the bytes of stores than a BSR SMVM requires.  Both floating-point
 * values and index arrays are counted.
 *
 * @param r  Each register block in the matrix has dimensions r x c.
 * @param c
 * @param nnzb   Number of nonzero blocks in the matrix.
 */
long
stores (const int r, const int c, const int nnzb);


/**
 * Run benchmark num_trials times, and store results in *p_results.
 * Unless the `warn' or `debug' options are specified, no output is
 * written to stdout or stderr.
 * 
 * @param p_results [OUT]        Pointer to struct for holding benchmark results.
 * @param m [IN]  Number of rows in the sparse matrix.
 * @param n [IN]  Number of columns in the sparse matrix.
 * @param r [IN]  Number of rows in each register block in the sparse matrix.
 * @param c [IN]  Number of columns in each register block in the sparse matrix.
 * @param row_start [IN]   Part of the usual representation of a BSR matrix.
 * @param col_idx [IN]     Part of the usual representation of a BSR matrix.
 * @param values [IN]      Part of the usual representation of a BSR matrix.
 * @param nnzb [IN]        Number of nonzero blocks in the matrix.
 * @param src [IN]         Source vector for the matrix - vector multiplication.
 * @param dest [OUT]       Destination vector the the matrix - vector multiplication.
 * @param num_trials [IN]  How many times to repeat each run of the benchmark.
 * @param b_warn [IN]      If nonzero, send status output to stdout.
 * @param b_dbg [IN]       If nonzero, send (copious) debug output to stderr.
 */
void
smvm_timing_run_with_results (struct SMVM_timing_results* p_results,
			      const int m, const int n, const int r, 
			      const int c, const int row_start[], 
			      const int col_idx[], const double values[], 
			      const int nnzb, const double src[], 
			      double dest[], const int num_trials, 
			      const int b_warn, const int b_dbg);

void
smvm_timing_run_with_results2 (struct SMVM_timing_results* p_results,
			       const int m, const int n, const int r, 
			       const int c, int row_start[], 
			       int col_idx[], double values[], 
			       const int nnzb, double src[], 
			       double dest[], const int num_trials, 
			       const int b_warn, const int b_dbg,
                               const int b_verify, const double tol);

/**
 * @brief Collect data for num_trials repetitions of the benchmark.
 *
 * Run SMVM num_trials times, collecting data using the selected timer.
 * Output results to outfile. 
 * 
 * @param m   The matrix is m x n.
 * @param n
 * @param r   The register blocks in the matrix are r x c.
 * @param c
 * @param row_start    Part of the usual representation of a BSR matrix.
 * @param col_idx      Part of the usual representation of a BSR matrix.
 * @param values       Part of the usual representation of a BSR matrix.
 * @param nnzb         Number of nonzero blocks in the matrix.
 * @param src          Source vector for the matrix - vector multiplication.
 * @param dest         Destination vector the the matrix - vector mult.
 * @param num_trials   How many times to run SMVM.
 * @param outfile      Data file in which to put the results.
 * @param b_warn       If nonzero, send some terse status output to stdout.
 * @param b_dbg        If nonzero, send copious debug output to stderr.
 */
void
smvm_timing_run (int m, int n, int r, int c, int row_start[], int col_idx[], 
		 double values[], int nnzb, double src[], double dest[], 
		 int num_trials, FILE* outfile, int b_warn, int b_dbg);

#endif /* NOT _smvm_timing_run_h */

