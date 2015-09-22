/**
 * @file bcsr_matrix.c
 * @author Mark Hoemmen
 * @date 23 Feb 2006
 * @since June 2005
 *
 * Implementation of bcsr_matrix_t member functions.
 *
 * Copyright (c) 2006, Regents of the University of California 
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the
 * following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in 
 *   the documentation and/or other materials provided with the 
 *   distribution.
 *
 * * Neither the name of the University of California, Berkeley, nor
 *   the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior
 *   written permission.  
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <bcsr_matrix.h>

#include <__complex.h>
#include <smvm_malloc.h>
#include <smvm_util.h>
/*#include <sort_joint_arrays.h>*/

#include <stdio.h>
#include <string.h>

/*======================================================================*/
void
init_bcsr_matrix (struct bcsr_matrix_t* A, const int bm, const int bn, 
		  const int r, const int c, const int nnzb, void* values, 
		  int* colind, int* rowptr, const enum symmetry_type_t symmetry_type,
		  const enum symmetric_storage_location_t symmetric_storage_location,
		  const enum value_type_t value_type, const int col_oriented_p)
{
  A->bm = bm;
  A->bn = bn;
  A->r = r;
  A->c = c;
  A->nnzb = nnzb;
  A->values = values;
  A->colind = colind;
  A->rowptr = rowptr;
  A->symmetry_type = symmetry_type;
  A->symmetric_storage_location = symmetric_storage_location;
  A->value_type = value_type;
  A->col_oriented_p = col_oriented_p;
}


/*======================================================================*/
void
unpack_bcsr_matrix (struct bcsr_matrix_t* A, int* bm, int* bn, int* r, int* c,
		    int* nnzb, void** values, int** colind, int** rowptr,
		    enum symmetry_type_t* symmetry_type,
		    enum symmetric_storage_location_t* symmetric_storage_location,
		    enum value_type_t* value_type,
		    int* col_oriented_p)
{
  *bm = A->bm;
  *bn = A->bn;
  *r = A->r;
  *c = A->c;
  *nnzb = A->nnzb;
  *values = A->values;
  *colind = A->colind;
  *rowptr = A->rowptr;
  *symmetry_type = A->symmetry_type;
  *symmetric_storage_location = A->symmetric_storage_location;
  *value_type = A->value_type;
  *col_oriented_p = A->col_oriented_p;
}





/*======================================================================*/
struct bcsr_matrix_t* 
create_bcsr_matrix_handle ()
{
  struct bcsr_matrix_t* A = smvm_calloc (sizeof (struct bcsr_matrix_t), 1);

  return A;
}


/*======================================================================*/
void
destroy_bcsr_matrix_handle (struct bcsr_matrix_t* A)
{
  smvm_free (A);
}


/*======================================================================*/
struct bcsr_matrix_t*
create_bcsr_matrix (const int bm, const int bn, const int r, const int c, 
		    const int nnzb, void* values, int* colind, int* rowptr,
		    const enum symmetry_type_t symmetry_type,
		    const enum symmetric_storage_location_t symmetric_storage_location, 
		    const enum value_type_t value_type,
		    const int col_oriented_p)
{
  struct bcsr_matrix_t* A = smvm_malloc (sizeof (struct bcsr_matrix_t));
  init_bcsr_matrix (A, bm, bn, r, c, nnzb, values, colind, rowptr, 
		    symmetry_type, symmetric_storage_location, value_type,
		    col_oriented_p);
  return A;
}


/*======================================================================*/
void
destroy_bcsr_matrix (struct bcsr_matrix_t* A)
{
  smvm_free (A->values);
  smvm_free (A->colind);
  smvm_free (A->rowptr);

  smvm_free (A);
}


/*======================================================================*/
struct bcsr_matrix_t*
clone_bcsr_matrix (struct bcsr_matrix_t* A)
{
  const int bm = A->bm;
  const int bn = A->bn;
  const int r = A->r;
  const int c = A->c;
  const int nnzb = A->nnzb;
  const int nnz = nnzb * r * c;
  void* values = NULL;
  int* colind = NULL;
  int* rowptr = NULL;

  if (A->value_type == REAL)
    {
      double* v = smvm_malloc (nnz * sizeof (double));
      memcpy (v, A->values, nnz * sizeof (double));
      values = (void*) v;
    } 
  else if (A->value_type == COMPLEX)
    {
      double_Complex* v = smvm_malloc (nnz * sizeof (double_Complex));
      memcpy (v, A->values, nnz * sizeof (double_Complex));
      values = (void*) v;
    } 
  else if (A->value_type == PATTERN)
    values = NULL;

  colind = smvm_malloc (nnzb * sizeof (int));
  memcpy (colind, A->colind, nnzb * sizeof (int));

  rowptr = smvm_malloc ((bm + 1) * sizeof (int));
  memcpy (rowptr, A->rowptr, (bm + 1) * sizeof (int));

  return create_bcsr_matrix (bm, bn, r, c, nnzb, values, colind, rowptr,
			     A->symmetry_type, A->symmetric_storage_location, 
			     A->value_type, A->col_oriented_p);
}


