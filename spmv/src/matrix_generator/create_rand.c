/** 
 * @file create_rand.c
 * @author mfh
 * @since Fall 2003
 * @date 30 Oct 2005
 *
 * Implementation of functions to create a block CSR matrix with randomly
 * scattered blocks.  
 ****************************************************************************/

#include <create_rand.h>

#include <bcoo_matrix.h>
#include <bcsr_matrix.h>
#include <enumerations.h>
#include <random_number.h>
#include <smvm_malloc.h>
#include <smvm_util.h>
#include <sort_joint_arrays.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

/** 
 * Macro returning the absolute value of x.  
 *
 * Assumes the following operations make sense:
 * - Negation:  -(x)
 * - less-than comparison with the integer zero:  (x) < 0
 *
 * Unfortunately, the only way in C to avoid evaluating x in this macro more
 * than once is to rely on gcc-specific typing tricks.  There's no way (as far
 * as I know) to get away with it in ANSI C99.  If we were coding in Common 
 * Lisp it would be easy ;) 
 */
#ifdef ABS
#  undef ABS
#endif /* #ifdef ABS */
#define ABS( x )  ((x) < 0 ? -(x) : (x))



static int
compare_ints (const void* a, const void* b)
{
  int x = *((int*) a);
  int y = *((int*) b);

  if (x < y)
    return -1;
  else if (x > y)
    return +1;
  else 
    return 0;
}


/** 
 * Removes duplicates from the given array, and returns the new length
 * of the array.
 *
 * @param array [IN,OUT]  The array from which to remove duplicates
 * @param length [IN]     The old length of the array 
 */
static int
remove_duplicates (int* array, int length)
{
  int curidx = 0;
  int i = 1;

  if (length == 0)
    return 0;

  /* We have to sort the array first to remove duplicates */
  qsort (array, length, sizeof (int), compare_ints);

  while (i < length)
    {
      if (array[i] != array[curidx])
	curidx++;

      array[curidx] = array[i];
      i++;
    }

  return curidx + 1;
}




/** 
 * @brief Creates a banded random block compressed sparse row matrix.
 *
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block 
 * dimensions bm x bn.  The actual dimensions of the matrix are r*bm x c*bn.
 * The matrix contains nnzb dense blocks of nonzeros.  Each of these blocks has
 * the dimensions r x c.  Blocks are aligned so that if (i,j) are the indices 
 * of the upper left corner of a block, then i+1 divides r and j+1 divides c.  
 * This particular function does no verification of the inputs.
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
static struct bcsr_matrix_t*
__create_random_matrix_banded (const int bm, const int bn, 
			       const int r, const int c, 
			       const int nnzb, 
			       const int lower_block_bandwidth, 
			       const int upper_block_bandwidth)
{
  struct bcoo_matrix_t* A = NULL;
  struct bcsr_matrix_t* B = NULL;
  int num_iterations = 0;
  /* Just a guess for now */
  const int MAX_NUM_ITERATIONS = 10 + nnzb; 

  WITH_DEBUG2(fprintf(stderr, "=== __create_random_matrix_banded ===\n"));

  A = create_empty_bcoo_matrix (bm, bn, r, c, 0, UNSYMMETRIC, 0, PATTERN);

  /*
   * We first add random entries to the matrix.  This process doesn't check for
   * duplicates, so afterwards we have to go back and remove duplicates.  Once
   * we have removed duplicates, we need to add more entries, remove duplicates
   * again, and so on, until we have enough entries.  We only continue this
   * process a fixed number (MAX_NUM_ITERATIONS) of times; if that isn't
   * enough, we give up with an error message.
   */


  while (num_iterations < MAX_NUM_ITERATIONS && A->nnzb < nnzb)
    {
      int num_removed = 0;

      WITH_DEBUG2(fprintf (stderr, "Iteration %d\n", num_iterations + 1));
      while (A->nnzb < nnzb)
	{
	  const int bi = smvm_random_integer (0, bm-1);
	  const int bj = smvm_random_integer (MAX(0, bi - lower_block_bandwidth), 
					      MIN(bn - 1, bi + upper_block_bandwidth));

	  bcoo_matrix_add_entry (A, bi, bj, NULL);
          WITH_DEBUG2(fprintf (stderr, "\tAdded entry (%d,%d)\n", bi, bj));
	  assert (bi >= 0 && bi < bm);
	  assert (bj >= 0 && bj < bn);

	  if (smvm_debug_level() > 1)
	    {
	      /* Make sure that the BCOO data structure is valid */
	      if (! valid_bcoo_matrix_p (A))
		{
		  fprintf (stderr, "*** __create_random_matrix_banded: A is i"
			   "nvalid at iteration %d after adding an entry (%d,"
			   "%d) ***\n", num_iterations, bi, bj);
		  exit (EXIT_FAILURE);
		}
	    }
	}

      /* Remove duplicate entries.  We can use the "coalesce" routine because
       * the value type of A is PATTERN (so we don't have to worry about 
       * whether or not the duplicate entries will have their values added 
       * together -- because they don't have any values). */
      bcoo_matrix_coalesce_duplicate_entries (A, &num_removed);

      if (smvm_debug_level() > 1)
	{
	  int diff;

	  /* Check if A is a valid BCOO matrix */
	  if (! valid_bcoo_matrix_p (A))
	    {
	      fprintf (stderr, "*** __create_random_matrix_banded: Removing "
		       "duplicate entries seems to have corrupted the BCOO "
		       "structure of the sparse matrix A! ***\n");
	      exit (EXIT_FAILURE);
	    }

	  /* Make sure that A->II and A->JJ don't overlap */
	  /* fprintf (stderr, "Checking to make sure that A->II and A->JJ do not overlap\n"); */
	  diff = ABS(A->JJ - A->II);

	  if (diff < A->nnzb)
	    {
	      fprintf (stderr, "*** __create_random_matrix_banded: A->II and "
		       "A->JJ overlap by %d elements, after calling bcoo_matrix_coalesce_duplicate_entries ! ***\n", 
		       A->nnzb - diff);
	      fprintf (stderr, "*** nnzb = %d, diff = %d ***\n", 
		       A->nnzb, diff);
	      exit (EXIT_FAILURE);
	    }
	}


      WITH_DEBUG(fprintf(stderr, "\t-- Removed %d duplicates\n", num_removed));
      WITH_DEBUG(fprintf(stderr, "\t-- Total nnzb (no dups) is now %d\n", A->nnzb));
      WITH_DEBUG(fprintf(stderr, "\t-- Target nnzb: %d\n", nnzb));

      num_iterations++;
    }

  if (A->nnzb < nnzb)
    {
      fprintf (stderr, "*** __create_random_matrix_banded: Failed to achieve "
	       "%d nonzero blocks after %d iterations ***\n", 
	       nnzb, num_iterations);
      destroy_bcoo_matrix (A);
      return NULL;
    }

  if (smvm_debug_level() > 0)
    {
      /* Make sure that A->II and A->JJ don't overlap */
      int diff;
      fprintf (stderr, "Checking to make sure that A->II and A->JJ do not overlap\n");
      diff = ABS(A->JJ - A->II);

      if (diff < A->nnzb)
	{
	  fprintf (stderr, "*** __create_random_matrix_banded: A->II and "
		   "A->JJ overlap by %d elements! ***\n", 
		   A->nnzb - diff);
	  fprintf (stderr, "*** nnzb = %d, diff = %d ***\n", 
		   A->nnzb, diff);
	  exit (EXIT_FAILURE);
	}
    }

  B = bcoo_matrix_to_random_bcsr_matrix (A);
  destroy_bcoo_matrix (A);

  return B;
}

