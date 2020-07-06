/**
 * @file smvm_benchmark.c
 * @author mfh
 * @date 08 Nov 2005
 * 
 * 6/26/2005 -- modified by hormozd to add in new matrix generator
 ***********************************************************************/
#include "smvm_benchmark.h"
#include "smvm_timing_run.h"
#include "smvm_verify_result.h"

#include <smvm_util.h>
#include <smvm_malloc.h>
#include <create_rand.h>
#include <bcsr_matrix.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#ifdef USING_VALGRIND_CLIENT_REQUESTS
#  include <valgrind/memcheck.h>
#endif /* USING_VALGRIND_CLIENT_REQUESTS */

/**
 * Does range checking on the parameters.  Bails out with an error
 * message if any of them are out of range.
 * 
 * @param p_params     Holds the parameters for the SMVM benchmark.
 */
static void
smvm_range_check_parameters (struct SMVM_parameters *p_params)
{
  if (p_params->m < 1)
    die_with_message ("Parameter <m> must be positive ", 2);

  if( p_params->n < 1 )
    die_with_message ("Parameter <n> must be positive ", 2);

  if (p_params->r < 0 || p_params->r > 12)
    die_with_message ("Parameter <r> must satisfy 0 <= r <= 12 ", 2);

  if (p_params->c < 0 || p_params->c > 12)
    die_with_message ("Parameter <c> must satisfy 0 <= c <= 12 ", 2);

  /* changed by hormozd to allow negative percent_fill */
  if (p_params->percent_fill > 100.0)
    die_with_message ("Parameter <percent_fill> must satisfy percent_fill <= 100.0 ", 2);

  if (p_params->num_trials < 0)
    die_with_message ("Parameter <num_trials> must be nonnegative ", 2);

  if (p_params->dataoutfile == NULL)
    die_with_message ("Data output file is NULL! ", 2);
}


/***********************************************************************/
int 
compute_NNZ (int m, int n, double percent_fill)
{
  /* Hackery to ensure that in case m*n is too big to fit within an
   * int, no overflows will mess up the computations.  Specifically,
   * first we case m and n to doubles, then do all arithmetic in
   * double precision floating-point, and then recast back to int.
   * But first we check the arithmetic result to ensure that it will
   * not overflow the int.
   *
   * FIXME:  Check to make sure the doubles aren't being overflowed!!!
   */
  double m_d = (double) m;
  double n_d = (double) n;
  double fill = percent_fill / 100.0;
  
  double NNZ_d = fill * (m_d * n_d);
  if (NNZ_d > INT_MAX)
    {
      fprintf (stderr, "*** ERROR:  NNZ = %.0f is too big to fit within an "
	       "int! ***\n", NNZ_d);
      die (-1);
    }
  
  return (int) NNZ_d;
}


/***********************************************************************/
void 
smvm_allocate_sparse_data_structures (int** p_row_start, int** p_col_idx, 
				      double** p_values, int NNZ, 
				      int num_block_rows)
{
  /* It's possible that NNZ * sizeof(double) could overflow, if NNZ is large
   * enough.  This case is unlikely, because we already calculated NNZ so that
   * it doesn't overflow.  However, we check for it anyway, and abort in case
   * it happens.
   *
   * It's less likely that num_block_rows * sizeof(double) overflows, but we 
   * check anyway.
   */
  int NNZ_times_sizeof_double, num_block_rows_times_sizeof_double;

  WITH_DEBUG( fprintf(stderr, "\tsmvm_allocate_sparse_data_structures:\n") );

  if (smvm_checked_positive_product (&NNZ_times_sizeof_double, NNZ, sizeof (double)) == 0)
    {
      fprintf (stderr, "*** ERROR:  NNZ * sizeof(double) is too big to fit "
	       "in int!:  NNZ = %d, sizeof(double) = %u\n", 
	       NNZ, (unsigned)sizeof (double));
      die (-1);
    }
  
  if (smvm_checked_positive_product (&num_block_rows_times_sizeof_double, 
				num_block_rows, sizeof (double)) == 0)
    {
      fprintf (stderr, "*** ERROR:  NNZ * sizeof(double) is too big to fit "
	       "in int!:  NNZ = %d, sizeof(double) = %u\n", 
	       NNZ, (unsigned)sizeof (double));
      die (-1);
    }
  
  *p_row_start = (int*)    smvm_malloc (num_block_rows_times_sizeof_double);
  *p_col_idx   = (int*)    smvm_malloc (NNZ_times_sizeof_double);
  *p_values    = (double*) smvm_malloc (NNZ_times_sizeof_double);
}


