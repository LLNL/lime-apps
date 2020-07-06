/**
 * @file smvm_verify_result.c
 * @author mfh
 * @date 2004 Sep 28
 ************************************************************************/
#include "smvm_verify_result.h"

#include <smvm_malloc.h>
#include <smvm_util.h>
#include <stdio.h>
#include <math.h>


/**
 * Returns the max norm of the difference between y and y_check.
 * 
 * @param y
 * @param y_check
 * @param n         Number of elements in y, and also in y_check.
 *
 * @return $\|y - y_check\|_{\infty}$
 */
double
smvm_max_norm_error (double y[], double y_check[], int n)
{
  int i;
  double maxerr = 0.0;

  for (i = 0; i < n; i++) {
    maxerr = MAX (maxerr, fabs (y[i] - y_check[i]));
    /*printf("[%d] expect:%f got:%f\n", i, y_check[i], y[i]);*/
  }

  return maxerr;
}

/**
 * Do matrix-vector multiplication on a general BSR matrix with r x c register
 * blocks.  Blocks must be row- and column-aligned, and all must have the same r
 * x c dimensions.
 */
void 
smvm_general_bsr_matvec(int num_block_rows, int r, int c, int row_start[], 
                        int col_idx[], double values[], double src[], 
                        double dest[])
{
  /* i,j:    Indices of blocks.
   * ii,jj:  Indices within a block.
   */
  int i, j, ii, jj;

  for (i = 0; i < num_block_rows; i++, dest += r)
    for (j = row_start[i]; j < row_start[i+1]; j++, col_idx++, values += r*c)
      for (ii = 0; ii < r; ii++)
        for (jj = 0; jj < c; jj++)
          dest[ii] += values[ii*c + jj] * src[(*col_idx) + jj];
}


/************************************************************************/
int
smvm_verify_result (int m, int n, int r, int c, int row_start[], 
                    int col_idx[], double values[], double x[],
                    double y[], double tol, int b_warn, int b_dbg)
{
  int     num_block_rows = m / r;
  double* y_check        = smvm_calloc (m, sizeof(double));
  double  err;

  smvm_general_bsr_matvec(num_block_rows, r, c, row_start, col_idx, values, 
                          x, y_check);

  err = smvm_max_norm_error (y, y_check, m);
  smvm_free(y_check);

  if (err > tol)
    {
      fprintf (stderr, "*** SMVM:  error %e exceeds tolerance %e ***\n", 
               err, tol);

      return 1;
    }

  return 0;
}