/**
 * Special case of __create_random_matrix_banded_by_nnz_per_row when the rows 
 * are nearly dense.  We use a function from the BeBOP Utility Library that 
 * lets us choose integers from a range without replacement.  The resulting
 * algorithm is no longer iterative and is guaranteed to succeed, but the 
 * workspace requirements are increased to O(bn), and the worst-case runtime
 * is O(bm bn nnzb_per_block_row) (instead of O(bm nnzb_per_block_row)).
 */
static struct bcsr_matrix_t*
__create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows (const int bm, const int bn, 
								 const int r, const int c, 
								 const int nnzb_per_block_row, 
								 const int lower_block_bandwidth, 
								 const int upper_block_bandwidth)
{
  const int nnzb = nnzb_per_block_row * bm;
  const int nnz = nnzb * r * c;
  int bi = 0; /* index of current block row */
  struct bcsr_matrix_t* A = smvm_calloc (1, sizeof (struct bcsr_matrix_t));

  /* Vector for holding the column indices of the current row */
  int* current_row = smvm_calloc (nnzb_per_block_row, sizeof (int));

  WITH_DEBUG2(fprintf(stderr, "=== __create_random_matrix_banded_by_nnz_per"
	"_row__nearly_dense_rows ===\n"));

  A->bm = bm;
  A->bn = bn;
  A->r = r;
  A->c = c;
  A->nnzb = nnzb;
  A->values = smvm_malloc (nnz * sizeof (double));
  A->colind = smvm_malloc (nnzb * sizeof (int));
  A->rowptr = smvm_malloc ((bm + 1) * sizeof (int));
  A->symmetry_type = UNSYMMETRIC;
  A->symmetric_storage_location = 0;
  A->value_type = REAL;
  A->col_oriented_p = 0;

  /* 
   * For each row, we add nnzb_per_block_row entries to that row, taking 
   * integers without replacement from the range of valid column indices for 
   * that row.
   */
  for (bi = 0; bi < bm; bi++)
    {
      /* Key to local variables:
       *
       * num_added_to_this_row:
       *   Number of block entries that have been added to this row thus far.
       * gen:
       *   Returns random integers without replacement from the valid range of
       *   column indices for this row.
       */
      int num_added_to_this_row = 0;
      struct random_integer_from_range_without_replacement_generator_t* gen = NULL;

      WITH_DEBUG2(fprintf(stderr, "Block row %d\n", bi));

      gen = create_random_integer_from_range_without_replacement_generator (
	MAX(0, bi - lower_block_bandwidth), 
	MIN(bn - 1, bi + upper_block_bandwidth));

      while (num_added_to_this_row < nnzb_per_block_row)
	{
	  int bj = -1;
	  int failure = 0;

	  failure = return_random_integer_from_range_without_replacement (&bj, gen);
	  if (failure)
	    {
	      fprintf (stderr, "*** __create_random_matrix_banded_by_nnz_per_"
		       "row__nearly_dense_rows: At block row %d, failed to ge"
		       "nerate the next random block column index! ***\n", 
		       bi);
	      destroy_random_integer_from_range_without_replacement_generator (gen);
	      destroy_bcsr_matrix (A);
	      smvm_free (current_row);
	      return NULL;
	    }
	  WITH_DEBUG2(fprintf(stderr, "\t\tAdding block entry (%d,%d); %d left to choose from\n",
			      bi, bj, gen->num_remaining));
	  current_row[num_added_to_this_row] = bj;
	  num_added_to_this_row++;
	}

      destroy_random_integer_from_range_without_replacement_generator (gen);

      /* Sort the column indices of the current block row */
      qsort (current_row, num_added_to_this_row, sizeof (int), compare_ints);

      /* Copy the new column indices into the current row of the matrix */
      A->rowptr[bi] = nnzb_per_block_row * bi;
      {
        int k;
        for (k = 0; k < nnzb_per_block_row; k++)
          A->colind[nnzb_per_block_row*bi + k] = current_row[k];
      }
    }

  /* ??? should it be nnzb or nnz? */
  A->rowptr[bm] = nnzb;

  smvm_free (current_row);

  /* Fill in the values array with random doubles */
  {
    double* vals = (double*) (A->values);
    int k;
    for (k = 0; k < nnz; k++)
      vals[k] = smvm_random_double (-1.0, 1.0);
  }

  WITH_DEBUG2(fprintf(stderr, "=== Done with __create_random_matrix_banded_by_nnz_per"
	"_row__nearly_dense_rows ===\n"));
  return A;
}


/**
 * Optimized version of __create_random_matrix_banded_by_nnz_per_row that 
 * does not use a COO format sparse matrix as an intermediate step.
 */
