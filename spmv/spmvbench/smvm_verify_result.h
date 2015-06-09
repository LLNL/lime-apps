#ifndef _smvm_verify_result_h
#define _smvm_verify_result_h
/**
 * @file smvm_verify_result.h
 * @author mfh
 * @since 2004 Jan 29
 * @date 2004 Oct 02
 ************************************************************************/

/**
 * Does verification of SMVM computations.  Recomputes matrix * x
 * using a general (r x c) BSR routine, and compares it with y (the
 * result of an SMVM run).  If the max-norm of the two results are not
 * within `tol' of each other, prints an error message, and returns 
 * nonzero.  Otherwise, returns zero.
 *
 * @param m            The matrix is m x n.
 * @param n
 * @param r            The register blocks in the matrix are r x c.
 * @param c
 * @param row_start    Part of the usual representation of a BSR matrix.
 * @param col_idx      Part of the usual representation of a BSR matrix.
 * @param values       Part of the usual representation of a BSR matrix
 * @param x            Source vector.
 * @param y            Original destination vector (:= matrix * x).
 * @param tol          Max-norm error tolerance.
 * @param b_warn       If nonzero, send some terse status output to stdout.
 * @param b_dbg        If nonzero, send copious debug output to stderr.
 *
 * @return             Zero if within error tolerance, nonzero otherwise.
 */
int
smvm_verify_result (int m, int n, int r, int c, int row_start[], 
		    int col_idx[], double values[], double x[],
                    double y[], double tol, int b_warn, int b_dbg);

#endif /* NOT _smvm_verify_result_h */
