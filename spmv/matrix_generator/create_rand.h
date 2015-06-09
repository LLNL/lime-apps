#ifndef _create_rand_h
#define _create_rand_h
/*****************************************************************************
 * @file create_matrix.h
 * @author mfh
 * @since Fall 2003
 * @date 30 Oct 2005
 *
 * Functions for creating block CSR matrix with randomly (uniformly) 
 * distributed blocks.  
 ****************************************************************************/

/* forward declaration */
struct bcsr_matrix_t;

/** 
 * @brief Creates a random block compressed sparse row matrix.
 *
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block 
 * dimensions bm x bn.  The actual dimensions of the matrix are r*bm x c*bn.
 * The matrix contains nnzb dense blocks of nonzeros.  Each of these blocks has
 * the dimensions r x c.  Blocks are aligned so that if (i,j) are the indices 
 * of the upper left corner of a block, then i+1 divides r and j+1 divides c.  
 *
 * @param bm   [IN]  Number of block rows (m == r * bm)
 * @param bn   [IN]  Number of block columns (n == c * bn)
 * @param r    [IN]  Number of rows in each dense subblock
 * @param c    [IN]  Number of columns in each dense subblock
 * @param nnzb [IN]  Number of r x c nonzero blocks
 *
 * @return Dynamically allocated bcsr_matrix_t object, which is the requested 
 *         sparse matrix, if no error; else NULL on error.
 */
struct bcsr_matrix_t*
create_random_matrix (const int bm, const int bn, const int r, const int c, 
		      const int nnzb);


/** 
 * @brief Creates a banded random block compressed sparse row matrix.
 *
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block 
 * dimensions bm x bn.  The actual dimensions of the matrix are r*bm x c*bn.
 * The matrix contains nnzb dense blocks of nonzeros.  Each of these blocks has
 * the dimensions r x c.  Blocks are aligned so that if (i,j) are the indices 
 * of the upper left corner of a block, then i+1 divides r and j+1 divides c.  
 * The blocks are constrained to fit within the given block bandwidths.
 *
 * @param bm   [IN]  Number of block rows (m == r * bm)
 * @param bn   [IN]  Number of block columns (n == c * bn)
 * @param r    [IN]  Number of rows in each dense subblock
 * @param c    [IN]  Number of columns in each dense subblock
 * @param nnzb [IN]  Number of r x c nonzero blocks
 * @param lower_block_bandwidth [IN]  Number of nonzero block rows that are 
 *                                    allowed below the diagonal.
 * @param upper_block_bandwidth [IN]  Number of nonzero block rows that are 
 *                                    allowed above the diagonal.
 *
 * @return Dynamically allocated bcsr_matrix_t object, which is the requested 
 *         sparse matrix, if no error; else NULL on error.
 */
struct bcsr_matrix_t*
create_random_matrix_banded (const int bm, const int bn, 
			     const int r, const int c, const int nnzb, 
			     const int lower_block_bandwidth, 
			     const int upper_block_bandwidth);


/**
 * @brief Creates a banded random block compressed sparse matrix with a fixed nnz/row.
 * 
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block
 * dimensions bm x bn.  The number of nonzeros per block row is fixed.  The
 * actual dimensions of the matrix are r*bm x c*bn.  The matrix contains nnzb
 * dense blocks of nonzeros.  Each of these blocks has the dimensions r x c.
 * Blocks are aligned so that if (i,j) are the indices of the upper left corner
 * of a block, then i+1 divides r and j+1 divides c.  The blocks are
 * constrained to fit within the given block bandwidths.
 */
struct bcsr_matrix_t*
create_random_matrix_banded_by_nnz_per_row (const int bm, const int bn, 
					    const int r, const int c, 
					    const int nnzb_per_block_row, 
					    const int lower_block_bandwidth, 
					    const int upper_block_bandwidth);

/* statistics-based matrix generator */
struct bcsr_matrix_t*
create_random_matrix_banded_by_statistics (const int bm, const int bn,
                                           const int r, const int c,
                                           const int nnzb_per_block_row,
                                           const double *interval_fracs);

/**
 * @brief Creates a banded random block compressed sparse matrix with a fixed nnz/row.
 * 
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block
 * dimensions bm x bn.  The number of nonzeros per block row is fixed.  The
 * actual dimensions of the matrix are r*bm x c*bn.  The matrix contains nnzb
 * dense blocks of nonzeros.  Each of these blocks has the dimensions r x c.
 * Blocks are aligned so that if (i,j) are the indices of the upper left corner
 * of a block, then i+1 divides r and j+1 divides c.  The blocks are
 * constrained to fit within the given block bandwidths.  This function also 
 * allows the user to force a particular algorithm for generating the matrix:
 * if "algorithm" is zero, the algorithm is automatically selected.  If 
 * "algorithm" is one, an iterative algorithm is used, and if "algorithm" is 
 * two, the (usually slower) deterministic algorithm is used.  "Deterministic" 
 * doesn't refer to the positions of the entries in the matrix (those are always 
 * random), but to the runtime of the algorithm; the iterative algorithm does 
 * not have a guaranteed runtime (though it does revert to the deterministic 
 * algorithm if the number of iterations required is too high).
 */
struct bcsr_matrix_t*
create_random_matrix_banded_by_nnz_per_row_using_algorithm (const int bm, const int bn, 
							    const int r, const int c, 
							    const int nnzb_per_block_row, 
							    const int lower_block_bandwidth, 
							    const int upper_block_bandwidth,
							    const int algorithm);




#endif /* NOT _create_rand_h */