/***********************************************************************/
int
smvm_benchmark_with_results (struct SMVM_parameters *p_params, 
			     struct SMVM_timing_results* p_results, const int b_verify)
{
  /* Output level parameters:
   *
   * b_dbg:  If nonzero, send copious debug output to stderr; if zero, surpress.
   * b_warn: If nonzero, send status messages to stdout; if zero, surpress.
   */
#ifdef DEBUG
  int b_dbg = 1;
#else
  int b_dbg = 0;
#endif /* DEBUG */

#ifdef WARN 
  int b_warn = 1;
#else
  int b_warn = 0;
#endif /* WARN */

  /* Test parameters:
   *
   * m,n
   *   Dimensions of the matrix (m x n).
   * r,c
   *   Block dimensions (r x c).
   * num_trials
   *   Number of times to run each test.
   * percent_fill
   *   100 * fill.
   * dataoutfile
   *   Filename in which to save results.
   */
  int m               = p_params->m;
  int n               = p_params->n;
  int r               = p_params->r;
  int c               = p_params->c;
  int num_trials      = p_params->num_trials;
  double percent_fill = p_params->percent_fill;
  /* FILE* dataoutfile   = p_params->dataoutfile; */

  /* Variables for constructing the matrix:
   *
   * num_nonzero_blocks   
   *   Number of nonzero (r x c) blocks in the matrix.
   * NNZ
   *   Number of nonzeros in the matrix (_actual_ number of nonzeros will be
   *   num_nonzero_blocks * r * c; it is not possible to have a fraction of a 
   *   block, so we can only have the number of nonzeros be a multiple of r*c.
   * row_start, col_idx, values
   *   storage for the block CSR matrix
   * num_block_rows
   *   The number of block rows in the matrix: rounded(m/r).
   * num_block_columns
   *   The number of block columns in the matrix: rounded(n/c).
   * row_start
   *   row_start[i] is where to look in values[] and col_idx[] for the first 
   *   entry of the i-th row in the matrix.
   * col_idx
   *   col_idx[j] is the column index of values[j] in the matrix.
   * values
   *   Where the actual values in the matrix are stored.
   * x     
   *   Source vector
   * yy0  
   *   Destination vector.
   * tol
   *   Tolerance to use for verification. Should be dim*epsilon.
   */
  int num_block_rows;
  int num_block_columns;
  int NNZ;
  int num_nonzero_blocks;
  int nnzb_per_row;
  int bw;
  int m_eff, n_eff, dimmax;
  double tol;
  double *x;
  double *yy0;
  struct bcsr_matrix_t *A = NULL;

  /***************************************************************************
   * Body of smvm_benchmark                                                  *
   ***************************************************************************/

  WITH_DEBUG( fprintf(stderr, "smvm_benchmark_with_results:\n") );

  /* Check parameters first, before computing with them. */
  WITH_DEBUG( fprintf(stderr, "\tRange checking parameters\n") );
  smvm_range_check_parameters (p_params);

  WITH_DEBUG( fprintf(stderr, "\tComputing NNZ and blocking parameters\n") );

  /* hormozd 6/27/05 -- on the one hand, we want to be able to generate
   * matrices with a certain nnz/row instead of a %nz. on the other hand,
   * accessing the old generator will still be useful. thus, to simplify
   * things (and avoid adding in any new parameters to process), we'll use
   * the percent_fill for both possibilities. a negative number (whose decimal
   * part will be cut off) will tell us that we're specifying nnz/row, and a
   * positive number will tell us that we want %nz. not that straightforward
   * of a solution, but it gets the job of integrating the new matrix
   * generator done without too much in the way of modifying the existing code
   */
  if (percent_fill < 0) {
    NNZ = (int)floor((-percent_fill)*m + .5);
  }
  else {
    NNZ = compute_NNZ (m, n, percent_fill);
  }