static struct bcsr_matrix_t*
__create_random_matrix_banded_by_nnz_per_row2 (const int bm, const int bn, 
					       const int r, const int c, 
					       const int nnzb_per_block_row, 
					       const int lower_block_bandwidth, 
					       const int upper_block_bandwidth)
{
  const int nnzb = nnzb_per_block_row * bm;
  const int nnz = nnzb * r * c;

  struct bcsr_matrix_t* A = smvm_calloc (1, sizeof (struct bcsr_matrix_t));
  /* Just a guess for now */
  const int MAX_NUM_ITERATIONS = 2 * nnzb_per_block_row * nnzb_per_block_row; 
  int bi;

  /* Vector for holding the column indices of the current row */
  int* current_row = smvm_calloc (nnzb_per_block_row, sizeof (int));

  A->bm = bm;
  A->bn = bn;
  A->r = r;
  A->c = c;
  A->nnzb = nnzb;
  A->values = smvm_malloc (nnz * sizeof (double));
  A->colind = smvm_malloc (nnzb * sizeof (int));
  A->rowptr = smvm_malloc ((bm + 1) * sizeof (int));
  A->symmetry_type = UNSYMMETRIC;
  A->symmetric_storage_location = 0;
  A->value_type = REAL;
  A->col_oriented_p = 0;

  /* 
   * For each row, we add nnzb_per_block_row entries to that row.  This process
   * doesn't check for duplicates, so afterwards we have to go back and remove
   * duplicates.  Once we have removed duplicates, we need to add more entries,
   * remove duplicates again, and so on, until we have enough entries in the row.  
   * We only continue this process a fixed number (MAX_NUM_ITERATIONS) of times
   * per row; if that isn't enough, we give up with an error message.  After 
   * finishing with that row, we move on to the next one.
   */

  for (bi = 0; bi < bm; bi++) 
    {
      /* Key to local variables:
       *
       * num_added_without_duplicates:  
       *   Number of block entries that we have confirmed added to the block 
       *   row this far, in the sense that we have searched them for duplicates
       *   and found none.
       * num_iterations:
       *   Number of rounds that we have taken to try to add the requested 
       *   number of block entries to the current block row.  We take no more 
       *   than MAX_NUM_ITERATIONS tries to add the entries.  If we don't 
       *   succeed after that many iterations, we revert to a different, non-
       *   iterative algorithm for generating the matrix.
       */
      int num_added_without_duplicates = 0;
      int num_iterations = 0;

      WITH_DEBUG2(fprintf(stderr, "Block row %d\n", bi));

      while (num_iterations < MAX_NUM_ITERATIONS && 
	     num_added_without_duplicates < nnzb_per_block_row)
	{
	  /* Key to local variables:
	   *
	   * num_added_this_round_with_dups:
	   *   Number of block entries that we have added _this_iteration_.
	   *   Duplicate entries are counted.  We will remove the duplicate
	   *   entries at the end of this iteration.
	   */
	  int num_added_this_round_with_dups = 0;

	  WITH_DEBUG2(fprintf(stderr, "\tIteration %d, added %d thus far to row\n", 
				  num_iterations + 1, num_added_without_duplicates));
	  /* 
	   * While the total number of block entries added to this row 
	   * (including the potential duplicates from this round) is less than
	   * the number we are supposed to add, add another block entry.
	   */
	  while (num_added_without_duplicates + num_added_this_round_with_dups < nnzb_per_block_row)
	    {
              /* Constrain the column index so that the block entry fits 
	       * within the block bandwidth. */
	      const int bj = 
		smvm_random_integer (MAX(0, bi - lower_block_bandwidth), 
	                             MIN(bn - 1, bi + upper_block_bandwidth));
              WITH_DEBUG2(fprintf(stderr, "\t\tAdding block entry (%d,%d)\n",
                   bi, bj));
	      current_row[num_added_without_duplicates + num_added_this_round_with_dups] = bj;
	      num_added_this_round_with_dups++;
	    }
          WITH_DEBUG2(fprintf(stderr, "\t-- Added %d this round with potential dups\n",
               num_added_this_round_with_dups));

	  /* Remove duplicate entries in this row */
	  num_added_without_duplicates = 
	    remove_duplicates (current_row, num_added_without_duplicates + 
			                    num_added_this_round_with_dups);

	  num_iterations++;
	}

      if (num_added_without_duplicates < nnzb_per_block_row)
	{
	  WITH_DEBUG2(fprintf (stderr, "*** __create_random_matrix_banded_by_nnz_per_row: at blo"
			       "ck row %d, added only %d nonzero blocks instead of at least %d: "
			       "reverting to \"deterministic\" algorithm ***\n", 
			       bi, num_added_without_duplicates, nnzb_per_block_row));
	  smvm_free (current_row);
	  destroy_bcsr_matrix (A);
	  return
	    __create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows
	    (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth,
	     upper_block_bandwidth);
	}
      else 
	{
	  int k;
	  /* We succeeded in adding all the entries which we wanted to add to 
	   * the current row, so now we can copy the entries into the matrix. */
	  A->rowptr[bi] = nnzb_per_block_row * bi;
	  for (k = 0; k < nnzb_per_block_row; k++)
	    A->colind[nnzb_per_block_row*bi + k] = current_row[k];
	}
    }

  /* ??? should it be nnzb or nnz? */
  A->rowptr[bm] = nnzb;

  smvm_free (current_row);

  /* Fill in the values array with random doubles */
  {
    double* vals = (double*) (A->values);
    int k;
    for (k = 0; k < nnz; k++)
      vals[k] = smvm_random_double (-1.0, 1.0);
  }

  return A;
}

/* added by hormozd 4/23/2006 
 * matrix generator that assigns nnzb to rows at intervals with size equal to
 * 10% of the number of columns according to passed-in statistics */