/*======================================================================*/
void 
dump_bcsr_matrix (FILE* out, const struct bcsr_matrix_t* A)
{
  int i;

  fprintf (out, "\nBCSR matrix dump:\n");
  fprintf (out, "bm, bn = %d, %d\n", A->bm, A->bn);
  fprintf (out, "r, c = %d, %d\n", A->r, A->c);
  if (A->col_oriented_p)
    fprintf (out, "Nonzero blocks are column-oriented\n");
  else
    fprintf (out, "Nonzero blocks are row-oriented\n");

  fprintf (out, "rowptr[0:%d]:", A->bm);
  for (i = 0; i <= A->bm; i++)
    fprintf (out, " %d", A->rowptr[i]);

  fprintf (out, "\ncolind[0:%d]:", A->nnzb - 1);
  for (i = 0; i < A->nnzb; i++)
    fprintf (out, " %d", A->colind[i]);

  fprintf (out, "\nvalues[0:%d]:", A->nnzb * A->r * A->c - 1);
  if (A->value_type == REAL)
    {
      double* values = (double*) (A->values);

      for (i = 0; i < A->nnzb * A->r * A->c; i++)
	fprintf (out, " %g", values[i]);
    }
  else if (A->value_type == COMPLEX)
    {
      double_Complex* values = (double_Complex*) (A->values);

      for (i = 0; i < A->nnzb * A->r * A->c; i++)
	fprintf (out, " (%g, %g)", double_Complex_real_part(values[i]), 
		 double_Complex_imag_part(values[i]));
    }

  fprintf (out, "\n\n");
}


/*======================================================================*/
int
valid_bcsr_matrix_p (const struct bcsr_matrix_t* A)
{
  const int nnzb           = A->nnzb;
  const int bm             = A->bm;
  const int bn             = A->bn;
  const int r              = A->r;
  const int c              = A->c;
  const int n              = bn * c;
  int i = 0;

  if (nnzb < 0)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: nnzb = %d < 0 ***\n", nnzb);
      return 0;
    }
  else if (bm < 1)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: bm = %d < 1 ***\n", bm);
      return 0;
    }
  else if (bn < 1)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: bn = %d < 1 ***\n", bn);
      return 0;
    }
  else if (r < 1)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: r = %d < 1 ***\n", r);
      return 0;
    }
  else if (c < 1)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: c = %d < 1 ***\n", c);
      return 0;
    }
  else if (nnzb > 0 && (A->value_type == REAL || A->value_type == COMPLEX) && A->values == NULL)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: A is not a pattern matrix "
	       "and has nonzero values, yet A->values is NULL! ***\n");
      return 0;
    }

  /* Range-check all the rowptr indices */
  for (i = 0; i < bm; i++)
    {
      const int start = A->rowptr[i];
      const int end   = A->rowptr[i+1];

      if (start < 0 || start > nnzb)
	{
	  fprintf (stderr, "*** valid_bcsr_matrix_p: At block row i=%d, "
		   "rowptr[i] = %d is out of valid range [%d,%d] ***\n", 
		   i, start, 0, nnzb);
	  dump_bcsr_matrix (stderr, A);
	  return 0;
	}
      else if (start > end)
	{
	  fprintf (stderr, "*** valid_bcsr_matrix_p: At block row i=%d, "
		   "rowptr[i] = %d > rowptr[i+1] = %d ***\n", i, start, end);
	  dump_bcsr_matrix (stderr, A);
	  return 0;
	}
    }
  /* The last entry of rowptr should be nnzb */
  if (A->rowptr[bm] != nnzb)
    {
      fprintf (stderr, "*** valid_bcsr_matrix_p: rowptr[bm = %d] = %d != nnzb = "
	       "%d ***\n", bm, A->rowptr[bm], nnzb);
      dump_bcsr_matrix (stderr, A);
      return 0;
    }

  /* Range-check all the colind indices */
  for (i = 0; i < nnzb; i++)
    {
      const int j = A->colind[i];

      if (j < 0 || j >= n)
	{
	  fprintf (stderr, "*** valid_bcsr_matrix_p: colind[%d] = %d is out "
		   "of range ***\n", i, j);
	  dump_bcsr_matrix (stderr, A);
	  return 0;
	}
      /* All entries in colind are divisible by c, since they are the 
       * indices of the upper left corners of nonzero blocks, and the 
       * nonzero blocks are aligned */
      else if (j % c != 0)
	{
	  fprintf (stderr, "*** valid_bcsr_matrix_p: colind[%d] = %d is not "
		   "divisible by c = %d ***\n", i, j, c);
	  dump_bcsr_matrix (stderr, A);
	  return 0;
	}
    }

  return 1;
}