  num_nonzero_blocks = (int)floor(((double)NNZ/(double)(r * c)) + .5);
  num_block_rows     = (int)floor((double)m/(double)r + .5);
  num_block_columns  = (int)floor((double)n/(double)c + .5);
  m_eff = num_block_rows*r;
  n_eff = num_block_columns*c;
  dimmax = MAX(m_eff, n_eff);

  /* @todo What should tolerance be???
   *
   * hormozd 7.26.2006 4:52 PDT -- solved. tolerance should be
   * ntimes*dim*epsilon = ntimes*dim*1.11022e-16 for double precision
   */
  tol = m_eff*1.11022e-16;

  WITH_DEBUG( fprintf(stderr, "\t(m,n) = (%d,%d)\n", m, n) );
  WITH_DEBUG( fprintf(stderr, "\tpercent_fill = %g\n", percent_fill) );
  WITH_DEBUG( fprintf(stderr, "\tComputed NNZ:  %d\n", NNZ) );

  /************************************************************************
   * Init source and destination vectors.
   ************************************************************************/

  WITH_DEBUG( fprintf(stderr, "\tAllocating source and destination vectors\n") );
  x   = (double *) smvm_malloc (dimmax * sizeof(double));
  yy0 = (double *) smvm_malloc (dimmax * sizeof(double));

  /* Fill source vector with random numbers, and initialize dest to zero. */
  WITH_DEBUG( fprintf(stderr, "\tInitializing source and destination vectors\n") );
  smvm_init_vector_val (dimmax, yy0, 0.0);
  smvm_init_vector_rand (dimmax, x);

  /*************************************************************************
   * Generate a random block CSR the matrix.  The blocks will be randomly 
   * scattered throughout the matrix.  Ask for reproducible random numbers, 
   * and row-major ordering of the register blocks.  
   *************************************************************************/ 

  /* hormozd, 6/29/2005 -- again, which generator we use depends on the
     sign of percent_fill */
  if (percent_fill < 0) {
    bw = MAX(num_block_rows,num_block_columns);
    nnzb_per_row = (int)floor((-percent_fill/((double)c))+.5);

    if (nnzb_per_row == 0)
      nnzb_per_row = 1;

    num_nonzero_blocks = nnzb_per_row * num_block_rows;

    /* will we be using statistics too? */
    if (p_params->interval_fracs == NULL)
      A = create_random_matrix_banded_by_nnz_per_row(num_block_rows,
		num_block_columns, r, c, nnzb_per_row, bw, bw);
    else
      A = create_random_matrix_banded_by_statistics(num_block_rows,
		num_block_columns, r, c, nnzb_per_row,
                p_params->interval_fracs);
  }
  else {
    WITH_DEBUG( fprintf(stderr, "Generating dimension (%d, %d) matrix with "
		      "randomly placed (%d, %d) blocks; NNZ = %d.\n", 
		      m, n, r, c, NNZ) );
    A = create_random_matrix(num_block_rows, num_block_columns, r, c,
			     num_nonzero_blocks);
  }
 
  /************************************************************************* 
   * Do measurements. 
   *************************************************************************/

  /* hormozd, 6/29/2005 -- call here changes to reflect the appropriate
   * entries for row_start, col_idx, and values in the bcsr_matrix_t struct:
   *
   *	row_start -> rowptr
   *	col_idx -> colind
   *	values -> values
   */
  smvm_timing_run_with_results2 (p_results, m_eff, n_eff, r, c, A->rowptr, 
                                 A->colind, A->values, num_nonzero_blocks,
                                 x, yy0, num_trials, b_warn, b_dbg, b_verify, tol);

  WITH_DEBUG( fprintf(stderr,"\nDeallocating matrix and vectors...") );
  destroy_bcsr_matrix (A);
  smvm_free (x);
  smvm_free (yy0);
  WITH_DEBUG( fprintf(stderr,"done.\n\n") );

  return 0;
}