static struct bcsr_matrix_t*
__create_random_matrix_banded_by_statistics (const int bm, const int bn, 
					     const int r, const int c, 
					     const int nnzb_per_block_row, 
					     const double *interval_fracs)
{
  const int nnzb = nnzb_per_block_row * bm;
  const int nnz = nnzb * r * c;

  struct bcsr_matrix_t* A = smvm_calloc (1, sizeof (struct bcsr_matrix_t));
  /* Just a guess for now */
  const int MAX_NUM_ITERATIONS = 2 * nnzb_per_block_row * nnzb_per_block_row; 
  int bi, i, idx, tmp1, tmp2, decile_highest = 0, insertpt;

  /* hold info about deciles of entries */
  int *entries_per_decile, *colinds_to_add;
  int entries_per_decile_per_row, row_elem_add_ct;
  double nnzb_per_decile_this_row[10];
  int nnzb_per_decile_this_row_rounded[10];
  int deciles_highest_to_lowest[10];
  int decile_start_left_rounded, decile_end_left_rounded;
  int decile_start_right_rounded, decile_end_right_rounded;
  int decile_length, decile_split_point, entries_per_decile_prev;
  double diagpos = 0.0, dim_div_10;
  double decile_start_left, decile_start_right;
  double decile_end_left, decile_end_right;
  int nnzb_to_add_this_row, nnzb_adjustment_idx, amt_added;
  double overflow_guard[10];   /* protects entries_per_decile from overflow */
  double decile_proportion_counters[10];   /* to make sure roundoff compensation is done proportionally */
  int decile_rounded_proportion_counters[10];
  int decile_lengths[10];

  /* Vector for holding the column indices of the current row */
  int* current_row = smvm_calloc(nnzb_per_block_row, sizeof (int));
  /* allocate space for decile data */
  entries_per_decile = smvm_calloc(10, sizeof(int));
  colinds_to_add = smvm_calloc(nnzb_per_block_row, sizeof(int));

  /* initialize overflow-handling arrays */
  for (i = 0; i < 10; i++)
    overflow_guard[i] = 0.0;

  A->bm = bm;
  A->bn = bn;
  A->r = r;
  A->c = c;
  A->nnzb = nnzb;
  A->values = smvm_malloc (nnz * sizeof (double));
  A->colind = smvm_malloc (nnzb * sizeof (int));
  A->rowptr = smvm_malloc ((bm + 1) * sizeof (int));
  A->symmetry_type = UNSYMMETRIC;
  A->symmetric_storage_location = 0;
  A->value_type = REAL;
  A->col_oriented_p = 0;
  dim_div_10 = (double)bn/10.0;

  /* arrange decile fractions from highest to lowest */
  deciles_highest_to_lowest[0] = 0;

  for (i = 1; i < 10; i++) {
    if (interval_fracs[i] > interval_fracs[decile_highest]) {
      decile_highest = i;
      insertpt = 0;
    }
    else {
      /* find insertion point */
      insertpt = 1;

      while (insertpt < i && interval_fracs[deciles_highest_to_lowest[insertpt]] > interval_fracs[i])
        insertpt++;
    }

    tmp1 = deciles_highest_to_lowest[insertpt];
    deciles_highest_to_lowest[insertpt] = i;

    for (bi = insertpt+1; bi <= i; bi++) {
      tmp2 = deciles_highest_to_lowest[bi];
      deciles_highest_to_lowest[bi] = tmp1;
      tmp1 = tmp2;
    }
  }

  /* sum up number of entry positions in each decile */
  WITH_DEBUG(fprintf(stderr, "Sum up entry positions\n"));
  for (bi = 0; bi < bm; bi++) {
    decile_start_left = diagpos;
    decile_start_right = diagpos;
    decile_start_left_rounded = (int)floor(diagpos + .5);
    decile_start_right_rounded = decile_start_left_rounded;

    WITH_DEBUG(if (bi % 1024 == 0) fputc('.',stderr));
    for (i = 0; i < 10; i++) {
      entries_per_decile_prev = entries_per_decile[i];
      amt_added = 0;

      /* compute decile boundaries */
      decile_end_left = decile_start_left - dim_div_10;
      decile_end_right = decile_start_right + dim_div_10;

      /* round to determine which entries are in this decile */
      decile_end_left_rounded = (int)floor(decile_end_left + .5);
      decile_end_right_rounded = (int)floor(decile_end_right + .5);

      /* bounds checks */
      if (decile_end_left_rounded < 0)
        decile_end_left_rounded = 0;

      if (decile_end_right_rounded > bn-1)
        decile_end_right_rounded = bn-1;

      /* add number of entries enclosed by these bounds to nnzb_per_decile */
      if (decile_start_left_rounded > 0) {
        entries_per_decile[i] += decile_start_left_rounded - decile_end_left_rounded + 1;
        amt_added += decile_start_left_rounded - decile_end_left_rounded + 1;
      }

      if (decile_start_right_rounded < bn-1) {
        entries_per_decile[i] += decile_end_right_rounded - decile_start_right_rounded + 1;
        amt_added += decile_end_right_rounded - decile_start_right_rounded + 1;
      }

      /* correction for first decile since both its ends hit the diagonal */
      if (i == 0 && decile_start_left_rounded > 0 && decile_start_right_rounded < bn-1) {
        entries_per_decile[i]--;
        amt_added--;
      }

      /* have we overflowed? if so, employ the extra space that's been set out for
         overflow handling */
      if (entries_per_decile[i] < 0) {
        overflow_guard[i] += entries_per_decile_prev;
        entries_per_decile[i] = amt_added;
      }

      /* the endpoint of this decile becomes the starting point of the next one */
      decile_start_left = decile_end_left;
      decile_start_left_rounded = decile_end_left_rounded - 1;
      decile_start_right = decile_end_right;
      decile_start_right_rounded = decile_end_right_rounded + 1;
    }

    diagpos += ((double)bn)/((double)bm);
  }
  WITH_DEBUG(fputc('\n',stderr));

  /* now place entries in their appropriate deciles. this loop looks a lot like the last
     one, except that the code for adding the entries is also in place. */
  WITH_DEBUG(fprintf(stderr, "Place entries in deciles\n"));
  diagpos = 0.0;

  for (bi = 0; bi < bm; bi++) {
    row_elem_add_ct = 0;
    nnzb_to_add_this_row = 0;
    decile_start_left = diagpos;
    decile_start_right = diagpos;
    decile_start_left_rounded = (int)floor(diagpos + .5);
    decile_start_right_rounded = decile_start_left_rounded;

    WITH_DEBUG(if (bi % 1024 == 0) fputc('.',stderr));
    for (i = 0; i < 10; i++) {
      entries_per_decile_per_row = 0;

      /* compute decile boundaries */
      decile_end_left = decile_start_left - dim_div_10;
      decile_end_right = decile_start_right + dim_div_10;

      /* round to determine which entries are in this decile */
      decile_end_left_rounded = (int)floor(decile_end_left + .5);
      decile_end_right_rounded = (int)floor(decile_end_right + .5);

      /* bounds checks */
      if (decile_end_left_rounded < 0)
        decile_end_left_rounded = 0;

      if (decile_end_right_rounded > bn-1)
        decile_end_right_rounded = bn-1;

      /* add number of entries enclosed by these bounds to nnzb_per_decile */
      if (decile_start_left_rounded > 0)
        entries_per_decile_per_row += decile_start_left_rounded - decile_end_left_rounded + 1;

      if (decile_start_right_rounded < bn-1)
        entries_per_decile_per_row += decile_end_right_rounded - decile_start_right_rounded + 1;

      /* correction for first decile since both its ends hit the diagonal */
      if (i == 0 && decile_start_left_rounded > 0 && decile_start_right_rounded < bn-1)
        entries_per_decile_per_row--;

      decile_lengths[i] = entries_per_decile_per_row;

      /* how many nnzb to add to current (row,decile)? */
      nnzb_per_decile_this_row[i] = ((double)entries_per_decile_per_row)/((double)entries_per_decile[i]+overflow_guard[i])*nnzb*interval_fracs[i];
      nnzb_per_decile_this_row_rounded[i] = (int)floor(nnzb_per_decile_this_row[i] + .5);
      nnzb_to_add_this_row += nnzb_per_decile_this_row_rounded[i];

      /* the endpoint of this decile becomes the starting point of the next one */
      decile_start_left = decile_end_left;
      decile_start_left_rounded = decile_end_left_rounded - 1;
      decile_start_right = decile_end_right;
      decile_start_right_rounded = decile_end_right_rounded + 1;
    }

    /* now adjust nnzb per decile in this row accordingly to account for
       roundoff */
    if (nnzb_to_add_this_row > nnzb_per_block_row)
      nnzb_adjustment_idx = 9;
    else
      nnzb_adjustment_idx = 0;

    /* reset decile proportion counters */
    for (i = 0; i < 10; i++) {
      decile_proportion_counters[i] = 0;
      decile_rounded_proportion_counters[i] = 0;
    }

    while(nnzb_to_add_this_row != nnzb_per_block_row) {
      if (nnzb_to_add_this_row < nnzb_per_block_row
          && nnzb_per_decile_this_row[deciles_highest_to_lowest[nnzb_adjustment_idx]] > 0
          && nnzb_per_decile_this_row_rounded[deciles_highest_to_lowest[nnzb_adjustment_idx]] < decile_lengths[deciles_highest_to_lowest[nnzb_adjustment_idx]]) {
        decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] += interval_fracs[deciles_highest_to_lowest[nnzb_adjustment_idx]];

        if ((int)floor(decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] + .5) > decile_rounded_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]]) {
          nnzb_per_decile_this_row_rounded[deciles_highest_to_lowest[nnzb_adjustment_idx]]++;
          nnzb_to_add_this_row++;
          decile_rounded_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] = (int)floor(decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] + .5);
        }

        nnzb_adjustment_idx = (nnzb_adjustment_idx + 1) % 10;

        if (interval_fracs[deciles_highest_to_lowest[nnzb_adjustment_idx]] == 0)
          nnzb_adjustment_idx = 0;
      }
      else {
        if (nnzb_per_decile_this_row_rounded[deciles_highest_to_lowest[nnzb_adjustment_idx]] > 0) {
          decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] -= interval_fracs[deciles_highest_to_lowest[nnzb_adjustment_idx]];

          if ((int)floor(decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] + .5) < decile_rounded_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]]) {
            nnzb_per_decile_this_row_rounded[deciles_highest_to_lowest[nnzb_adjustment_idx]]--;
            nnzb_to_add_this_row--;
            decile_rounded_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] = (int)floor(decile_proportion_counters[deciles_highest_to_lowest[nnzb_adjustment_idx]] + .5);
          }
        }

        nnzb_adjustment_idx--;

        if (nnzb_adjustment_idx < 0) {
          nnzb_adjustment_idx = 9;

          while (interval_fracs[deciles_highest_to_lowest[nnzb_adjustment_idx]] == 0)
            nnzb_adjustment_idx--;
        }
      } 
    }

    /* reset decile boundaries */
    decile_start_left = diagpos;
    decile_start_right = diagpos;
    decile_start_left_rounded = (int)floor(diagpos + .5);
    decile_start_right_rounded = decile_start_left_rounded;

    /* now add elements to row */
    for (i = 0; i < 10; i++) {
      /* Key to local variables:
       *
       * num_added_without_duplicates:  
       *   Number of block entries that we have confirmed added to the block 
       *   row this far, in the sense that we have searched them for duplicates
       *   and found none.
       * num_iterations:
       *   Number of rounds that we have taken to try to add the requested 
       *   number of block entries to the current block row.  We take no more 
       *   than MAX_NUM_ITERATIONS tries to add the entries.  If we don't 
       *   succeed after that many iterations, we revert to a different, non-
       *   iterative algorithm for generating the matrix.
       */
      int num_added_without_duplicates = 0;
      int num_iterations = 0;

      /* compute decile boundaries */
      decile_end_left = decile_start_left - dim_div_10;
      decile_end_right = decile_start_right + dim_div_10;

      /* round to determine which entries are in this decile */
      decile_end_left_rounded = (int)floor(decile_end_left + .5);
      decile_end_right_rounded = (int)floor(decile_end_right + .5);

      /* bounds checks */
      if (decile_end_left_rounded < 0)
        decile_end_left_rounded = 0;

      if (decile_end_right_rounded > bn-1)
        decile_end_right_rounded = bn-1;

      /* compute decile length. we'll add entries within the interval
         [0,decile_length-1] first and adjust their values afterwards */
      decile_length = 0;

      if (decile_start_left_rounded > 0)
        decile_length += decile_start_left_rounded - decile_end_left_rounded + 1;

      if (decile_start_right_rounded < bn-1)
        decile_length += decile_end_right_rounded - decile_start_right_rounded + 1;

      if (i == 0 && decile_start_left_rounded > 0 && decile_start_right_rounded < bn-1)
        decile_length--;

      while (num_iterations < MAX_NUM_ITERATIONS && 
             num_added_without_duplicates < nnzb_per_decile_this_row_rounded[i])
        {
          /* Key to local variables:
           *
           * num_added_this_round_with_dups:
           *   Number of block entries that we have added _this_iteration_.
           *   Duplicate entries are counted.  We will remove the duplicate
           *   entries at the end of this iteration.
           */
          int num_added_this_round_with_dups = 0;
        
          WITH_DEBUG2(fprintf(stderr, "\tIteration %d, added %d thus far to row\n", 
                      num_iterations + 1, num_added_without_duplicates));
          /* 
           * While the total number of block entries added to this row 
           * (including the potential duplicates from this round) is less than
           * the number we are supposed to add, add another block entry.
           */
          while (num_added_without_duplicates + num_added_this_round_with_dups < nnzb_per_decile_this_row_rounded[i])
            {              
              /* add entry to [0,decile_length-1] */
              int bj = smvm_random_integer(0,decile_length-1);

              /* place this entry in its proper spot, given that the
                 current decile might be split */
              if (decile_start_left_rounded <= 0)
                /* no left decile */
                bj += decile_start_right_rounded;
              else if (decile_start_right_rounded >= bn-1)
                /* no right decile */
                bj += decile_end_left_rounded;
              else {
                decile_split_point = decile_start_left_rounded - decile_end_left_rounded;

                if (bj < decile_split_point)
                  bj += decile_end_left_rounded;
                else
                  bj += (decile_start_right_rounded - decile_split_point - 1);
              }

              current_row[num_added_without_duplicates + num_added_this_round_with_dups] = bj;
              num_added_this_round_with_dups++;
            }

          WITH_DEBUG2(fprintf(stderr, "\t-- Added %d this round with potential dups\n",
                      num_added_this_round_with_dups));

          /* Remove duplicate entries in this row */
          num_added_without_duplicates = 
            remove_duplicates(current_row, num_added_without_duplicates + 
                              num_added_this_round_with_dups);

          num_iterations++;
        }

      if (num_added_without_duplicates < nnzb_per_decile_this_row_rounded[i])
        {
          WITH_DEBUG2(fprintf(stderr, "*** __create_random_matrix_banded_by_statistics: at blo"
                      "ck row %d, added only %d nonzero blocks instead of at least %d: "
                      "reverting to \"deterministic\" algorithm ***\n", 
                      bi, num_added_without_duplicates, nnzb_per_block_row));
          smvm_free(colinds_to_add);
          smvm_free(entries_per_decile);
          smvm_free(current_row);
          destroy_bcsr_matrix(A);

          /* will at some point need to write deterministic version of generator */
          die_with_message("need to use deterministic algorithm",8);
        }
      else 
        {
          int k;
          /* We succeeded in adding all the entries which we wanted to add to 
           * the current row, so now we can copy the entries into the matrix. */
          A->rowptr[bi] = nnzb_per_block_row * bi;
          for (k = 0; k < nnzb_per_decile_this_row_rounded[i]; k++) {
            colinds_to_add[row_elem_add_ct] = current_row[k];
            row_elem_add_ct++;
          }
        }

      /* the endpoint of this decile becomes the starting point of the next one */
      decile_start_left = decile_end_left;
      decile_start_left_rounded = decile_end_left_rounded - 1;
      decile_start_right = decile_end_right;
      decile_start_right_rounded = decile_end_right_rounded + 1;
    }

    diagpos += ((double)bn)/((double)bm);

    /* sort column indices of this row */
    qsort(colinds_to_add,nnzb_per_block_row,sizeof(int),compare_ints);

    /* add column indices to matrix */
    for (idx = 0; idx < nnzb_per_block_row; idx++)
      A->colind[nnzb_per_block_row*bi + idx] = colinds_to_add[idx];
  }
  WITH_DEBUG(fputc('\n',stderr));

  /* ??? should it be nnzb or nnz? */
  A->rowptr[bm] = nnzb;

  /* Fill in the values array with random doubles */
  WITH_DEBUG(fprintf(stderr, "Fill array with %d values\n", nnz));
  {
    double* vals = (double*) (A->values);
    int k;
    for (k = 0; k < nnz; k++)
      vals[k] = smvm_random_double(-1.0, 1.0);
  }

  /* cleanup */
  smvm_free(colinds_to_add);
  smvm_free(entries_per_decile);
  smvm_free(current_row);

  return A;
}