/*======================================================================*/
int
print_bcsr_matrix_in_matrix_market_format (FILE* out, const struct bcsr_matrix_t* A)
{
  int start, end;
  int i, j, k;
  const int col_oriented_p = A->col_oriented_p;
  const int nnzb           = A->nnzb;
  const int bm             = A->bm;
  const int bn             = A->bn;
  const int r              = A->r;
  const int c              = A->c;
  const int block_size     = r * c;
  const int nnz            = nnzb * block_size;
  const int OUTPUT_INDEX_BASE = 1;
  char symmetry_type_label[20];
  char value_type_label[20];

  WITH_DEBUG2(fprintf(stderr, "=== print_bcsr_matrix_in_matrix_market_format ===\n"));

  if (A->symmetry_type == UNSYMMETRIC)
    strncpy (symmetry_type_label, "general", 19);
  else if (A->symmetry_type == SYMMETRIC)
    strncpy (symmetry_type_label, "symmetric", 19);
  else if (A->symmetry_type == SKEW_SYMMETRIC)
    strncpy (symmetry_type_label, "skew-symmetric", 19);
  else if (A->symmetry_type == HERMITIAN)
    strncpy (symmetry_type_label, "hermitian", 19);
  else 
    {
      fprintf (stderr, "*** print_bcsr_matrix_in_matrix_market_format: "
	       "Invalid symmetry type %d of the given bcsr_matrix_t sparse "
	       "matrix! ***\n", A->symmetry_type);
      WITH_DEBUG2(fprintf(stderr, "=== Done with print_bcsr_matrix_in_matrix_market_format ===\n"));
      return -1;
    }

  if (A->value_type == REAL)
    strncpy (value_type_label, "real", 19);
  else if (A->value_type == COMPLEX)
    strncpy (value_type_label, "complex", 19);
  else if (A->value_type == PATTERN)
    strncpy (value_type_label, "pattern", 19);
  else
    {
      fprintf (stderr, "*** print_bcsr_matrix_in_matrix_market_format: Uns"
	       "upported value type! ***\n");
      WITH_DEBUG2(fprintf(stderr, "=== Done with print_bcsr_matrix_in_matr"
			  "ix_market_format ===\n"));
      return -1;
    }

  fprintf (out, "%%%%MatrixMarket matrix coordinate %s %s\n", value_type_label, symmetry_type_label);
  fprintf (out, "%d %d %d\n", bm * r, bn * c, nnz);

  if (block_size == 1)
    {
      for (i = 0; i < bm; i++)
	{
	  start = A->rowptr[i];
	  end   = A->rowptr[i+1];

	  for (j = start; j < end; j++)
	    {
	      if (A->value_type == REAL)
		{
		  double* values = (double*) (A->values);
		  fprintf (out, "%d %d %.13e\n", 
			   OUTPUT_INDEX_BASE + i*r,
			   OUTPUT_INDEX_BASE + A->colind[j],
			   values[j * block_size]);
		}
	      else if (A->value_type == COMPLEX)
		{
		  double_Complex* values = (double_Complex*) (A->values);
		  fprintf (out, "%d %d %.13e %.13e\n", 
			   OUTPUT_INDEX_BASE + i*r,
			   OUTPUT_INDEX_BASE + A->colind[j],
			   double_Complex_real_part (values[j * block_size]),
			   double_Complex_imag_part (values[j * block_size]));
		}
	      else if (A->value_type == PATTERN)
		{
		  fprintf (out, "%d %d\n", 
			   OUTPUT_INDEX_BASE + i*r,
			   OUTPUT_INDEX_BASE + A->colind[j]);
		}
	    }
	}
    }
  else if (col_oriented_p)
    {
      for (i = 0; i < A->bm; i++)
	{
	  start = A->rowptr[i];
	  end   = A->rowptr[i+1];

	  for (j = start; j < end; j++)
	    {
	      for (k = 0; k < block_size; k++) /* k goes down columns */
		{
		  if (A->value_type == REAL)
		    {
		      double* values = (double*) (A->values);
		      fprintf (out, "%d %d %.13e\n", 
			       OUTPUT_INDEX_BASE + i*r + (k % r), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k / r), 
			       values[j*block_size + k]);
		    }
		  else if (A->value_type == COMPLEX)
		    {
		      double_Complex* values = (double_Complex*) (A->values);
		      fprintf (out, "%d %d %.13e %.13e\n", 
			       OUTPUT_INDEX_BASE + i*r + (k % r), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k / r), 
			       double_Complex_real_part (values[j*block_size + k]),
			       double_Complex_imag_part (values[j*block_size + k]));
		    }
		  else if (A->value_type == PATTERN)
		    {
		      fprintf (out, "%d %d\n", 
			       OUTPUT_INDEX_BASE + i*r + (k % r), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k / r));
		    }
		}
	    }
	}
    }
  else /* Nonzero blocks in the values array are row-oriented */
    {
      for (i = 0; i < A->bm; i++)
	{
	  start = A->rowptr[i];
	  end   = A->rowptr[i+1];

	  for (j = start; j < end; j++)
	    {
	      for (k = 0; k < r*c; k++)  /* k goes across rows */
		{
		  if (A->value_type == REAL)
		    {
		      double* values = (double*) (A->values);
		      fprintf (out, "%d %d %.13e\n", 
			       OUTPUT_INDEX_BASE + i*r + (k / c), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k % c), 
			       values[j*block_size + k]);
		    }
		  else if (A->value_type == COMPLEX)
		    {
		      double_Complex* values = (double_Complex*) (A->values);
		      fprintf (out, "%d %d %.13e %.13e\n", 
			       OUTPUT_INDEX_BASE + i*r + (k / c), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k % c), 
			       double_Complex_real_part (values[j*block_size + k]),
			       double_Complex_imag_part (values[j*block_size + k]));
		    }
		  else if (A->value_type == PATTERN)
		    {
		      fprintf (out, "%d %d\n", 
			       OUTPUT_INDEX_BASE + i*r + (k / c), 
			       OUTPUT_INDEX_BASE + A->colind[j] + (k % c));
		    }
		}
	    }
	}
    }
  return 0;
}