/***********************************************************************/
int
smvm_benchmark (struct SMVM_parameters *p_params)
{
  /* Output level parameters:
   *
   * b_dbg:  If nonzero, send copious debug output to stderr; if zero, surpress.
   * b_warn: If nonzero, send status messages to stdout; if zero, surpress.
   */
#ifdef DEBUG
  int b_dbg = 1;
#else
  int b_dbg = 0;
#endif /* DEBUG */

#ifdef WARN 
  int b_warn = 1;
#else
  int b_warn = 0;
#endif /* WARN */

  /* Test parameters:
   *
   * m,n
   *   Dimensions of the matrix (m x n).
   * r,c
   *   Block dimensions (r x c).
   * num_trials
   *   Number of times to run each test.
   * percent_fill
   *   100 * fill.
   * dataoutfile
   *   Filename in which to save results.
   */
  int m               = p_params->m;
  int n               = p_params->n;
  int r               = p_params->r;
  int c               = p_params->c;
  int num_trials      = p_params->num_trials;
  double percent_fill = p_params->percent_fill;

  FILE* dataoutfile   = p_params->dataoutfile;

  /* Variables for constructing the matrix:
   *
   * num_nonzero_blocks   
   *   Number of nonzero (r x c) blocks in the matrix.
   * NNZ
   *   Number of nonzeros in the matrix (_actual_ number of nonzeros will be
   *   num_nonzero_blocks * r * c; it is not possible to have a fraction of a 
   *   block, so we can only have the number of nonzeros be a multiple of r*c.
   * row_start, col_idx, values
   *   storage for the block CSR matrix
   * num_block_rows
   *   The number of block rows in the matrix: round(m/r).
   * num_block_columns
   *   The number of block columns in the matrix: round(n/c).
   * row_start
   *   row_start[i] is where to look in values[] and col_idx[] for the first 
   *   entry of the i-th row in the matrix.
   * col_idx
   *   col_idx[j] is the column index of values[j] in the matrix.
   * values
   *   Where the actual values in the matrix are stored.
   * x     
   *   Source vector
   * yy0  
   *   Destination vector.
   *
   */
  int num_block_rows;
  int num_block_columns;
  int NNZ;
  int num_nonzero_blocks;
  int nnzb_per_row;
  int bw;
  int m_eff, n_eff;
  double *x;
  double *yy0;
  struct bcsr_matrix_t *A = NULL;

  /* Other parameters:
   *
   * b_verify    If nonzero, run verification code.
   */
  int b_verify = 0;

  /***************************************************************************
   * Body of smvm_benchmark                                                  *
   ***************************************************************************/

  WITH_DEBUG( fprintf(stderr, "smvm_benchmark:\n") );

  /* Check parameters first, before computing with them. */
  WITH_DEBUG( fprintf(stderr, "\tRange checking parameters\n") );
  smvm_range_check_parameters (p_params);

  WITH_DEBUG( fprintf(stderr, "\tComputing NNZ and blocking parameters\n") );

  /* hormozd 6/27/05 -- on the one hand, we want to be able to generate
   * matrices with a certain nnz/row instead of a %nz. on the other hand,
   * accessing the old generator will still be useful. thus, to simplify
   * things (and avoid adding in any new parameters to process), we'll use
   * the percent_fill for both possibilities. a negative number (whose decimal
   * part will be cut off) will tell us that we're specifying nnz/row, and a
   * positive number will tell us that we want %nz. not that straightforward
   * of a solution, but it gets the job of integrating the new matrix
   * generator done without too much in the way of modifying the existing code
   */
  if (percent_fill < 0) {
    NNZ = (int)floor((-percent_fill)*m + .5);
  }
  else {
    NNZ = compute_NNZ (m, n, percent_fill);
  }

  num_nonzero_blocks = (int)floor(((double)NNZ/(double)(r * c)) + .5);
  num_block_rows     = (int)floor((double)m/(double)r + .5);
  num_block_columns  = (int)floor((double)n/(double)c + .5);
  m_eff = num_block_rows*r;
  n_eff = num_block_columns*c;

  WITH_DEBUG( fprintf(stderr, "\t(m,n) = (%d,%d)\n", m, n) );
  WITH_DEBUG( fprintf(stderr, "\tpercent_fill = %g\n", percent_fill) );
  WITH_DEBUG( fprintf(stderr, "\tComputed NNZ:  %d\n", NNZ) );

  /************************************************************************
   * Init source and destination vectors.
   ************************************************************************/

  WITH_DEBUG( fprintf(stderr, "\tAllocating source and destination vectors\n") );
  x   = (double *) smvm_malloc (n_eff * sizeof(double));
  yy0 = (double *) smvm_malloc (m_eff * sizeof(double));

  /* Fill source vector with random numbers, and initialize dest to zero. */
  WITH_DEBUG( fprintf(stderr, "\tInitializing source and destination vectors\n") );
  smvm_init_vector_val (m_eff, yy0, 0.0);
  smvm_init_vector_rand (n_eff, x);


  /*************************************************************************
   * Generate a random block CSR the matrix.  The blocks will be randomly 
   * scattered throughout the matrix.  Ask for reproducible random numbers, 
   * and row-major ordering of the register blocks.  
   *************************************************************************/ 

  /* hormozd, 6/29/2005 -- again, which generator we use depends on the
     sign of percent_fill 
     
     mfh 1 Oct 2005:  To clarify, if percent_fill is negative, its integer 
     part is the number of nonzeros per row that we want. */
  if (percent_fill < 0) 
    {
      bw = MAX(num_block_rows,num_block_columns);
      nnzb_per_row = (int)floor((-percent_fill/((double)c))+.5);

      if (nnzb_per_row == 0)
        nnzb_per_row = 1;

      num_nonzero_blocks = nnzb_per_row * num_block_rows;

      /* will we be using statistics too? */
      if (p_params->interval_fracs == NULL)
        A = create_random_matrix_banded_by_nnz_per_row(num_block_rows,
               num_block_columns, r, c, (int) (-percent_fill / (double) c),
               bw, bw);
      else
        A = create_random_matrix_banded_by_statistics(num_block_rows,
               num_block_columns, r, c, (int)(-percent_fill/(double)c),
               p_params->interval_fracs);

      if (A == NULL)
	{
	  fprintf (stderr, "*** smvm_benchmark:  create_random_matrix_banded_"
		   "by_nnz_per_row failed to create matrix (it returned NULL)"
		   "! ***\n");
	  exit (EXIT_FAILURE);
	}
    }
  else 
    {
      WITH_DEBUG( fprintf(stderr, "Generating dimension (%d, %d) matrix with "
			  "randomly placed (%d, %d) blocks; NNZ = %d.\n", 
			  m, n, r, c, NNZ) );
      A = create_random_matrix (num_block_rows, num_block_columns, r, c,
                                num_nonzero_blocks);

      if (A == NULL)
	{
	  fprintf (stderr, "*** smvm_benchmark:  create_random_matrix "
		   "failed to create matrix (it returned NULL)! ***\n");
	  exit (EXIT_FAILURE);
	}
    }
 
  /************************************************************************* 
   * Do measurements. 
   *************************************************************************/

#ifdef USING_VALGRIND_CLIENT_REQUESTS 
  /* Use Valgrind to check if A and A->rowptr are valid */
  VALGRIND_CHECK_READABLE( A, sizeof (struct bcsr_matrix_t) );
  VALGRIND_CHECK_READABLE( A->rowptr, (m/r + 1) * sizeof(int) );
#endif /* USING_VALGRIND_CLIENT_REQUESTS */

  /* hormozd, 6/29/2005 -- call here changes to reflect the appropriate
   * entries for row_start, col_idx, and values in the bcsr_matrix_t struct:
   *
   *	row_start -> rowptr
   *	col_idx -> colind
   *	values -> values
   */
  smvm_timing_run (m_eff, n_eff, r, c, A->rowptr, A->colind, A->values, 
		   num_nonzero_blocks, x, yy0, num_trials, 
		   dataoutfile, b_warn, b_dbg);

  /* Verification: Take the result of the last SMVM run, and compare it against
   * a general (r x c) BSR matrix-vector multiplication routine.
   * 
   * FIXME:  What should tolerance be???
   */
  if (b_verify)
    smvm_verify_result (m_eff, n_eff, r, c, A->rowptr, A->colind, A->values,
                        x, yy0, 0.000001, b_warn, b_dbg);

  WITH_DEBUG( fprintf (stderr, "\nDeallocating matrix and vectors...") );
  destroy_bcsr_matrix (A);
  smvm_free (x);
  smvm_free (yy0);
  WITH_DEBUG( fprintf (stderr, "done.\n\n") );

  return 0;
}