#if 0
static struct bcsr_matrix_t*
__create_random_matrix_banded_by_nnz_per_row (const int bm, const int bn, 
					      const int r, const int c, 
					      const int nnzb_per_block_row, 
					      const int lower_block_bandwidth, 
					      const int upper_block_bandwidth)
{
  struct bcoo_matrix_t* A = create_empty_bcoo_matrix (bm, bn, r, c, 0, 
						      UNSYMMETRIC, 0, PATTERN);
  struct bcsr_matrix_t* B = NULL;
  /* Just a guess for now */
  const int MAX_NUM_ITERATIONS = 2 * nnzb_per_block_row * nnzb_per_block_row; 
  int current_block_row = 0;

  WITH_DEBUG(fprintf(stderr, "=== __create_random_matrix_banded_by_nnz_per"
	"_row ===\n"));

  /* 
   * For each row, we add nnzb_per_block_row entries to that row.  This process
   * doesn't check for duplicates, so afterwards we have to go back and remove
   * duplicates.  Once we have removed duplicates, we need to add more entries,
   * remove duplicates again, and so on, until we have enough entries in the row.  
   * We only continue this process a fixed number (MAX_NUM_ITERATIONS) of times
   * per row; if that isn't enough, we give up with an error message.  After 
   * finishing with that row, we move on to the next one.
   */

  for (current_block_row = 0; current_block_row < bm; current_block_row++)
    {
      /* Key to local variables:
       *
       * num_added_without_duplicates:  
       *   Number of block entries that we have added to this block row thus 
       *   far (no duplicates). 
       * num_iterations:
       *   Number of rounds that we have taken to try to add the requested 
       *   number of block entries to the current block row.  We take no more 
       *   than MAX_NUM_ITERATIONS tries to add the entries.
       * start_idx:
       *   The current block row starts at this index in the COO-format matrix.
       */

      const int start_idx = A->nnzb;
      int num_added_without_duplicates = 0;
      int num_iterations = 0;

      WITH_DEBUG2(fprintf(stderr, "Block row %d\n", current_block_row));

      while (num_iterations < MAX_NUM_ITERATIONS && num_added_without_duplicates < nnzb_per_block_row)
	{
	  /* Key to local variables:
	   *
	   * num_added_this_round_with_dups:
	   *   Number of block entries that we have added _this_iteration_.
	   *   Duplicate entries are counted.  We will remove the duplicate
	   *   entries at the end of this iteration.
	   */
	  int num_added_this_round_with_dups = 0;

	  WITH_DEBUG2(fprintf(stderr, "\tIteration %d, added %d thus far to row\n", 
				  num_iterations + 1, num_added_without_duplicates));
	  /* 
	   * While the total number of block entries added to this row 
	   * (including the potential duplicates from this round) is less than
	   * the number we are supposed to add, add another block entry.
	   */
	  while (num_added_without_duplicates + num_added_this_round_with_dups < nnzb_per_block_row)
	    {
              /* Constrain the column index so that the block entry fits 
	       * within the block bandwidth. */
	      const int bj = smvm_random_integer (MAX(0, current_block_row - lower_block_bandwidth), 
						  MIN(bn - 1, current_block_row + upper_block_bandwidth));
              WITH_DEBUG2(fprintf(stderr, "\t\tAdding block entry (%d,%d)\n",
                   current_block_row, bj));
	      bcoo_matrix_add_entry (A, current_block_row, bj, NULL);
	      num_added_this_round_with_dups++;
	    }
          WITH_DEBUG2(fprintf(stderr, "\t-- Added %d this round with potential dups\n",
               num_added_this_round_with_dups));

	  /* Remove duplicate entries in this row */

	  /* To remove duplicates, first we sort the index arrays */
	  sort_joint_arrays (A->II + start_idx, A->JJ + start_idx, 
			     num_added_without_duplicates + num_added_this_round_with_dups, 
			     sizeof(int), 
			     compare_lexicographically);

	  if (smvm_debug_level() > 1)
            {
	      int iii;
              fprintf (stderr, "\t-- Current col indices (after sorting, before duplicate removal):");
	      for (iii = start_idx; iii < num_added_without_duplicates + num_added_this_round_with_dups; iii++)
                fprintf (stderr, " %d", A->JJ[iii]);
	      fprintf (stderr, "\n");
	    }
	  /* 
	   * Now we step through the index arrays, moving non-duplicates to 
	   * the beginning of the current row.  All the duplicate entries will 
	   * be at the end, and then we will readjust the lengths of the arrays.
	   */
	  if (num_added_this_round_with_dups > 0)
	    {
	      int curidx = start_idx;
	      int i = start_idx + 1;
	      int num_removed = 0;

	      while (i < start_idx + num_added_without_duplicates + num_added_this_round_with_dups)
		{
		  if (A->JJ[i] != A->JJ[curidx] || A->II[i] != A->II[curidx])
		    curidx++;
		  else 
		    num_removed++;

		  A->II[curidx] = A->II[i];
		  A->JJ[curidx] = A->JJ[i];
		  i++;
		}

	      /* We've reduced the number of nonzero entries by removing duplicates, 
	       * so update nnzb. */
              WITH_DEBUG2(fprintf(stderr, "\t-- Removed %d duplicates\n", num_removed));
	      num_added_without_duplicates += num_added_this_round_with_dups - num_removed;
	      WITH_DEBUG2(fprintf(stderr, "\t-- Incremented total num added to row by %d\n", 
				 num_added_this_round_with_dups - num_removed));
	      A->nnzb = start_idx + num_added_without_duplicates;
	      WITH_DEBUG2(fprintf(stderr, "\t-- Now nnzb = %d, added %d thus far to row\n", 
				 A->nnzb, num_added_without_duplicates));
	    }
	  num_iterations++;
	}

      if (num_added_without_duplicates < nnzb_per_block_row)
	{
	  WITH_DEBUG2(fprintf (stderr, "*** __create_random_matrix_banded_by_nnz_per_row: at blo"
			       "ck row %d, added only %d nonzero blocks instead of at least %d: "
			       "reverting to \"deterministic\" algorithm ***\n", 
			       current_block_row, num_added_without_duplicates, nnzb_per_block_row));
	  destroy_bcoo_matrix (A);
	  return
	    __create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows
	    (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth,
	     upper_block_bandwidth);
	}
    }

  B = bcoo_matrix_to_random_bcsr_matrix (A);
  destroy_bcoo_matrix (A);
  return B;
}
#endif