#if 0
/*======================================================================*/
struct bcsr_matrix_t*
identity_bcsr_matrix (const int n)
{
  int i;
  struct bcsr_matrix_t* A = 
    (struct bcsr_matrix_t*) smvm_malloc (sizeof (struct bcsr_matrix_t));

  A->bm = n;
  A->bn = n;
  A->r  = 1;
  A->c  = 1;
  A->nnzb = n;
  A->col_oriented_p = 0;

  A->values = (double*) smvm_malloc (n * sizeof (double));
  A->colind = (int*) smvm_malloc (n * sizeof (int));
  A->rowptr = (int*) smvm_malloc ((n+1) * sizeof (int));

  for (i = 0; i < n; i++)
    {
      A->rowptr[i] = i;
      A->colind[i] = i;
      A->values[i] = 1.0;
    }
  A->rowptr[n] = n;

  return A;
}


/*======================================================================*/
struct bcsr_matrix_t*
extended_identity_bcsr_matrix (const int bm, const int bn, const int r, const int c)
{
  int i, k;
  struct bcsr_matrix_t* A = 
    (struct bcsr_matrix_t*) smvm_malloc (sizeof (struct bcsr_matrix_t));
  int nnzb = MIN( bm, bn );

  A->bm = bm;
  A->bn = bn;
  A->r  = r;
  A->c  = c;
  A->nnzb = nnzb;
  A->col_oriented_p = 0;

  A->values = (double*) smvm_malloc (nnzb * r * c * sizeof (double));
  A->colind = (int*) smvm_malloc (nnzb * sizeof (int));
  A->rowptr = (int*) smvm_malloc ((bm+1) * sizeof (int));

  for (i = 0; i < nnzb; i++)
    {
      A->rowptr[i] = i;
      A->colind[i] = i;
     
      for (k = 0; k < r*c; k++)
	A->values[r*c*i + k] = 1.0;
    }
  A->rowptr[bm] = A->nnzb;

  return A;
}
#endif /* 0 */



int 
save_bcsr_matrix_in_matrix_market_format (const char* const filename, 
					  struct bcsr_matrix_t* A)
{
  FILE* out = NULL;
  int errcode = 0;

  WITH_DEBUG2(fprintf(stderr, "=== save_bcsr_matrix_in_matrix_market_format ===\n")); 
  out = fopen (filename, "w");
  if (out == NULL)
    {
      fprintf (stderr, "*** save_bcsr_matrix_in_matrix_market_format: failed "
	       "to open output file \"%s\" ***\n", filename);
      return -1;
    }

  errcode = print_bcsr_matrix_in_matrix_market_format (out, A);
  if (0 != fclose (out))
    {
      fprintf (stderr, "*** save_bcsr_matrix_in_matrix_market_format: failed "
	       "to close output file \"%s\" ***\n", filename);
      return -1;
    }
  WITH_DEBUG2(fprintf(stderr, "=== Done with save_bcsr_matrix_in_matrix_market_format ===\n")); 
  return errcode;
}