/** 
 * @brief Creates a random block compressed sparse row matrix.
 *
 * Creates a random block compressed sparse row (BCSR) matrix with fixed block 
 * dimensions bm x bn.  The actual dimensions of the matrix are r*bm x c*bn.
 * The matrix contains nnzb dense blocks of nonzeros.  Each of these blocks has
 * the dimensions r x c.  Blocks are aligned so that if (i,j) are the indices 
 * of the upper left corner of a block, then i+1 divides r and j+1 divides c.  
 * This particular function does no verification of the inputs.
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
static struct bcsr_matrix_t*
__create_random_matrix (const int bm, const int bn, 
			const int r, const int c, 
			const int nnzb)
{
  const int bw = MAX(bm, bn);
  return __create_random_matrix_banded (bm, bn, r, c, nnzb, bw, bw);
}


struct bcsr_matrix_t*
create_random_matrix (const int bm, const int bn, const int r, 
		      const int c, const int nnzb)
{
  WITH_DEBUG2( fprintf (stderr, "=== create_random_matrix ===\n") );

  if (bm <= 0)
    {
      fprintf (stderr, "*** create_random_matrix: bm=%d <= 0 ***\n", bm);
      return NULL;
    }
  else if (bn <= 0)
    {
      fprintf (stderr, "*** create_random_matrix: bn=%d <= 0 ***\n", bn);
      return NULL;
    }
  else if (r <= 0)
    {
      fprintf (stderr, "*** create_random_matrix: r=%d <= 0 ***\n", r);
      return NULL;
    }
  else if (c <= 0)
    {
      fprintf (stderr, "*** create_random_matrix: c=%d <= 0 ***\n", c);
      return NULL;
    }
  else if (nnzb < 0)
    {
      fprintf (stderr, "*** create_random_matrix: nnzb=%d < 0 ***\n", nnzb);
      return NULL;
    }
  /* use doubles to avoid overflowing int */
  else if ((double) nnzb * (double) r * (double) c > 
	   (double) bm * (double) bn * (double) r * (double) c)
    {
      fprintf (stderr, "*** create_random_matrix: you said that the "
	       "matrix holds more nonzeros than it can possibly hold, given "
	       "the dimensions of the matrix ***\n");
      return NULL;
    }

  return __create_random_matrix (bm, bn, r, c, nnzb);
}


struct bcsr_matrix_t*
create_random_matrix_banded (const int bm, const int bn, 
			     const int r, const int c, const int nnzb, 
			     const int lower_block_bandwidth, 
			     const int upper_block_bandwidth)
{
  WITH_DEBUG2( fprintf (stderr, "=== create_random_matrix_banded ===\n") );

  if (bm <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: bm <= 0 ***\n");
      return NULL;
    }
  else if (bn <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: bn <= 0 ***\n");
      return NULL;
    }
  else if (r <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: r <= 0 ***\n");
      return NULL;
    }
  else if (c <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: c <= 0 ***\n");
      return NULL;
    }
  else if (nnzb < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: nnzb < 0 ***\n");
      return NULL;
    }
  /* use doubles to avoid overflowing int */
  else if ((double) nnzb * (double) r * (double) c > 
	   (double) bm * (double) bn * (double) r * (double) c)
    {
      fprintf (stderr, "*** create_random_matrix_banded: you said that the "
	       "matrix holds more nonzeros than it can possibly hold, given "
	       "the dimensions of the matrix ***\n");
      return NULL;
    }
  else if (lower_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: lower_block_bandwidth < 0 ***\n");
      return NULL;
    }
  else if (upper_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded: upper_block_bandwidth < 0 ***\n");
      return NULL;
    }

  return __create_random_matrix_banded (bm, bn, r, c, nnzb, 
					lower_block_bandwidth, upper_block_bandwidth);
}


struct bcsr_matrix_t*
create_random_matrix_banded_by_nnz_per_row (const int bm, const int bn, 
					    const int r, const int c, 
					    const int nnzb_per_block_row, 
					    const int lower_block_bandwidth, 
					    const int upper_block_bandwidth)
{
  double row_density = 0.0;

  WITH_DEBUG2( fprintf (stderr, "=== create_random_matrix_banded_by_nnz_per_row ===\n") );

  if (bm <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: bm <= 0 ***\n");
      return NULL;
    }
  else if (bn <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: bn <= 0 ***\n");
      return NULL;
    }
  else if (r <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: r <= 0 ***\n");
      return NULL;
    }
  else if (c <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: c <= 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: nnzb_per_block_row < 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row > bn)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: nnzb_per_block_row > bn ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row > lower_block_bandwidth + upper_block_bandwidth + 1)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: nnzb_"
	       "per_block_row > lower_block_bandwidth + upper_block_bandwidth"
	       " + 1 ***\n");
      return NULL;
    }
  else if (lower_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: lower_block_bandwidth < 0 ***\n");
      return NULL;
    }
  else if (upper_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: upper_block_bandwidth < 0 ***\n");
      return NULL;
    }

  row_density = ((double) nnzb_per_block_row) / ((double) bn);

  if (row_density > 1.0/3.0)
    return __create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);
  else
    /* return __create_random_matrix_banded_by_nnz_per_row (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth); */
    return __create_random_matrix_banded_by_nnz_per_row2 (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);
}

/* added by hormozd 5/4/2006
 * wrapper function for statistics-based matrix generator */
struct bcsr_matrix_t*
create_random_matrix_banded_by_statistics (const int bm, const int bn, 
					   const int r, const int c, 
					   const int nnzb_per_block_row, 
					   const double *interval_fracs)
{
  /* double row_density = 0.0; */

  WITH_DEBUG2( fprintf (stderr, "=== create_random_matrix_banded_by_nnz_per_row ===\n") );

  if (bm <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: bm <= 0 ***\n");
      return NULL;
    }
  else if (bn <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: bn <= 0 ***\n");
      return NULL;
    }
  else if (r <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: r <= 0 ***\n");
      return NULL;
    }
  else if (c <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: c <= 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: nnzb_per_block_row < 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row > bn)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row: nnzb_per_block_row > bn ***\n");
      return NULL;
    }

  /* row_density = ((double) nnzb_per_block_row) / ((double) bn); */

  /* deterministic algorithm will go here */
  /*  if (row_density > 1.0/3.0)
    return __create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);
    else */
    return __create_random_matrix_banded_by_statistics (bm, bn, r, c, nnzb_per_block_row, interval_fracs);
}

struct bcsr_matrix_t*
create_random_matrix_banded_by_nnz_per_row_using_algorithm (const int bm, const int bn, 
							    const int r, const int c, 
							    const int nnzb_per_block_row, 
							    const int lower_block_bandwidth, 
							    const int upper_block_bandwidth,
							    const int algorithm)
{
  if (algorithm < 0 || algorithm > 2)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using_"
	       "algorithm: algorithm parameter is out of range: valid values "
	       "are 0, 1, 2 ***\n");
      return NULL;
    }

  if (algorithm == 0)
    return create_random_matrix_banded_by_nnz_per_row (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);

  if (bm <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: bm <= 0 ***\n");
      return NULL;
    }
  else if (bn <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: bn <= 0 ***\n");
      return NULL;
    }
  else if (r <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: r <= 0 ***\n");
      return NULL;
    }
  else if (c <= 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: c <= 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: nnzb_per_block_row < 0 ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row > bn)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using_"
	       "algorithm: nnzb_per_block_row > bn ***\n");
      return NULL;
    }
  else if (nnzb_per_block_row > lower_block_bandwidth + upper_block_bandwidth + 1)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using_"
	       "algorithm: nnzb_per_block_row > lower_block_bandwidth + upper"
	       "_block_bandwidth + 1 ***\n");
      return NULL;
    }
  else if (lower_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: lower_block_bandwidth < 0 ***\n");
      return NULL;
    }
  else if (upper_block_bandwidth < 0)
    {
      fprintf (stderr, "*** create_random_matrix_banded_by_nnz_per_row_using"
	       "_algorithm: upper_block_bandwidth < 0 ***\n");
      return NULL;
    }

  if (algorithm == 1)
    return __create_random_matrix_banded_by_nnz_per_row2 (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);
  else if (algorithm == 2)
    return __create_random_matrix_banded_by_nnz_per_row__nearly_dense_rows (bm, bn, r, c, nnzb_per_block_row, lower_block_bandwidth, upper_block_bandwidth);
  else 
    {
      fprintf (stderr, "*** You should not be here! ***\n");
      assert (0);
      return NULL; /* dummy for compiler */
    }
}


