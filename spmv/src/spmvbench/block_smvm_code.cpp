#define BLOCK_SMVM_MAIN 1

#ifdef __cplusplus
extern "C" {
#endif

#include "block_smvm_code.h"

#ifdef __cplusplus
}
#endif

#include <algorithm> // min
using namespace std;

#include "lime.h"

//#define PARTIAL 10 // used to shorten run time for trace capture

tick_t t0, t1, t2, t3, t4, t5, t6, t7, t8;
unsigned long long tsetup, treorg, toper, tcache;

SMVM_FP bsmvm_routines[12][12][1] = {
{{bsmvm_1x1_1},{bsmvm_1x2_1},{bsmvm_1x3_1},{bsmvm_1x4_1},{bsmvm_1x5_1},{bsmvm_1x6_1},{bsmvm_1x7_1},{bsmvm_1x8_1},{bsmvm_1x9_1},{bsmvm_1x10_1},{bsmvm_1x11_1},{bsmvm_1x12_1}}
,{{bsmvm_2x1_1},{bsmvm_2x2_1},{bsmvm_2x3_1},{bsmvm_2x4_1},{bsmvm_2x5_1},{bsmvm_2x6_1},{bsmvm_2x7_1},{bsmvm_2x8_1},{bsmvm_2x9_1},{bsmvm_2x10_1},{bsmvm_2x11_1},{bsmvm_2x12_1}}
,{{bsmvm_3x1_1},{bsmvm_3x2_1},{bsmvm_3x3_1},{bsmvm_3x4_1},{bsmvm_3x5_1},{bsmvm_3x6_1},{bsmvm_3x7_1},{bsmvm_3x8_1},{bsmvm_3x9_1},{bsmvm_3x10_1},{bsmvm_3x11_1},{bsmvm_3x12_1}}
,{{bsmvm_4x1_1},{bsmvm_4x2_1},{bsmvm_4x3_1},{bsmvm_4x4_1},{bsmvm_4x5_1},{bsmvm_4x6_1},{bsmvm_4x7_1},{bsmvm_4x8_1},{bsmvm_4x9_1},{bsmvm_4x10_1},{bsmvm_4x11_1},{bsmvm_4x12_1}}
,{{bsmvm_5x1_1},{bsmvm_5x2_1},{bsmvm_5x3_1},{bsmvm_5x4_1},{bsmvm_5x5_1},{bsmvm_5x6_1},{bsmvm_5x7_1},{bsmvm_5x8_1},{bsmvm_5x9_1},{bsmvm_5x10_1},{bsmvm_5x11_1},{bsmvm_5x12_1}}
,{{bsmvm_6x1_1},{bsmvm_6x2_1},{bsmvm_6x3_1},{bsmvm_6x4_1},{bsmvm_6x5_1},{bsmvm_6x6_1},{bsmvm_6x7_1},{bsmvm_6x8_1},{bsmvm_6x9_1},{bsmvm_6x10_1},{bsmvm_6x11_1},{bsmvm_6x12_1}}
,{{bsmvm_7x1_1},{bsmvm_7x2_1},{bsmvm_7x3_1},{bsmvm_7x4_1},{bsmvm_7x5_1},{bsmvm_7x6_1},{bsmvm_7x7_1},{bsmvm_7x8_1},{bsmvm_7x9_1},{bsmvm_7x10_1},{bsmvm_7x11_1},{bsmvm_7x12_1}}
,{{bsmvm_8x1_1},{bsmvm_8x2_1},{bsmvm_8x3_1},{bsmvm_8x4_1},{bsmvm_8x5_1},{bsmvm_8x6_1},{bsmvm_8x7_1},{bsmvm_8x8_1},{bsmvm_8x9_1},{bsmvm_8x10_1},{bsmvm_8x11_1},{bsmvm_8x12_1}}
,{{bsmvm_9x1_1},{bsmvm_9x2_1},{bsmvm_9x3_1},{bsmvm_9x4_1},{bsmvm_9x5_1},{bsmvm_9x6_1},{bsmvm_9x7_1},{bsmvm_9x8_1},{bsmvm_9x9_1},{bsmvm_9x10_1},{bsmvm_9x11_1},{bsmvm_9x12_1}}
,{{bsmvm_10x1_1},{bsmvm_10x2_1},{bsmvm_10x3_1},{bsmvm_10x4_1},{bsmvm_10x5_1},{bsmvm_10x6_1},{bsmvm_10x7_1},{bsmvm_10x8_1},{bsmvm_10x9_1},{bsmvm_10x10_1},{bsmvm_10x11_1},{bsmvm_10x12_1}}
,{{bsmvm_11x1_1},{bsmvm_11x2_1},{bsmvm_11x3_1},{bsmvm_11x4_1},{bsmvm_11x5_1},{bsmvm_11x6_1},{bsmvm_11x7_1},{bsmvm_11x8_1},{bsmvm_11x9_1},{bsmvm_11x10_1},{bsmvm_11x11_1},{bsmvm_11x12_1}}
,{{bsmvm_12x1_1},{bsmvm_12x2_1},{bsmvm_12x3_1},{bsmvm_12x4_1},{bsmvm_12x5_1},{bsmvm_12x6_1},{bsmvm_12x7_1},{bsmvm_12x8_1},{bsmvm_12x9_1},{bsmvm_12x10_1},{bsmvm_12x11_1},{bsmvm_12x12_1}}
};


#if defined(USE_ACC)

#include "IndexArray.hpp"

#define MIN_COLS 2
#define restrict __restrict__

typedef int index_t; // int used in bcsr_matrix_t

extern IndexArray<index_t> dre; // Data Reorganization Engine
extern size_t block_sz;


void bsmvm_1x1_1 (const int start_row, /*const*/ int end_row, const int bm,
	const int row_start[], const int col_idx[], const double value[], 
	const double src[], double dest[])
{
	int i, j;
	int col_len = row_start[end_row+1];
	int rows_to_batch;
	int max_block_size=block_sz/sizeof(double);
	double * restrict block = SP_NALLOC(double, block_sz/sizeof(double)); // view block
#if defined(PARTIAL)
	end_row /= PARTIAL;
#endif

	tget(t0);
	// receive block: not needed when block has been invalidated before entry
	// Xil_L1DCacheInvalidateRange((INTPTR)block, block_sz);
	tget(t1);
	// Set up view of src
	dre.setup((double*)src, sizeof(double), col_idx, col_len);
	dre.wait();
	tget(t2);
	// Iterate over each row of matrix
	for (i = 0; i <= end_row; i++) {
		size_t cols = row_start[i+1] - row_start[i];
		if (cols < MIN_COLS) {
			// Loop over vectors
			for (j = row_start[i]; j < row_start[i+1]; j++) {
				dest[i] += value[j] * src[col_idx[j]];
			}
		} 
		else {
			int current_row=row_start[i];
			// Increases rows_to_batch until view buffer full or reaches end of matrix
			for(rows_to_batch=1; row_start[i+rows_to_batch]-current_row < max_block_size && rows_to_batch <= end_row-i; rows_to_batch++);
			if(rows_to_batch > 1) rows_to_batch--;
			const double * restrict row = value + current_row;
			size_t view_off = current_row * sizeof(double);
			size_t view_end = view_off + (row_start[i+rows_to_batch]-current_row)*sizeof(double);
			int element_in_row=0;
			for(int view_iter=0; view_off < view_end; view_off+=block_sz, view_iter++){
				unsigned view_sz = min(block_sz, view_end-view_off);
#ifdef ENTIRE
				tget(t3);
				Xil_L1DCacheFlush();
#endif
				tget(t4);
				dre.fill(block, view_sz, view_off);
				dre.wait();
				tget(t5);
#ifndef ENTIRE
				Xil_L1DCacheInvalidateRange((INTPTR)block, CEIL(view_sz,ALIGN_SZ));
				tget(t6);
#endif
				// Iterates over each row in batch, fills corresponding entry in result vector with sum of products from DRE block and CSR vector
				int element_in_block=0;
				for(j=0; j < rows_to_batch; j++){
					int cnt=min(row_start[i+j+1]-current_row-view_iter*max_block_size,max_block_size);
					register double d0 =0;
					for (; element_in_block < (int)cnt; ++element_in_block,++element_in_row) {
						d0 += row[element_in_row] * block[element_in_block];
					}
					dest[i+j]+=d0;
				}
				tinc(treorg, tdiff(t5,t4));
#ifdef ENTIRE
				tinc(tcache, tdiff(t4,t3));
#else
				tinc(tcache, tdiff(t6,t5));
#endif
			}
			i+=rows_to_batch-1;
		}
	}
	tget(t7);
	// flush output product to memory
	host::cache_flush(dest+start_row, (end_row-start_row+1)*sizeof(double));
	// make sure to invalidate cache before delete
	CACHE_DISPOSE(dre, block, block_sz)
	tget(t8);
	SP_NFREE((double*)block);
	tinc(tcache, tdiff(t1,t0)+tdiff(t8,t7));
	tinc(tsetup, tdiff(t2,t1));
}

#else /* USE_ACC */

void bsmvm_1x1_1 (const int start_row, /*const*/ int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
#if defined(PARTIAL)
	end_row /= PARTIAL;
#endif

  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*1+0] * src[col_idx[j]+0];
    }
    dest[1*i+0] = d0;
  }
  tget(t0);
  // flush output product to memory
  host::cache_flush(dest+start_row, (end_row-start_row+1)*sizeof(double));
  tget(t1);
  tinc(tcache, tdiff(t1,t0));
}

#endif /* USE_ACC */

void bsmvm_1x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*2+0] * src[col_idx[j]+0];
      d0 += value[j*2+1] * src[col_idx[j]+1];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*3+0] * src[col_idx[j]+0];
      d0 += value[j*3+1] * src[col_idx[j]+1];
      d0 += value[j*3+2] * src[col_idx[j]+2];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*4+0] * src[col_idx[j]+0];
      d0 += value[j*4+1] * src[col_idx[j]+1];
      d0 += value[j*4+2] * src[col_idx[j]+2];
      d0 += value[j*4+3] * src[col_idx[j]+3];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*5+0] * src[col_idx[j]+0];
      d0 += value[j*5+1] * src[col_idx[j]+1];
      d0 += value[j*5+2] * src[col_idx[j]+2];
      d0 += value[j*5+3] * src[col_idx[j]+3];
      d0 += value[j*5+4] * src[col_idx[j]+4];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*6+0] * src[col_idx[j]+0];
      d0 += value[j*6+1] * src[col_idx[j]+1];
      d0 += value[j*6+2] * src[col_idx[j]+2];
      d0 += value[j*6+3] * src[col_idx[j]+3];
      d0 += value[j*6+4] * src[col_idx[j]+4];
      d0 += value[j*6+5] * src[col_idx[j]+5];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*7+0] * src[col_idx[j]+0];
      d0 += value[j*7+1] * src[col_idx[j]+1];
      d0 += value[j*7+2] * src[col_idx[j]+2];
      d0 += value[j*7+3] * src[col_idx[j]+3];
      d0 += value[j*7+4] * src[col_idx[j]+4];
      d0 += value[j*7+5] * src[col_idx[j]+5];
      d0 += value[j*7+6] * src[col_idx[j]+6];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*8+0] * src[col_idx[j]+0];
      d0 += value[j*8+1] * src[col_idx[j]+1];
      d0 += value[j*8+2] * src[col_idx[j]+2];
      d0 += value[j*8+3] * src[col_idx[j]+3];
      d0 += value[j*8+4] * src[col_idx[j]+4];
      d0 += value[j*8+5] * src[col_idx[j]+5];
      d0 += value[j*8+6] * src[col_idx[j]+6];
      d0 += value[j*8+7] * src[col_idx[j]+7];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*9+0] * src[col_idx[j]+0];
      d0 += value[j*9+1] * src[col_idx[j]+1];
      d0 += value[j*9+2] * src[col_idx[j]+2];
      d0 += value[j*9+3] * src[col_idx[j]+3];
      d0 += value[j*9+4] * src[col_idx[j]+4];
      d0 += value[j*9+5] * src[col_idx[j]+5];
      d0 += value[j*9+6] * src[col_idx[j]+6];
      d0 += value[j*9+7] * src[col_idx[j]+7];
      d0 += value[j*9+8] * src[col_idx[j]+8];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*10+0] * src[col_idx[j]+0];
      d0 += value[j*10+1] * src[col_idx[j]+1];
      d0 += value[j*10+2] * src[col_idx[j]+2];
      d0 += value[j*10+3] * src[col_idx[j]+3];
      d0 += value[j*10+4] * src[col_idx[j]+4];
      d0 += value[j*10+5] * src[col_idx[j]+5];
      d0 += value[j*10+6] * src[col_idx[j]+6];
      d0 += value[j*10+7] * src[col_idx[j]+7];
      d0 += value[j*10+8] * src[col_idx[j]+8];
      d0 += value[j*10+9] * src[col_idx[j]+9];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*11+0] * src[col_idx[j]+0];
      d0 += value[j*11+1] * src[col_idx[j]+1];
      d0 += value[j*11+2] * src[col_idx[j]+2];
      d0 += value[j*11+3] * src[col_idx[j]+3];
      d0 += value[j*11+4] * src[col_idx[j]+4];
      d0 += value[j*11+5] * src[col_idx[j]+5];
      d0 += value[j*11+6] * src[col_idx[j]+6];
      d0 += value[j*11+7] * src[col_idx[j]+7];
      d0 += value[j*11+8] * src[col_idx[j]+8];
      d0 += value[j*11+9] * src[col_idx[j]+9];
      d0 += value[j*11+10] * src[col_idx[j]+10];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_1x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0;
    d0 = dest[1*i+0];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d0 += value[j*12+1] * src[col_idx[j]+1];
      d0 += value[j*12+2] * src[col_idx[j]+2];
      d0 += value[j*12+3] * src[col_idx[j]+3];
      d0 += value[j*12+4] * src[col_idx[j]+4];
      d0 += value[j*12+5] * src[col_idx[j]+5];
      d0 += value[j*12+6] * src[col_idx[j]+6];
      d0 += value[j*12+7] * src[col_idx[j]+7];
      d0 += value[j*12+8] * src[col_idx[j]+8];
      d0 += value[j*12+9] * src[col_idx[j]+9];
      d0 += value[j*12+10] * src[col_idx[j]+10];
      d0 += value[j*12+11] * src[col_idx[j]+11];
    }
    dest[1*i+0] = d0;
  }
}
void bsmvm_2x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*2+0] * src[col_idx[j]+0];
      d1 += value[j*2+1] * src[col_idx[j]+0];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*4+0] * src[col_idx[j]+0];
      d1 += value[j*4+2] * src[col_idx[j]+0];
      d0 += value[j*4+1] * src[col_idx[j]+1];
      d1 += value[j*4+3] * src[col_idx[j]+1];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*6+0] * src[col_idx[j]+0];
      d1 += value[j*6+3] * src[col_idx[j]+0];
      d0 += value[j*6+1] * src[col_idx[j]+1];
      d1 += value[j*6+4] * src[col_idx[j]+1];
      d0 += value[j*6+2] * src[col_idx[j]+2];
      d1 += value[j*6+5] * src[col_idx[j]+2];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*8+0] * src[col_idx[j]+0];
      d1 += value[j*8+4] * src[col_idx[j]+0];
      d0 += value[j*8+1] * src[col_idx[j]+1];
      d1 += value[j*8+5] * src[col_idx[j]+1];
      d0 += value[j*8+2] * src[col_idx[j]+2];
      d1 += value[j*8+6] * src[col_idx[j]+2];
      d0 += value[j*8+3] * src[col_idx[j]+3];
      d1 += value[j*8+7] * src[col_idx[j]+3];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*10+0] * src[col_idx[j]+0];
      d1 += value[j*10+5] * src[col_idx[j]+0];
      d0 += value[j*10+1] * src[col_idx[j]+1];
      d1 += value[j*10+6] * src[col_idx[j]+1];
      d0 += value[j*10+2] * src[col_idx[j]+2];
      d1 += value[j*10+7] * src[col_idx[j]+2];
      d0 += value[j*10+3] * src[col_idx[j]+3];
      d1 += value[j*10+8] * src[col_idx[j]+3];
      d0 += value[j*10+4] * src[col_idx[j]+4];
      d1 += value[j*10+9] * src[col_idx[j]+4];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d1 += value[j*12+6] * src[col_idx[j]+0];
      d0 += value[j*12+1] * src[col_idx[j]+1];
      d1 += value[j*12+7] * src[col_idx[j]+1];
      d0 += value[j*12+2] * src[col_idx[j]+2];
      d1 += value[j*12+8] * src[col_idx[j]+2];
      d0 += value[j*12+3] * src[col_idx[j]+3];
      d1 += value[j*12+9] * src[col_idx[j]+3];
      d0 += value[j*12+4] * src[col_idx[j]+4];
      d1 += value[j*12+10] * src[col_idx[j]+4];
      d0 += value[j*12+5] * src[col_idx[j]+5];
      d1 += value[j*12+11] * src[col_idx[j]+5];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*14+0] * src[col_idx[j]+0];
      d1 += value[j*14+7] * src[col_idx[j]+0];
      d0 += value[j*14+1] * src[col_idx[j]+1];
      d1 += value[j*14+8] * src[col_idx[j]+1];
      d0 += value[j*14+2] * src[col_idx[j]+2];
      d1 += value[j*14+9] * src[col_idx[j]+2];
      d0 += value[j*14+3] * src[col_idx[j]+3];
      d1 += value[j*14+10] * src[col_idx[j]+3];
      d0 += value[j*14+4] * src[col_idx[j]+4];
      d1 += value[j*14+11] * src[col_idx[j]+4];
      d0 += value[j*14+5] * src[col_idx[j]+5];
      d1 += value[j*14+12] * src[col_idx[j]+5];
      d0 += value[j*14+6] * src[col_idx[j]+6];
      d1 += value[j*14+13] * src[col_idx[j]+6];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*16+0] * src[col_idx[j]+0];
      d1 += value[j*16+8] * src[col_idx[j]+0];
      d0 += value[j*16+1] * src[col_idx[j]+1];
      d1 += value[j*16+9] * src[col_idx[j]+1];
      d0 += value[j*16+2] * src[col_idx[j]+2];
      d1 += value[j*16+10] * src[col_idx[j]+2];
      d0 += value[j*16+3] * src[col_idx[j]+3];
      d1 += value[j*16+11] * src[col_idx[j]+3];
      d0 += value[j*16+4] * src[col_idx[j]+4];
      d1 += value[j*16+12] * src[col_idx[j]+4];
      d0 += value[j*16+5] * src[col_idx[j]+5];
      d1 += value[j*16+13] * src[col_idx[j]+5];
      d0 += value[j*16+6] * src[col_idx[j]+6];
      d1 += value[j*16+14] * src[col_idx[j]+6];
      d0 += value[j*16+7] * src[col_idx[j]+7];
      d1 += value[j*16+15] * src[col_idx[j]+7];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*18+0] * src[col_idx[j]+0];
      d1 += value[j*18+9] * src[col_idx[j]+0];
      d0 += value[j*18+1] * src[col_idx[j]+1];
      d1 += value[j*18+10] * src[col_idx[j]+1];
      d0 += value[j*18+2] * src[col_idx[j]+2];
      d1 += value[j*18+11] * src[col_idx[j]+2];
      d0 += value[j*18+3] * src[col_idx[j]+3];
      d1 += value[j*18+12] * src[col_idx[j]+3];
      d0 += value[j*18+4] * src[col_idx[j]+4];
      d1 += value[j*18+13] * src[col_idx[j]+4];
      d0 += value[j*18+5] * src[col_idx[j]+5];
      d1 += value[j*18+14] * src[col_idx[j]+5];
      d0 += value[j*18+6] * src[col_idx[j]+6];
      d1 += value[j*18+15] * src[col_idx[j]+6];
      d0 += value[j*18+7] * src[col_idx[j]+7];
      d1 += value[j*18+16] * src[col_idx[j]+7];
      d0 += value[j*18+8] * src[col_idx[j]+8];
      d1 += value[j*18+17] * src[col_idx[j]+8];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*20+0] * src[col_idx[j]+0];
      d1 += value[j*20+10] * src[col_idx[j]+0];
      d0 += value[j*20+1] * src[col_idx[j]+1];
      d1 += value[j*20+11] * src[col_idx[j]+1];
      d0 += value[j*20+2] * src[col_idx[j]+2];
      d1 += value[j*20+12] * src[col_idx[j]+2];
      d0 += value[j*20+3] * src[col_idx[j]+3];
      d1 += value[j*20+13] * src[col_idx[j]+3];
      d0 += value[j*20+4] * src[col_idx[j]+4];
      d1 += value[j*20+14] * src[col_idx[j]+4];
      d0 += value[j*20+5] * src[col_idx[j]+5];
      d1 += value[j*20+15] * src[col_idx[j]+5];
      d0 += value[j*20+6] * src[col_idx[j]+6];
      d1 += value[j*20+16] * src[col_idx[j]+6];
      d0 += value[j*20+7] * src[col_idx[j]+7];
      d1 += value[j*20+17] * src[col_idx[j]+7];
      d0 += value[j*20+8] * src[col_idx[j]+8];
      d1 += value[j*20+18] * src[col_idx[j]+8];
      d0 += value[j*20+9] * src[col_idx[j]+9];
      d1 += value[j*20+19] * src[col_idx[j]+9];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*22+0] * src[col_idx[j]+0];
      d1 += value[j*22+11] * src[col_idx[j]+0];
      d0 += value[j*22+1] * src[col_idx[j]+1];
      d1 += value[j*22+12] * src[col_idx[j]+1];
      d0 += value[j*22+2] * src[col_idx[j]+2];
      d1 += value[j*22+13] * src[col_idx[j]+2];
      d0 += value[j*22+3] * src[col_idx[j]+3];
      d1 += value[j*22+14] * src[col_idx[j]+3];
      d0 += value[j*22+4] * src[col_idx[j]+4];
      d1 += value[j*22+15] * src[col_idx[j]+4];
      d0 += value[j*22+5] * src[col_idx[j]+5];
      d1 += value[j*22+16] * src[col_idx[j]+5];
      d0 += value[j*22+6] * src[col_idx[j]+6];
      d1 += value[j*22+17] * src[col_idx[j]+6];
      d0 += value[j*22+7] * src[col_idx[j]+7];
      d1 += value[j*22+18] * src[col_idx[j]+7];
      d0 += value[j*22+8] * src[col_idx[j]+8];
      d1 += value[j*22+19] * src[col_idx[j]+8];
      d0 += value[j*22+9] * src[col_idx[j]+9];
      d1 += value[j*22+20] * src[col_idx[j]+9];
      d0 += value[j*22+10] * src[col_idx[j]+10];
      d1 += value[j*22+21] * src[col_idx[j]+10];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_2x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1;
    d0 = dest[2*i+0];
    d1 = dest[2*i+1];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+12] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+13] * src[col_idx[j]+1];
      d0 += value[j*24+2] * src[col_idx[j]+2];
      d1 += value[j*24+14] * src[col_idx[j]+2];
      d0 += value[j*24+3] * src[col_idx[j]+3];
      d1 += value[j*24+15] * src[col_idx[j]+3];
      d0 += value[j*24+4] * src[col_idx[j]+4];
      d1 += value[j*24+16] * src[col_idx[j]+4];
      d0 += value[j*24+5] * src[col_idx[j]+5];
      d1 += value[j*24+17] * src[col_idx[j]+5];
      d0 += value[j*24+6] * src[col_idx[j]+6];
      d1 += value[j*24+18] * src[col_idx[j]+6];
      d0 += value[j*24+7] * src[col_idx[j]+7];
      d1 += value[j*24+19] * src[col_idx[j]+7];
      d0 += value[j*24+8] * src[col_idx[j]+8];
      d1 += value[j*24+20] * src[col_idx[j]+8];
      d0 += value[j*24+9] * src[col_idx[j]+9];
      d1 += value[j*24+21] * src[col_idx[j]+9];
      d0 += value[j*24+10] * src[col_idx[j]+10];
      d1 += value[j*24+22] * src[col_idx[j]+10];
      d0 += value[j*24+11] * src[col_idx[j]+11];
      d1 += value[j*24+23] * src[col_idx[j]+11];
    }
    dest[2*i+0] = d0;
    dest[2*i+1] = d1;
  }
}
void bsmvm_3x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*3+0] * src[col_idx[j]+0];
      d1 += value[j*3+1] * src[col_idx[j]+0];
      d2 += value[j*3+2] * src[col_idx[j]+0];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*6+0] * src[col_idx[j]+0];
      d1 += value[j*6+2] * src[col_idx[j]+0];
      d2 += value[j*6+4] * src[col_idx[j]+0];
      d0 += value[j*6+1] * src[col_idx[j]+1];
      d1 += value[j*6+3] * src[col_idx[j]+1];
      d2 += value[j*6+5] * src[col_idx[j]+1];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*9+0] * src[col_idx[j]+0];
      d1 += value[j*9+3] * src[col_idx[j]+0];
      d2 += value[j*9+6] * src[col_idx[j]+0];
      d0 += value[j*9+1] * src[col_idx[j]+1];
      d1 += value[j*9+4] * src[col_idx[j]+1];
      d2 += value[j*9+7] * src[col_idx[j]+1];
      d0 += value[j*9+2] * src[col_idx[j]+2];
      d1 += value[j*9+5] * src[col_idx[j]+2];
      d2 += value[j*9+8] * src[col_idx[j]+2];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d1 += value[j*12+4] * src[col_idx[j]+0];
      d2 += value[j*12+8] * src[col_idx[j]+0];
      d0 += value[j*12+1] * src[col_idx[j]+1];
      d1 += value[j*12+5] * src[col_idx[j]+1];
      d2 += value[j*12+9] * src[col_idx[j]+1];
      d0 += value[j*12+2] * src[col_idx[j]+2];
      d1 += value[j*12+6] * src[col_idx[j]+2];
      d2 += value[j*12+10] * src[col_idx[j]+2];
      d0 += value[j*12+3] * src[col_idx[j]+3];
      d1 += value[j*12+7] * src[col_idx[j]+3];
      d2 += value[j*12+11] * src[col_idx[j]+3];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*15+0] * src[col_idx[j]+0];
      d1 += value[j*15+5] * src[col_idx[j]+0];
      d2 += value[j*15+10] * src[col_idx[j]+0];
      d0 += value[j*15+1] * src[col_idx[j]+1];
      d1 += value[j*15+6] * src[col_idx[j]+1];
      d2 += value[j*15+11] * src[col_idx[j]+1];
      d0 += value[j*15+2] * src[col_idx[j]+2];
      d1 += value[j*15+7] * src[col_idx[j]+2];
      d2 += value[j*15+12] * src[col_idx[j]+2];
      d0 += value[j*15+3] * src[col_idx[j]+3];
      d1 += value[j*15+8] * src[col_idx[j]+3];
      d2 += value[j*15+13] * src[col_idx[j]+3];
      d0 += value[j*15+4] * src[col_idx[j]+4];
      d1 += value[j*15+9] * src[col_idx[j]+4];
      d2 += value[j*15+14] * src[col_idx[j]+4];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*18+0] * src[col_idx[j]+0];
      d1 += value[j*18+6] * src[col_idx[j]+0];
      d2 += value[j*18+12] * src[col_idx[j]+0];
      d0 += value[j*18+1] * src[col_idx[j]+1];
      d1 += value[j*18+7] * src[col_idx[j]+1];
      d2 += value[j*18+13] * src[col_idx[j]+1];
      d0 += value[j*18+2] * src[col_idx[j]+2];
      d1 += value[j*18+8] * src[col_idx[j]+2];
      d2 += value[j*18+14] * src[col_idx[j]+2];
      d0 += value[j*18+3] * src[col_idx[j]+3];
      d1 += value[j*18+9] * src[col_idx[j]+3];
      d2 += value[j*18+15] * src[col_idx[j]+3];
      d0 += value[j*18+4] * src[col_idx[j]+4];
      d1 += value[j*18+10] * src[col_idx[j]+4];
      d2 += value[j*18+16] * src[col_idx[j]+4];
      d0 += value[j*18+5] * src[col_idx[j]+5];
      d1 += value[j*18+11] * src[col_idx[j]+5];
      d2 += value[j*18+17] * src[col_idx[j]+5];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*21+0] * src[col_idx[j]+0];
      d1 += value[j*21+7] * src[col_idx[j]+0];
      d2 += value[j*21+14] * src[col_idx[j]+0];
      d0 += value[j*21+1] * src[col_idx[j]+1];
      d1 += value[j*21+8] * src[col_idx[j]+1];
      d2 += value[j*21+15] * src[col_idx[j]+1];
      d0 += value[j*21+2] * src[col_idx[j]+2];
      d1 += value[j*21+9] * src[col_idx[j]+2];
      d2 += value[j*21+16] * src[col_idx[j]+2];
      d0 += value[j*21+3] * src[col_idx[j]+3];
      d1 += value[j*21+10] * src[col_idx[j]+3];
      d2 += value[j*21+17] * src[col_idx[j]+3];
      d0 += value[j*21+4] * src[col_idx[j]+4];
      d1 += value[j*21+11] * src[col_idx[j]+4];
      d2 += value[j*21+18] * src[col_idx[j]+4];
      d0 += value[j*21+5] * src[col_idx[j]+5];
      d1 += value[j*21+12] * src[col_idx[j]+5];
      d2 += value[j*21+19] * src[col_idx[j]+5];
      d0 += value[j*21+6] * src[col_idx[j]+6];
      d1 += value[j*21+13] * src[col_idx[j]+6];
      d2 += value[j*21+20] * src[col_idx[j]+6];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+8] * src[col_idx[j]+0];
      d2 += value[j*24+16] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+9] * src[col_idx[j]+1];
      d2 += value[j*24+17] * src[col_idx[j]+1];
      d0 += value[j*24+2] * src[col_idx[j]+2];
      d1 += value[j*24+10] * src[col_idx[j]+2];
      d2 += value[j*24+18] * src[col_idx[j]+2];
      d0 += value[j*24+3] * src[col_idx[j]+3];
      d1 += value[j*24+11] * src[col_idx[j]+3];
      d2 += value[j*24+19] * src[col_idx[j]+3];
      d0 += value[j*24+4] * src[col_idx[j]+4];
      d1 += value[j*24+12] * src[col_idx[j]+4];
      d2 += value[j*24+20] * src[col_idx[j]+4];
      d0 += value[j*24+5] * src[col_idx[j]+5];
      d1 += value[j*24+13] * src[col_idx[j]+5];
      d2 += value[j*24+21] * src[col_idx[j]+5];
      d0 += value[j*24+6] * src[col_idx[j]+6];
      d1 += value[j*24+14] * src[col_idx[j]+6];
      d2 += value[j*24+22] * src[col_idx[j]+6];
      d0 += value[j*24+7] * src[col_idx[j]+7];
      d1 += value[j*24+15] * src[col_idx[j]+7];
      d2 += value[j*24+23] * src[col_idx[j]+7];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*27+0] * src[col_idx[j]+0];
      d1 += value[j*27+9] * src[col_idx[j]+0];
      d2 += value[j*27+18] * src[col_idx[j]+0];
      d0 += value[j*27+1] * src[col_idx[j]+1];
      d1 += value[j*27+10] * src[col_idx[j]+1];
      d2 += value[j*27+19] * src[col_idx[j]+1];
      d0 += value[j*27+2] * src[col_idx[j]+2];
      d1 += value[j*27+11] * src[col_idx[j]+2];
      d2 += value[j*27+20] * src[col_idx[j]+2];
      d0 += value[j*27+3] * src[col_idx[j]+3];
      d1 += value[j*27+12] * src[col_idx[j]+3];
      d2 += value[j*27+21] * src[col_idx[j]+3];
      d0 += value[j*27+4] * src[col_idx[j]+4];
      d1 += value[j*27+13] * src[col_idx[j]+4];
      d2 += value[j*27+22] * src[col_idx[j]+4];
      d0 += value[j*27+5] * src[col_idx[j]+5];
      d1 += value[j*27+14] * src[col_idx[j]+5];
      d2 += value[j*27+23] * src[col_idx[j]+5];
      d0 += value[j*27+6] * src[col_idx[j]+6];
      d1 += value[j*27+15] * src[col_idx[j]+6];
      d2 += value[j*27+24] * src[col_idx[j]+6];
      d0 += value[j*27+7] * src[col_idx[j]+7];
      d1 += value[j*27+16] * src[col_idx[j]+7];
      d2 += value[j*27+25] * src[col_idx[j]+7];
      d0 += value[j*27+8] * src[col_idx[j]+8];
      d1 += value[j*27+17] * src[col_idx[j]+8];
      d2 += value[j*27+26] * src[col_idx[j]+8];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*30+0] * src[col_idx[j]+0];
      d1 += value[j*30+10] * src[col_idx[j]+0];
      d2 += value[j*30+20] * src[col_idx[j]+0];
      d0 += value[j*30+1] * src[col_idx[j]+1];
      d1 += value[j*30+11] * src[col_idx[j]+1];
      d2 += value[j*30+21] * src[col_idx[j]+1];
      d0 += value[j*30+2] * src[col_idx[j]+2];
      d1 += value[j*30+12] * src[col_idx[j]+2];
      d2 += value[j*30+22] * src[col_idx[j]+2];
      d0 += value[j*30+3] * src[col_idx[j]+3];
      d1 += value[j*30+13] * src[col_idx[j]+3];
      d2 += value[j*30+23] * src[col_idx[j]+3];
      d0 += value[j*30+4] * src[col_idx[j]+4];
      d1 += value[j*30+14] * src[col_idx[j]+4];
      d2 += value[j*30+24] * src[col_idx[j]+4];
      d0 += value[j*30+5] * src[col_idx[j]+5];
      d1 += value[j*30+15] * src[col_idx[j]+5];
      d2 += value[j*30+25] * src[col_idx[j]+5];
      d0 += value[j*30+6] * src[col_idx[j]+6];
      d1 += value[j*30+16] * src[col_idx[j]+6];
      d2 += value[j*30+26] * src[col_idx[j]+6];
      d0 += value[j*30+7] * src[col_idx[j]+7];
      d1 += value[j*30+17] * src[col_idx[j]+7];
      d2 += value[j*30+27] * src[col_idx[j]+7];
      d0 += value[j*30+8] * src[col_idx[j]+8];
      d1 += value[j*30+18] * src[col_idx[j]+8];
      d2 += value[j*30+28] * src[col_idx[j]+8];
      d0 += value[j*30+9] * src[col_idx[j]+9];
      d1 += value[j*30+19] * src[col_idx[j]+9];
      d2 += value[j*30+29] * src[col_idx[j]+9];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*33+0] * src[col_idx[j]+0];
      d1 += value[j*33+11] * src[col_idx[j]+0];
      d2 += value[j*33+22] * src[col_idx[j]+0];
      d0 += value[j*33+1] * src[col_idx[j]+1];
      d1 += value[j*33+12] * src[col_idx[j]+1];
      d2 += value[j*33+23] * src[col_idx[j]+1];
      d0 += value[j*33+2] * src[col_idx[j]+2];
      d1 += value[j*33+13] * src[col_idx[j]+2];
      d2 += value[j*33+24] * src[col_idx[j]+2];
      d0 += value[j*33+3] * src[col_idx[j]+3];
      d1 += value[j*33+14] * src[col_idx[j]+3];
      d2 += value[j*33+25] * src[col_idx[j]+3];
      d0 += value[j*33+4] * src[col_idx[j]+4];
      d1 += value[j*33+15] * src[col_idx[j]+4];
      d2 += value[j*33+26] * src[col_idx[j]+4];
      d0 += value[j*33+5] * src[col_idx[j]+5];
      d1 += value[j*33+16] * src[col_idx[j]+5];
      d2 += value[j*33+27] * src[col_idx[j]+5];
      d0 += value[j*33+6] * src[col_idx[j]+6];
      d1 += value[j*33+17] * src[col_idx[j]+6];
      d2 += value[j*33+28] * src[col_idx[j]+6];
      d0 += value[j*33+7] * src[col_idx[j]+7];
      d1 += value[j*33+18] * src[col_idx[j]+7];
      d2 += value[j*33+29] * src[col_idx[j]+7];
      d0 += value[j*33+8] * src[col_idx[j]+8];
      d1 += value[j*33+19] * src[col_idx[j]+8];
      d2 += value[j*33+30] * src[col_idx[j]+8];
      d0 += value[j*33+9] * src[col_idx[j]+9];
      d1 += value[j*33+20] * src[col_idx[j]+9];
      d2 += value[j*33+31] * src[col_idx[j]+9];
      d0 += value[j*33+10] * src[col_idx[j]+10];
      d1 += value[j*33+21] * src[col_idx[j]+10];
      d2 += value[j*33+32] * src[col_idx[j]+10];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_3x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2;
    d0 = dest[3*i+0];
    d1 = dest[3*i+1];
    d2 = dest[3*i+2];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*36+0] * src[col_idx[j]+0];
      d1 += value[j*36+12] * src[col_idx[j]+0];
      d2 += value[j*36+24] * src[col_idx[j]+0];
      d0 += value[j*36+1] * src[col_idx[j]+1];
      d1 += value[j*36+13] * src[col_idx[j]+1];
      d2 += value[j*36+25] * src[col_idx[j]+1];
      d0 += value[j*36+2] * src[col_idx[j]+2];
      d1 += value[j*36+14] * src[col_idx[j]+2];
      d2 += value[j*36+26] * src[col_idx[j]+2];
      d0 += value[j*36+3] * src[col_idx[j]+3];
      d1 += value[j*36+15] * src[col_idx[j]+3];
      d2 += value[j*36+27] * src[col_idx[j]+3];
      d0 += value[j*36+4] * src[col_idx[j]+4];
      d1 += value[j*36+16] * src[col_idx[j]+4];
      d2 += value[j*36+28] * src[col_idx[j]+4];
      d0 += value[j*36+5] * src[col_idx[j]+5];
      d1 += value[j*36+17] * src[col_idx[j]+5];
      d2 += value[j*36+29] * src[col_idx[j]+5];
      d0 += value[j*36+6] * src[col_idx[j]+6];
      d1 += value[j*36+18] * src[col_idx[j]+6];
      d2 += value[j*36+30] * src[col_idx[j]+6];
      d0 += value[j*36+7] * src[col_idx[j]+7];
      d1 += value[j*36+19] * src[col_idx[j]+7];
      d2 += value[j*36+31] * src[col_idx[j]+7];
      d0 += value[j*36+8] * src[col_idx[j]+8];
      d1 += value[j*36+20] * src[col_idx[j]+8];
      d2 += value[j*36+32] * src[col_idx[j]+8];
      d0 += value[j*36+9] * src[col_idx[j]+9];
      d1 += value[j*36+21] * src[col_idx[j]+9];
      d2 += value[j*36+33] * src[col_idx[j]+9];
      d0 += value[j*36+10] * src[col_idx[j]+10];
      d1 += value[j*36+22] * src[col_idx[j]+10];
      d2 += value[j*36+34] * src[col_idx[j]+10];
      d0 += value[j*36+11] * src[col_idx[j]+11];
      d1 += value[j*36+23] * src[col_idx[j]+11];
      d2 += value[j*36+35] * src[col_idx[j]+11];
    }
    dest[3*i+0] = d0;
    dest[3*i+1] = d1;
    dest[3*i+2] = d2;
  }
}
void bsmvm_4x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*4+0] * src[col_idx[j]+0];
      d1 += value[j*4+1] * src[col_idx[j]+0];
      d2 += value[j*4+2] * src[col_idx[j]+0];
      d3 += value[j*4+3] * src[col_idx[j]+0];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*8+0] * src[col_idx[j]+0];
      d1 += value[j*8+2] * src[col_idx[j]+0];
      d2 += value[j*8+4] * src[col_idx[j]+0];
      d3 += value[j*8+6] * src[col_idx[j]+0];
      d0 += value[j*8+1] * src[col_idx[j]+1];
      d1 += value[j*8+3] * src[col_idx[j]+1];
      d2 += value[j*8+5] * src[col_idx[j]+1];
      d3 += value[j*8+7] * src[col_idx[j]+1];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d1 += value[j*12+3] * src[col_idx[j]+0];
      d2 += value[j*12+6] * src[col_idx[j]+0];
      d3 += value[j*12+9] * src[col_idx[j]+0];
      d0 += value[j*12+1] * src[col_idx[j]+1];
      d1 += value[j*12+4] * src[col_idx[j]+1];
      d2 += value[j*12+7] * src[col_idx[j]+1];
      d3 += value[j*12+10] * src[col_idx[j]+1];
      d0 += value[j*12+2] * src[col_idx[j]+2];
      d1 += value[j*12+5] * src[col_idx[j]+2];
      d2 += value[j*12+8] * src[col_idx[j]+2];
      d3 += value[j*12+11] * src[col_idx[j]+2];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;

  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;

    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*16+0] * src[col_idx[j]+0];
      d1 += value[j*16+4] * src[col_idx[j]+0];
      d2 += value[j*16+8] * src[col_idx[j]+0];
      d3 += value[j*16+12] * src[col_idx[j]+0];
      d0 += value[j*16+1] * src[col_idx[j]+1];
      d1 += value[j*16+5] * src[col_idx[j]+1];
      d2 += value[j*16+9] * src[col_idx[j]+1];
      d3 += value[j*16+13] * src[col_idx[j]+1];
      d0 += value[j*16+2] * src[col_idx[j]+2];
      d1 += value[j*16+6] * src[col_idx[j]+2];
      d2 += value[j*16+10] * src[col_idx[j]+2];
      d3 += value[j*16+14] * src[col_idx[j]+2];
      d0 += value[j*16+3] * src[col_idx[j]+3];
      d1 += value[j*16+7] * src[col_idx[j]+3];
      d2 += value[j*16+11] * src[col_idx[j]+3];
      d3 += value[j*16+15] * src[col_idx[j]+3];
    }

    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*20+0] * src[col_idx[j]+0];
      d1 += value[j*20+5] * src[col_idx[j]+0];
      d2 += value[j*20+10] * src[col_idx[j]+0];
      d3 += value[j*20+15] * src[col_idx[j]+0];
      d0 += value[j*20+1] * src[col_idx[j]+1];
      d1 += value[j*20+6] * src[col_idx[j]+1];
      d2 += value[j*20+11] * src[col_idx[j]+1];
      d3 += value[j*20+16] * src[col_idx[j]+1];
      d0 += value[j*20+2] * src[col_idx[j]+2];
      d1 += value[j*20+7] * src[col_idx[j]+2];
      d2 += value[j*20+12] * src[col_idx[j]+2];
      d3 += value[j*20+17] * src[col_idx[j]+2];
      d0 += value[j*20+3] * src[col_idx[j]+3];
      d1 += value[j*20+8] * src[col_idx[j]+3];
      d2 += value[j*20+13] * src[col_idx[j]+3];
      d3 += value[j*20+18] * src[col_idx[j]+3];
      d0 += value[j*20+4] * src[col_idx[j]+4];
      d1 += value[j*20+9] * src[col_idx[j]+4];
      d2 += value[j*20+14] * src[col_idx[j]+4];
      d3 += value[j*20+19] * src[col_idx[j]+4];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+6] * src[col_idx[j]+0];
      d2 += value[j*24+12] * src[col_idx[j]+0];
      d3 += value[j*24+18] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+7] * src[col_idx[j]+1];
      d2 += value[j*24+13] * src[col_idx[j]+1];
      d3 += value[j*24+19] * src[col_idx[j]+1];
      d0 += value[j*24+2] * src[col_idx[j]+2];
      d1 += value[j*24+8] * src[col_idx[j]+2];
      d2 += value[j*24+14] * src[col_idx[j]+2];
      d3 += value[j*24+20] * src[col_idx[j]+2];
      d0 += value[j*24+3] * src[col_idx[j]+3];
      d1 += value[j*24+9] * src[col_idx[j]+3];
      d2 += value[j*24+15] * src[col_idx[j]+3];
      d3 += value[j*24+21] * src[col_idx[j]+3];
      d0 += value[j*24+4] * src[col_idx[j]+4];
      d1 += value[j*24+10] * src[col_idx[j]+4];
      d2 += value[j*24+16] * src[col_idx[j]+4];
      d3 += value[j*24+22] * src[col_idx[j]+4];
      d0 += value[j*24+5] * src[col_idx[j]+5];
      d1 += value[j*24+11] * src[col_idx[j]+5];
      d2 += value[j*24+17] * src[col_idx[j]+5];
      d3 += value[j*24+23] * src[col_idx[j]+5];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*28+0] * src[col_idx[j]+0];
      d1 += value[j*28+7] * src[col_idx[j]+0];
      d2 += value[j*28+14] * src[col_idx[j]+0];
      d3 += value[j*28+21] * src[col_idx[j]+0];
      d0 += value[j*28+1] * src[col_idx[j]+1];
      d1 += value[j*28+8] * src[col_idx[j]+1];
      d2 += value[j*28+15] * src[col_idx[j]+1];
      d3 += value[j*28+22] * src[col_idx[j]+1];
      d0 += value[j*28+2] * src[col_idx[j]+2];
      d1 += value[j*28+9] * src[col_idx[j]+2];
      d2 += value[j*28+16] * src[col_idx[j]+2];
      d3 += value[j*28+23] * src[col_idx[j]+2];
      d0 += value[j*28+3] * src[col_idx[j]+3];
      d1 += value[j*28+10] * src[col_idx[j]+3];
      d2 += value[j*28+17] * src[col_idx[j]+3];
      d3 += value[j*28+24] * src[col_idx[j]+3];
      d0 += value[j*28+4] * src[col_idx[j]+4];
      d1 += value[j*28+11] * src[col_idx[j]+4];
      d2 += value[j*28+18] * src[col_idx[j]+4];
      d3 += value[j*28+25] * src[col_idx[j]+4];
      d0 += value[j*28+5] * src[col_idx[j]+5];
      d1 += value[j*28+12] * src[col_idx[j]+5];
      d2 += value[j*28+19] * src[col_idx[j]+5];
      d3 += value[j*28+26] * src[col_idx[j]+5];
      d0 += value[j*28+6] * src[col_idx[j]+6];
      d1 += value[j*28+13] * src[col_idx[j]+6];
      d2 += value[j*28+20] * src[col_idx[j]+6];
      d3 += value[j*28+27] * src[col_idx[j]+6];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*32+0] * src[col_idx[j]+0];
      d1 += value[j*32+8] * src[col_idx[j]+0];
      d2 += value[j*32+16] * src[col_idx[j]+0];
      d3 += value[j*32+24] * src[col_idx[j]+0];
      d0 += value[j*32+1] * src[col_idx[j]+1];
      d1 += value[j*32+9] * src[col_idx[j]+1];
      d2 += value[j*32+17] * src[col_idx[j]+1];
      d3 += value[j*32+25] * src[col_idx[j]+1];
      d0 += value[j*32+2] * src[col_idx[j]+2];
      d1 += value[j*32+10] * src[col_idx[j]+2];
      d2 += value[j*32+18] * src[col_idx[j]+2];
      d3 += value[j*32+26] * src[col_idx[j]+2];
      d0 += value[j*32+3] * src[col_idx[j]+3];
      d1 += value[j*32+11] * src[col_idx[j]+3];
      d2 += value[j*32+19] * src[col_idx[j]+3];
      d3 += value[j*32+27] * src[col_idx[j]+3];
      d0 += value[j*32+4] * src[col_idx[j]+4];
      d1 += value[j*32+12] * src[col_idx[j]+4];
      d2 += value[j*32+20] * src[col_idx[j]+4];
      d3 += value[j*32+28] * src[col_idx[j]+4];
      d0 += value[j*32+5] * src[col_idx[j]+5];
      d1 += value[j*32+13] * src[col_idx[j]+5];
      d2 += value[j*32+21] * src[col_idx[j]+5];
      d3 += value[j*32+29] * src[col_idx[j]+5];
      d0 += value[j*32+6] * src[col_idx[j]+6];
      d1 += value[j*32+14] * src[col_idx[j]+6];
      d2 += value[j*32+22] * src[col_idx[j]+6];
      d3 += value[j*32+30] * src[col_idx[j]+6];
      d0 += value[j*32+7] * src[col_idx[j]+7];
      d1 += value[j*32+15] * src[col_idx[j]+7];
      d2 += value[j*32+23] * src[col_idx[j]+7];
      d3 += value[j*32+31] * src[col_idx[j]+7];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*36+0] * src[col_idx[j]+0];
      d1 += value[j*36+9] * src[col_idx[j]+0];
      d2 += value[j*36+18] * src[col_idx[j]+0];
      d3 += value[j*36+27] * src[col_idx[j]+0];
      d0 += value[j*36+1] * src[col_idx[j]+1];
      d1 += value[j*36+10] * src[col_idx[j]+1];
      d2 += value[j*36+19] * src[col_idx[j]+1];
      d3 += value[j*36+28] * src[col_idx[j]+1];
      d0 += value[j*36+2] * src[col_idx[j]+2];
      d1 += value[j*36+11] * src[col_idx[j]+2];
      d2 += value[j*36+20] * src[col_idx[j]+2];
      d3 += value[j*36+29] * src[col_idx[j]+2];
      d0 += value[j*36+3] * src[col_idx[j]+3];
      d1 += value[j*36+12] * src[col_idx[j]+3];
      d2 += value[j*36+21] * src[col_idx[j]+3];
      d3 += value[j*36+30] * src[col_idx[j]+3];
      d0 += value[j*36+4] * src[col_idx[j]+4];
      d1 += value[j*36+13] * src[col_idx[j]+4];
      d2 += value[j*36+22] * src[col_idx[j]+4];
      d3 += value[j*36+31] * src[col_idx[j]+4];
      d0 += value[j*36+5] * src[col_idx[j]+5];
      d1 += value[j*36+14] * src[col_idx[j]+5];
      d2 += value[j*36+23] * src[col_idx[j]+5];
      d3 += value[j*36+32] * src[col_idx[j]+5];
      d0 += value[j*36+6] * src[col_idx[j]+6];
      d1 += value[j*36+15] * src[col_idx[j]+6];
      d2 += value[j*36+24] * src[col_idx[j]+6];
      d3 += value[j*36+33] * src[col_idx[j]+6];
      d0 += value[j*36+7] * src[col_idx[j]+7];
      d1 += value[j*36+16] * src[col_idx[j]+7];
      d2 += value[j*36+25] * src[col_idx[j]+7];
      d3 += value[j*36+34] * src[col_idx[j]+7];
      d0 += value[j*36+8] * src[col_idx[j]+8];
      d1 += value[j*36+17] * src[col_idx[j]+8];
      d2 += value[j*36+26] * src[col_idx[j]+8];
      d3 += value[j*36+35] * src[col_idx[j]+8];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*40+0] * src[col_idx[j]+0];
      d1 += value[j*40+10] * src[col_idx[j]+0];
      d2 += value[j*40+20] * src[col_idx[j]+0];
      d3 += value[j*40+30] * src[col_idx[j]+0];
      d0 += value[j*40+1] * src[col_idx[j]+1];
      d1 += value[j*40+11] * src[col_idx[j]+1];
      d2 += value[j*40+21] * src[col_idx[j]+1];
      d3 += value[j*40+31] * src[col_idx[j]+1];
      d0 += value[j*40+2] * src[col_idx[j]+2];
      d1 += value[j*40+12] * src[col_idx[j]+2];
      d2 += value[j*40+22] * src[col_idx[j]+2];
      d3 += value[j*40+32] * src[col_idx[j]+2];
      d0 += value[j*40+3] * src[col_idx[j]+3];
      d1 += value[j*40+13] * src[col_idx[j]+3];
      d2 += value[j*40+23] * src[col_idx[j]+3];
      d3 += value[j*40+33] * src[col_idx[j]+3];
      d0 += value[j*40+4] * src[col_idx[j]+4];
      d1 += value[j*40+14] * src[col_idx[j]+4];
      d2 += value[j*40+24] * src[col_idx[j]+4];
      d3 += value[j*40+34] * src[col_idx[j]+4];
      d0 += value[j*40+5] * src[col_idx[j]+5];
      d1 += value[j*40+15] * src[col_idx[j]+5];
      d2 += value[j*40+25] * src[col_idx[j]+5];
      d3 += value[j*40+35] * src[col_idx[j]+5];
      d0 += value[j*40+6] * src[col_idx[j]+6];
      d1 += value[j*40+16] * src[col_idx[j]+6];
      d2 += value[j*40+26] * src[col_idx[j]+6];
      d3 += value[j*40+36] * src[col_idx[j]+6];
      d0 += value[j*40+7] * src[col_idx[j]+7];
      d1 += value[j*40+17] * src[col_idx[j]+7];
      d2 += value[j*40+27] * src[col_idx[j]+7];
      d3 += value[j*40+37] * src[col_idx[j]+7];
      d0 += value[j*40+8] * src[col_idx[j]+8];
      d1 += value[j*40+18] * src[col_idx[j]+8];
      d2 += value[j*40+28] * src[col_idx[j]+8];
      d3 += value[j*40+38] * src[col_idx[j]+8];
      d0 += value[j*40+9] * src[col_idx[j]+9];
      d1 += value[j*40+19] * src[col_idx[j]+9];
      d2 += value[j*40+29] * src[col_idx[j]+9];
      d3 += value[j*40+39] * src[col_idx[j]+9];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*44+0] * src[col_idx[j]+0];
      d1 += value[j*44+11] * src[col_idx[j]+0];
      d2 += value[j*44+22] * src[col_idx[j]+0];
      d3 += value[j*44+33] * src[col_idx[j]+0];
      d0 += value[j*44+1] * src[col_idx[j]+1];
      d1 += value[j*44+12] * src[col_idx[j]+1];
      d2 += value[j*44+23] * src[col_idx[j]+1];
      d3 += value[j*44+34] * src[col_idx[j]+1];
      d0 += value[j*44+2] * src[col_idx[j]+2];
      d1 += value[j*44+13] * src[col_idx[j]+2];
      d2 += value[j*44+24] * src[col_idx[j]+2];
      d3 += value[j*44+35] * src[col_idx[j]+2];
      d0 += value[j*44+3] * src[col_idx[j]+3];
      d1 += value[j*44+14] * src[col_idx[j]+3];
      d2 += value[j*44+25] * src[col_idx[j]+3];
      d3 += value[j*44+36] * src[col_idx[j]+3];
      d0 += value[j*44+4] * src[col_idx[j]+4];
      d1 += value[j*44+15] * src[col_idx[j]+4];
      d2 += value[j*44+26] * src[col_idx[j]+4];
      d3 += value[j*44+37] * src[col_idx[j]+4];
      d0 += value[j*44+5] * src[col_idx[j]+5];
      d1 += value[j*44+16] * src[col_idx[j]+5];
      d2 += value[j*44+27] * src[col_idx[j]+5];
      d3 += value[j*44+38] * src[col_idx[j]+5];
      d0 += value[j*44+6] * src[col_idx[j]+6];
      d1 += value[j*44+17] * src[col_idx[j]+6];
      d2 += value[j*44+28] * src[col_idx[j]+6];
      d3 += value[j*44+39] * src[col_idx[j]+6];
      d0 += value[j*44+7] * src[col_idx[j]+7];
      d1 += value[j*44+18] * src[col_idx[j]+7];
      d2 += value[j*44+29] * src[col_idx[j]+7];
      d3 += value[j*44+40] * src[col_idx[j]+7];
      d0 += value[j*44+8] * src[col_idx[j]+8];
      d1 += value[j*44+19] * src[col_idx[j]+8];
      d2 += value[j*44+30] * src[col_idx[j]+8];
      d3 += value[j*44+41] * src[col_idx[j]+8];
      d0 += value[j*44+9] * src[col_idx[j]+9];
      d1 += value[j*44+20] * src[col_idx[j]+9];
      d2 += value[j*44+31] * src[col_idx[j]+9];
      d3 += value[j*44+42] * src[col_idx[j]+9];
      d0 += value[j*44+10] * src[col_idx[j]+10];
      d1 += value[j*44+21] * src[col_idx[j]+10];
      d2 += value[j*44+32] * src[col_idx[j]+10];
      d3 += value[j*44+43] * src[col_idx[j]+10];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_4x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3;
    d0 = dest[4*i+0];
    d1 = dest[4*i+1];
    d2 = dest[4*i+2];
    d3 = dest[4*i+3];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*48+0] * src[col_idx[j]+0];
      d1 += value[j*48+12] * src[col_idx[j]+0];
      d2 += value[j*48+24] * src[col_idx[j]+0];
      d3 += value[j*48+36] * src[col_idx[j]+0];
      d0 += value[j*48+1] * src[col_idx[j]+1];
      d1 += value[j*48+13] * src[col_idx[j]+1];
      d2 += value[j*48+25] * src[col_idx[j]+1];
      d3 += value[j*48+37] * src[col_idx[j]+1];
      d0 += value[j*48+2] * src[col_idx[j]+2];
      d1 += value[j*48+14] * src[col_idx[j]+2];
      d2 += value[j*48+26] * src[col_idx[j]+2];
      d3 += value[j*48+38] * src[col_idx[j]+2];
      d0 += value[j*48+3] * src[col_idx[j]+3];
      d1 += value[j*48+15] * src[col_idx[j]+3];
      d2 += value[j*48+27] * src[col_idx[j]+3];
      d3 += value[j*48+39] * src[col_idx[j]+3];
      d0 += value[j*48+4] * src[col_idx[j]+4];
      d1 += value[j*48+16] * src[col_idx[j]+4];
      d2 += value[j*48+28] * src[col_idx[j]+4];
      d3 += value[j*48+40] * src[col_idx[j]+4];
      d0 += value[j*48+5] * src[col_idx[j]+5];
      d1 += value[j*48+17] * src[col_idx[j]+5];
      d2 += value[j*48+29] * src[col_idx[j]+5];
      d3 += value[j*48+41] * src[col_idx[j]+5];
      d0 += value[j*48+6] * src[col_idx[j]+6];
      d1 += value[j*48+18] * src[col_idx[j]+6];
      d2 += value[j*48+30] * src[col_idx[j]+6];
      d3 += value[j*48+42] * src[col_idx[j]+6];
      d0 += value[j*48+7] * src[col_idx[j]+7];
      d1 += value[j*48+19] * src[col_idx[j]+7];
      d2 += value[j*48+31] * src[col_idx[j]+7];
      d3 += value[j*48+43] * src[col_idx[j]+7];
      d0 += value[j*48+8] * src[col_idx[j]+8];
      d1 += value[j*48+20] * src[col_idx[j]+8];
      d2 += value[j*48+32] * src[col_idx[j]+8];
      d3 += value[j*48+44] * src[col_idx[j]+8];
      d0 += value[j*48+9] * src[col_idx[j]+9];
      d1 += value[j*48+21] * src[col_idx[j]+9];
      d2 += value[j*48+33] * src[col_idx[j]+9];
      d3 += value[j*48+45] * src[col_idx[j]+9];
      d0 += value[j*48+10] * src[col_idx[j]+10];
      d1 += value[j*48+22] * src[col_idx[j]+10];
      d2 += value[j*48+34] * src[col_idx[j]+10];
      d3 += value[j*48+46] * src[col_idx[j]+10];
      d0 += value[j*48+11] * src[col_idx[j]+11];
      d1 += value[j*48+23] * src[col_idx[j]+11];
      d2 += value[j*48+35] * src[col_idx[j]+11];
      d3 += value[j*48+47] * src[col_idx[j]+11];
    }
    dest[4*i+0] = d0;
    dest[4*i+1] = d1;
    dest[4*i+2] = d2;
    dest[4*i+3] = d3;
  }
}
void bsmvm_5x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*5+0] * src[col_idx[j]+0];
      d1 += value[j*5+1] * src[col_idx[j]+0];
      d2 += value[j*5+2] * src[col_idx[j]+0];
      d3 += value[j*5+3] * src[col_idx[j]+0];
      d4 += value[j*5+4] * src[col_idx[j]+0];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*10+0] * src[col_idx[j]+0];
      d1 += value[j*10+2] * src[col_idx[j]+0];
      d2 += value[j*10+4] * src[col_idx[j]+0];
      d3 += value[j*10+6] * src[col_idx[j]+0];
      d4 += value[j*10+8] * src[col_idx[j]+0];
      d0 += value[j*10+1] * src[col_idx[j]+1];
      d1 += value[j*10+3] * src[col_idx[j]+1];
      d2 += value[j*10+5] * src[col_idx[j]+1];
      d3 += value[j*10+7] * src[col_idx[j]+1];
      d4 += value[j*10+9] * src[col_idx[j]+1];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*15+0] * src[col_idx[j]+0];
      d1 += value[j*15+3] * src[col_idx[j]+0];
      d2 += value[j*15+6] * src[col_idx[j]+0];
      d3 += value[j*15+9] * src[col_idx[j]+0];
      d4 += value[j*15+12] * src[col_idx[j]+0];
      d0 += value[j*15+1] * src[col_idx[j]+1];
      d1 += value[j*15+4] * src[col_idx[j]+1];
      d2 += value[j*15+7] * src[col_idx[j]+1];
      d3 += value[j*15+10] * src[col_idx[j]+1];
      d4 += value[j*15+13] * src[col_idx[j]+1];
      d0 += value[j*15+2] * src[col_idx[j]+2];
      d1 += value[j*15+5] * src[col_idx[j]+2];
      d2 += value[j*15+8] * src[col_idx[j]+2];
      d3 += value[j*15+11] * src[col_idx[j]+2];
      d4 += value[j*15+14] * src[col_idx[j]+2];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*20+0] * src[col_idx[j]+0];
      d1 += value[j*20+4] * src[col_idx[j]+0];
      d2 += value[j*20+8] * src[col_idx[j]+0];
      d3 += value[j*20+12] * src[col_idx[j]+0];
      d4 += value[j*20+16] * src[col_idx[j]+0];
      d0 += value[j*20+1] * src[col_idx[j]+1];
      d1 += value[j*20+5] * src[col_idx[j]+1];
      d2 += value[j*20+9] * src[col_idx[j]+1];
      d3 += value[j*20+13] * src[col_idx[j]+1];
      d4 += value[j*20+17] * src[col_idx[j]+1];
      d0 += value[j*20+2] * src[col_idx[j]+2];
      d1 += value[j*20+6] * src[col_idx[j]+2];
      d2 += value[j*20+10] * src[col_idx[j]+2];
      d3 += value[j*20+14] * src[col_idx[j]+2];
      d4 += value[j*20+18] * src[col_idx[j]+2];
      d0 += value[j*20+3] * src[col_idx[j]+3];
      d1 += value[j*20+7] * src[col_idx[j]+3];
      d2 += value[j*20+11] * src[col_idx[j]+3];
      d3 += value[j*20+15] * src[col_idx[j]+3];
      d4 += value[j*20+19] * src[col_idx[j]+3];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*25+0] * src[col_idx[j]+0];
      d1 += value[j*25+5] * src[col_idx[j]+0];
      d2 += value[j*25+10] * src[col_idx[j]+0];
      d3 += value[j*25+15] * src[col_idx[j]+0];
      d4 += value[j*25+20] * src[col_idx[j]+0];
      d0 += value[j*25+1] * src[col_idx[j]+1];
      d1 += value[j*25+6] * src[col_idx[j]+1];
      d2 += value[j*25+11] * src[col_idx[j]+1];
      d3 += value[j*25+16] * src[col_idx[j]+1];
      d4 += value[j*25+21] * src[col_idx[j]+1];
      d0 += value[j*25+2] * src[col_idx[j]+2];
      d1 += value[j*25+7] * src[col_idx[j]+2];
      d2 += value[j*25+12] * src[col_idx[j]+2];
      d3 += value[j*25+17] * src[col_idx[j]+2];
      d4 += value[j*25+22] * src[col_idx[j]+2];
      d0 += value[j*25+3] * src[col_idx[j]+3];
      d1 += value[j*25+8] * src[col_idx[j]+3];
      d2 += value[j*25+13] * src[col_idx[j]+3];
      d3 += value[j*25+18] * src[col_idx[j]+3];
      d4 += value[j*25+23] * src[col_idx[j]+3];
      d0 += value[j*25+4] * src[col_idx[j]+4];
      d1 += value[j*25+9] * src[col_idx[j]+4];
      d2 += value[j*25+14] * src[col_idx[j]+4];
      d3 += value[j*25+19] * src[col_idx[j]+4];
      d4 += value[j*25+24] * src[col_idx[j]+4];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*30+0] * src[col_idx[j]+0];
      d1 += value[j*30+6] * src[col_idx[j]+0];
      d2 += value[j*30+12] * src[col_idx[j]+0];
      d3 += value[j*30+18] * src[col_idx[j]+0];
      d4 += value[j*30+24] * src[col_idx[j]+0];
      d0 += value[j*30+1] * src[col_idx[j]+1];
      d1 += value[j*30+7] * src[col_idx[j]+1];
      d2 += value[j*30+13] * src[col_idx[j]+1];
      d3 += value[j*30+19] * src[col_idx[j]+1];
      d4 += value[j*30+25] * src[col_idx[j]+1];
      d0 += value[j*30+2] * src[col_idx[j]+2];
      d1 += value[j*30+8] * src[col_idx[j]+2];
      d2 += value[j*30+14] * src[col_idx[j]+2];
      d3 += value[j*30+20] * src[col_idx[j]+2];
      d4 += value[j*30+26] * src[col_idx[j]+2];
      d0 += value[j*30+3] * src[col_idx[j]+3];
      d1 += value[j*30+9] * src[col_idx[j]+3];
      d2 += value[j*30+15] * src[col_idx[j]+3];
      d3 += value[j*30+21] * src[col_idx[j]+3];
      d4 += value[j*30+27] * src[col_idx[j]+3];
      d0 += value[j*30+4] * src[col_idx[j]+4];
      d1 += value[j*30+10] * src[col_idx[j]+4];
      d2 += value[j*30+16] * src[col_idx[j]+4];
      d3 += value[j*30+22] * src[col_idx[j]+4];
      d4 += value[j*30+28] * src[col_idx[j]+4];
      d0 += value[j*30+5] * src[col_idx[j]+5];
      d1 += value[j*30+11] * src[col_idx[j]+5];
      d2 += value[j*30+17] * src[col_idx[j]+5];
      d3 += value[j*30+23] * src[col_idx[j]+5];
      d4 += value[j*30+29] * src[col_idx[j]+5];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*35+0] * src[col_idx[j]+0];
      d1 += value[j*35+7] * src[col_idx[j]+0];
      d2 += value[j*35+14] * src[col_idx[j]+0];
      d3 += value[j*35+21] * src[col_idx[j]+0];
      d4 += value[j*35+28] * src[col_idx[j]+0];
      d0 += value[j*35+1] * src[col_idx[j]+1];
      d1 += value[j*35+8] * src[col_idx[j]+1];
      d2 += value[j*35+15] * src[col_idx[j]+1];
      d3 += value[j*35+22] * src[col_idx[j]+1];
      d4 += value[j*35+29] * src[col_idx[j]+1];
      d0 += value[j*35+2] * src[col_idx[j]+2];
      d1 += value[j*35+9] * src[col_idx[j]+2];
      d2 += value[j*35+16] * src[col_idx[j]+2];
      d3 += value[j*35+23] * src[col_idx[j]+2];
      d4 += value[j*35+30] * src[col_idx[j]+2];
      d0 += value[j*35+3] * src[col_idx[j]+3];
      d1 += value[j*35+10] * src[col_idx[j]+3];
      d2 += value[j*35+17] * src[col_idx[j]+3];
      d3 += value[j*35+24] * src[col_idx[j]+3];
      d4 += value[j*35+31] * src[col_idx[j]+3];
      d0 += value[j*35+4] * src[col_idx[j]+4];
      d1 += value[j*35+11] * src[col_idx[j]+4];
      d2 += value[j*35+18] * src[col_idx[j]+4];
      d3 += value[j*35+25] * src[col_idx[j]+4];
      d4 += value[j*35+32] * src[col_idx[j]+4];
      d0 += value[j*35+5] * src[col_idx[j]+5];
      d1 += value[j*35+12] * src[col_idx[j]+5];
      d2 += value[j*35+19] * src[col_idx[j]+5];
      d3 += value[j*35+26] * src[col_idx[j]+5];
      d4 += value[j*35+33] * src[col_idx[j]+5];
      d0 += value[j*35+6] * src[col_idx[j]+6];
      d1 += value[j*35+13] * src[col_idx[j]+6];
      d2 += value[j*35+20] * src[col_idx[j]+6];
      d3 += value[j*35+27] * src[col_idx[j]+6];
      d4 += value[j*35+34] * src[col_idx[j]+6];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*40+0] * src[col_idx[j]+0];
      d1 += value[j*40+8] * src[col_idx[j]+0];
      d2 += value[j*40+16] * src[col_idx[j]+0];
      d3 += value[j*40+24] * src[col_idx[j]+0];
      d4 += value[j*40+32] * src[col_idx[j]+0];
      d0 += value[j*40+1] * src[col_idx[j]+1];
      d1 += value[j*40+9] * src[col_idx[j]+1];
      d2 += value[j*40+17] * src[col_idx[j]+1];
      d3 += value[j*40+25] * src[col_idx[j]+1];
      d4 += value[j*40+33] * src[col_idx[j]+1];
      d0 += value[j*40+2] * src[col_idx[j]+2];
      d1 += value[j*40+10] * src[col_idx[j]+2];
      d2 += value[j*40+18] * src[col_idx[j]+2];
      d3 += value[j*40+26] * src[col_idx[j]+2];
      d4 += value[j*40+34] * src[col_idx[j]+2];
      d0 += value[j*40+3] * src[col_idx[j]+3];
      d1 += value[j*40+11] * src[col_idx[j]+3];
      d2 += value[j*40+19] * src[col_idx[j]+3];
      d3 += value[j*40+27] * src[col_idx[j]+3];
      d4 += value[j*40+35] * src[col_idx[j]+3];
      d0 += value[j*40+4] * src[col_idx[j]+4];
      d1 += value[j*40+12] * src[col_idx[j]+4];
      d2 += value[j*40+20] * src[col_idx[j]+4];
      d3 += value[j*40+28] * src[col_idx[j]+4];
      d4 += value[j*40+36] * src[col_idx[j]+4];
      d0 += value[j*40+5] * src[col_idx[j]+5];
      d1 += value[j*40+13] * src[col_idx[j]+5];
      d2 += value[j*40+21] * src[col_idx[j]+5];
      d3 += value[j*40+29] * src[col_idx[j]+5];
      d4 += value[j*40+37] * src[col_idx[j]+5];
      d0 += value[j*40+6] * src[col_idx[j]+6];
      d1 += value[j*40+14] * src[col_idx[j]+6];
      d2 += value[j*40+22] * src[col_idx[j]+6];
      d3 += value[j*40+30] * src[col_idx[j]+6];
      d4 += value[j*40+38] * src[col_idx[j]+6];
      d0 += value[j*40+7] * src[col_idx[j]+7];
      d1 += value[j*40+15] * src[col_idx[j]+7];
      d2 += value[j*40+23] * src[col_idx[j]+7];
      d3 += value[j*40+31] * src[col_idx[j]+7];
      d4 += value[j*40+39] * src[col_idx[j]+7];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*45+0] * src[col_idx[j]+0];
      d1 += value[j*45+9] * src[col_idx[j]+0];
      d2 += value[j*45+18] * src[col_idx[j]+0];
      d3 += value[j*45+27] * src[col_idx[j]+0];
      d4 += value[j*45+36] * src[col_idx[j]+0];
      d0 += value[j*45+1] * src[col_idx[j]+1];
      d1 += value[j*45+10] * src[col_idx[j]+1];
      d2 += value[j*45+19] * src[col_idx[j]+1];
      d3 += value[j*45+28] * src[col_idx[j]+1];
      d4 += value[j*45+37] * src[col_idx[j]+1];
      d0 += value[j*45+2] * src[col_idx[j]+2];
      d1 += value[j*45+11] * src[col_idx[j]+2];
      d2 += value[j*45+20] * src[col_idx[j]+2];
      d3 += value[j*45+29] * src[col_idx[j]+2];
      d4 += value[j*45+38] * src[col_idx[j]+2];
      d0 += value[j*45+3] * src[col_idx[j]+3];
      d1 += value[j*45+12] * src[col_idx[j]+3];
      d2 += value[j*45+21] * src[col_idx[j]+3];
      d3 += value[j*45+30] * src[col_idx[j]+3];
      d4 += value[j*45+39] * src[col_idx[j]+3];
      d0 += value[j*45+4] * src[col_idx[j]+4];
      d1 += value[j*45+13] * src[col_idx[j]+4];
      d2 += value[j*45+22] * src[col_idx[j]+4];
      d3 += value[j*45+31] * src[col_idx[j]+4];
      d4 += value[j*45+40] * src[col_idx[j]+4];
      d0 += value[j*45+5] * src[col_idx[j]+5];
      d1 += value[j*45+14] * src[col_idx[j]+5];
      d2 += value[j*45+23] * src[col_idx[j]+5];
      d3 += value[j*45+32] * src[col_idx[j]+5];
      d4 += value[j*45+41] * src[col_idx[j]+5];
      d0 += value[j*45+6] * src[col_idx[j]+6];
      d1 += value[j*45+15] * src[col_idx[j]+6];
      d2 += value[j*45+24] * src[col_idx[j]+6];
      d3 += value[j*45+33] * src[col_idx[j]+6];
      d4 += value[j*45+42] * src[col_idx[j]+6];
      d0 += value[j*45+7] * src[col_idx[j]+7];
      d1 += value[j*45+16] * src[col_idx[j]+7];
      d2 += value[j*45+25] * src[col_idx[j]+7];
      d3 += value[j*45+34] * src[col_idx[j]+7];
      d4 += value[j*45+43] * src[col_idx[j]+7];
      d0 += value[j*45+8] * src[col_idx[j]+8];
      d1 += value[j*45+17] * src[col_idx[j]+8];
      d2 += value[j*45+26] * src[col_idx[j]+8];
      d3 += value[j*45+35] * src[col_idx[j]+8];
      d4 += value[j*45+44] * src[col_idx[j]+8];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*50+0] * src[col_idx[j]+0];
      d1 += value[j*50+10] * src[col_idx[j]+0];
      d2 += value[j*50+20] * src[col_idx[j]+0];
      d3 += value[j*50+30] * src[col_idx[j]+0];
      d4 += value[j*50+40] * src[col_idx[j]+0];
      d0 += value[j*50+1] * src[col_idx[j]+1];
      d1 += value[j*50+11] * src[col_idx[j]+1];
      d2 += value[j*50+21] * src[col_idx[j]+1];
      d3 += value[j*50+31] * src[col_idx[j]+1];
      d4 += value[j*50+41] * src[col_idx[j]+1];
      d0 += value[j*50+2] * src[col_idx[j]+2];
      d1 += value[j*50+12] * src[col_idx[j]+2];
      d2 += value[j*50+22] * src[col_idx[j]+2];
      d3 += value[j*50+32] * src[col_idx[j]+2];
      d4 += value[j*50+42] * src[col_idx[j]+2];
      d0 += value[j*50+3] * src[col_idx[j]+3];
      d1 += value[j*50+13] * src[col_idx[j]+3];
      d2 += value[j*50+23] * src[col_idx[j]+3];
      d3 += value[j*50+33] * src[col_idx[j]+3];
      d4 += value[j*50+43] * src[col_idx[j]+3];
      d0 += value[j*50+4] * src[col_idx[j]+4];
      d1 += value[j*50+14] * src[col_idx[j]+4];
      d2 += value[j*50+24] * src[col_idx[j]+4];
      d3 += value[j*50+34] * src[col_idx[j]+4];
      d4 += value[j*50+44] * src[col_idx[j]+4];
      d0 += value[j*50+5] * src[col_idx[j]+5];
      d1 += value[j*50+15] * src[col_idx[j]+5];
      d2 += value[j*50+25] * src[col_idx[j]+5];
      d3 += value[j*50+35] * src[col_idx[j]+5];
      d4 += value[j*50+45] * src[col_idx[j]+5];
      d0 += value[j*50+6] * src[col_idx[j]+6];
      d1 += value[j*50+16] * src[col_idx[j]+6];
      d2 += value[j*50+26] * src[col_idx[j]+6];
      d3 += value[j*50+36] * src[col_idx[j]+6];
      d4 += value[j*50+46] * src[col_idx[j]+6];
      d0 += value[j*50+7] * src[col_idx[j]+7];
      d1 += value[j*50+17] * src[col_idx[j]+7];
      d2 += value[j*50+27] * src[col_idx[j]+7];
      d3 += value[j*50+37] * src[col_idx[j]+7];
      d4 += value[j*50+47] * src[col_idx[j]+7];
      d0 += value[j*50+8] * src[col_idx[j]+8];
      d1 += value[j*50+18] * src[col_idx[j]+8];
      d2 += value[j*50+28] * src[col_idx[j]+8];
      d3 += value[j*50+38] * src[col_idx[j]+8];
      d4 += value[j*50+48] * src[col_idx[j]+8];
      d0 += value[j*50+9] * src[col_idx[j]+9];
      d1 += value[j*50+19] * src[col_idx[j]+9];
      d2 += value[j*50+29] * src[col_idx[j]+9];
      d3 += value[j*50+39] * src[col_idx[j]+9];
      d4 += value[j*50+49] * src[col_idx[j]+9];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*55+0] * src[col_idx[j]+0];
      d1 += value[j*55+11] * src[col_idx[j]+0];
      d2 += value[j*55+22] * src[col_idx[j]+0];
      d3 += value[j*55+33] * src[col_idx[j]+0];
      d4 += value[j*55+44] * src[col_idx[j]+0];
      d0 += value[j*55+1] * src[col_idx[j]+1];
      d1 += value[j*55+12] * src[col_idx[j]+1];
      d2 += value[j*55+23] * src[col_idx[j]+1];
      d3 += value[j*55+34] * src[col_idx[j]+1];
      d4 += value[j*55+45] * src[col_idx[j]+1];
      d0 += value[j*55+2] * src[col_idx[j]+2];
      d1 += value[j*55+13] * src[col_idx[j]+2];
      d2 += value[j*55+24] * src[col_idx[j]+2];
      d3 += value[j*55+35] * src[col_idx[j]+2];
      d4 += value[j*55+46] * src[col_idx[j]+2];
      d0 += value[j*55+3] * src[col_idx[j]+3];
      d1 += value[j*55+14] * src[col_idx[j]+3];
      d2 += value[j*55+25] * src[col_idx[j]+3];
      d3 += value[j*55+36] * src[col_idx[j]+3];
      d4 += value[j*55+47] * src[col_idx[j]+3];
      d0 += value[j*55+4] * src[col_idx[j]+4];
      d1 += value[j*55+15] * src[col_idx[j]+4];
      d2 += value[j*55+26] * src[col_idx[j]+4];
      d3 += value[j*55+37] * src[col_idx[j]+4];
      d4 += value[j*55+48] * src[col_idx[j]+4];
      d0 += value[j*55+5] * src[col_idx[j]+5];
      d1 += value[j*55+16] * src[col_idx[j]+5];
      d2 += value[j*55+27] * src[col_idx[j]+5];
      d3 += value[j*55+38] * src[col_idx[j]+5];
      d4 += value[j*55+49] * src[col_idx[j]+5];
      d0 += value[j*55+6] * src[col_idx[j]+6];
      d1 += value[j*55+17] * src[col_idx[j]+6];
      d2 += value[j*55+28] * src[col_idx[j]+6];
      d3 += value[j*55+39] * src[col_idx[j]+6];
      d4 += value[j*55+50] * src[col_idx[j]+6];
      d0 += value[j*55+7] * src[col_idx[j]+7];
      d1 += value[j*55+18] * src[col_idx[j]+7];
      d2 += value[j*55+29] * src[col_idx[j]+7];
      d3 += value[j*55+40] * src[col_idx[j]+7];
      d4 += value[j*55+51] * src[col_idx[j]+7];
      d0 += value[j*55+8] * src[col_idx[j]+8];
      d1 += value[j*55+19] * src[col_idx[j]+8];
      d2 += value[j*55+30] * src[col_idx[j]+8];
      d3 += value[j*55+41] * src[col_idx[j]+8];
      d4 += value[j*55+52] * src[col_idx[j]+8];
      d0 += value[j*55+9] * src[col_idx[j]+9];
      d1 += value[j*55+20] * src[col_idx[j]+9];
      d2 += value[j*55+31] * src[col_idx[j]+9];
      d3 += value[j*55+42] * src[col_idx[j]+9];
      d4 += value[j*55+53] * src[col_idx[j]+9];
      d0 += value[j*55+10] * src[col_idx[j]+10];
      d1 += value[j*55+21] * src[col_idx[j]+10];
      d2 += value[j*55+32] * src[col_idx[j]+10];
      d3 += value[j*55+43] * src[col_idx[j]+10];
      d4 += value[j*55+54] * src[col_idx[j]+10];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_5x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4;
    d0 = dest[5*i+0];
    d1 = dest[5*i+1];
    d2 = dest[5*i+2];
    d3 = dest[5*i+3];
    d4 = dest[5*i+4];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*60+0] * src[col_idx[j]+0];
      d1 += value[j*60+12] * src[col_idx[j]+0];
      d2 += value[j*60+24] * src[col_idx[j]+0];
      d3 += value[j*60+36] * src[col_idx[j]+0];
      d4 += value[j*60+48] * src[col_idx[j]+0];
      d0 += value[j*60+1] * src[col_idx[j]+1];
      d1 += value[j*60+13] * src[col_idx[j]+1];
      d2 += value[j*60+25] * src[col_idx[j]+1];
      d3 += value[j*60+37] * src[col_idx[j]+1];
      d4 += value[j*60+49] * src[col_idx[j]+1];
      d0 += value[j*60+2] * src[col_idx[j]+2];
      d1 += value[j*60+14] * src[col_idx[j]+2];
      d2 += value[j*60+26] * src[col_idx[j]+2];
      d3 += value[j*60+38] * src[col_idx[j]+2];
      d4 += value[j*60+50] * src[col_idx[j]+2];
      d0 += value[j*60+3] * src[col_idx[j]+3];
      d1 += value[j*60+15] * src[col_idx[j]+3];
      d2 += value[j*60+27] * src[col_idx[j]+3];
      d3 += value[j*60+39] * src[col_idx[j]+3];
      d4 += value[j*60+51] * src[col_idx[j]+3];
      d0 += value[j*60+4] * src[col_idx[j]+4];
      d1 += value[j*60+16] * src[col_idx[j]+4];
      d2 += value[j*60+28] * src[col_idx[j]+4];
      d3 += value[j*60+40] * src[col_idx[j]+4];
      d4 += value[j*60+52] * src[col_idx[j]+4];
      d0 += value[j*60+5] * src[col_idx[j]+5];
      d1 += value[j*60+17] * src[col_idx[j]+5];
      d2 += value[j*60+29] * src[col_idx[j]+5];
      d3 += value[j*60+41] * src[col_idx[j]+5];
      d4 += value[j*60+53] * src[col_idx[j]+5];
      d0 += value[j*60+6] * src[col_idx[j]+6];
      d1 += value[j*60+18] * src[col_idx[j]+6];
      d2 += value[j*60+30] * src[col_idx[j]+6];
      d3 += value[j*60+42] * src[col_idx[j]+6];
      d4 += value[j*60+54] * src[col_idx[j]+6];
      d0 += value[j*60+7] * src[col_idx[j]+7];
      d1 += value[j*60+19] * src[col_idx[j]+7];
      d2 += value[j*60+31] * src[col_idx[j]+7];
      d3 += value[j*60+43] * src[col_idx[j]+7];
      d4 += value[j*60+55] * src[col_idx[j]+7];
      d0 += value[j*60+8] * src[col_idx[j]+8];
      d1 += value[j*60+20] * src[col_idx[j]+8];
      d2 += value[j*60+32] * src[col_idx[j]+8];
      d3 += value[j*60+44] * src[col_idx[j]+8];
      d4 += value[j*60+56] * src[col_idx[j]+8];
      d0 += value[j*60+9] * src[col_idx[j]+9];
      d1 += value[j*60+21] * src[col_idx[j]+9];
      d2 += value[j*60+33] * src[col_idx[j]+9];
      d3 += value[j*60+45] * src[col_idx[j]+9];
      d4 += value[j*60+57] * src[col_idx[j]+9];
      d0 += value[j*60+10] * src[col_idx[j]+10];
      d1 += value[j*60+22] * src[col_idx[j]+10];
      d2 += value[j*60+34] * src[col_idx[j]+10];
      d3 += value[j*60+46] * src[col_idx[j]+10];
      d4 += value[j*60+58] * src[col_idx[j]+10];
      d0 += value[j*60+11] * src[col_idx[j]+11];
      d1 += value[j*60+23] * src[col_idx[j]+11];
      d2 += value[j*60+35] * src[col_idx[j]+11];
      d3 += value[j*60+47] * src[col_idx[j]+11];
      d4 += value[j*60+59] * src[col_idx[j]+11];
    }
    dest[5*i+0] = d0;
    dest[5*i+1] = d1;
    dest[5*i+2] = d2;
    dest[5*i+3] = d3;
    dest[5*i+4] = d4;
  }
}
void bsmvm_6x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*6+0] * src[col_idx[j]+0];
      d1 += value[j*6+1] * src[col_idx[j]+0];
      d2 += value[j*6+2] * src[col_idx[j]+0];
      d3 += value[j*6+3] * src[col_idx[j]+0];
      d4 += value[j*6+4] * src[col_idx[j]+0];
      d5 += value[j*6+5] * src[col_idx[j]+0];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d1 += value[j*12+2] * src[col_idx[j]+0];
      d2 += value[j*12+4] * src[col_idx[j]+0];
      d3 += value[j*12+6] * src[col_idx[j]+0];
      d4 += value[j*12+8] * src[col_idx[j]+0];
      d5 += value[j*12+10] * src[col_idx[j]+0];
      d0 += value[j*12+1] * src[col_idx[j]+1];
      d1 += value[j*12+3] * src[col_idx[j]+1];
      d2 += value[j*12+5] * src[col_idx[j]+1];
      d3 += value[j*12+7] * src[col_idx[j]+1];
      d4 += value[j*12+9] * src[col_idx[j]+1];
      d5 += value[j*12+11] * src[col_idx[j]+1];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*18+0] * src[col_idx[j]+0];
      d1 += value[j*18+3] * src[col_idx[j]+0];
      d2 += value[j*18+6] * src[col_idx[j]+0];
      d3 += value[j*18+9] * src[col_idx[j]+0];
      d4 += value[j*18+12] * src[col_idx[j]+0];
      d5 += value[j*18+15] * src[col_idx[j]+0];
      d0 += value[j*18+1] * src[col_idx[j]+1];
      d1 += value[j*18+4] * src[col_idx[j]+1];
      d2 += value[j*18+7] * src[col_idx[j]+1];
      d3 += value[j*18+10] * src[col_idx[j]+1];
      d4 += value[j*18+13] * src[col_idx[j]+1];
      d5 += value[j*18+16] * src[col_idx[j]+1];
      d0 += value[j*18+2] * src[col_idx[j]+2];
      d1 += value[j*18+5] * src[col_idx[j]+2];
      d2 += value[j*18+8] * src[col_idx[j]+2];
      d3 += value[j*18+11] * src[col_idx[j]+2];
      d4 += value[j*18+14] * src[col_idx[j]+2];
      d5 += value[j*18+17] * src[col_idx[j]+2];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+4] * src[col_idx[j]+0];
      d2 += value[j*24+8] * src[col_idx[j]+0];
      d3 += value[j*24+12] * src[col_idx[j]+0];
      d4 += value[j*24+16] * src[col_idx[j]+0];
      d5 += value[j*24+20] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+5] * src[col_idx[j]+1];
      d2 += value[j*24+9] * src[col_idx[j]+1];
      d3 += value[j*24+13] * src[col_idx[j]+1];
      d4 += value[j*24+17] * src[col_idx[j]+1];
      d5 += value[j*24+21] * src[col_idx[j]+1];
      d0 += value[j*24+2] * src[col_idx[j]+2];
      d1 += value[j*24+6] * src[col_idx[j]+2];
      d2 += value[j*24+10] * src[col_idx[j]+2];
      d3 += value[j*24+14] * src[col_idx[j]+2];
      d4 += value[j*24+18] * src[col_idx[j]+2];
      d5 += value[j*24+22] * src[col_idx[j]+2];
      d0 += value[j*24+3] * src[col_idx[j]+3];
      d1 += value[j*24+7] * src[col_idx[j]+3];
      d2 += value[j*24+11] * src[col_idx[j]+3];
      d3 += value[j*24+15] * src[col_idx[j]+3];
      d4 += value[j*24+19] * src[col_idx[j]+3];
      d5 += value[j*24+23] * src[col_idx[j]+3];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*30+0] * src[col_idx[j]+0];
      d1 += value[j*30+5] * src[col_idx[j]+0];
      d2 += value[j*30+10] * src[col_idx[j]+0];
      d3 += value[j*30+15] * src[col_idx[j]+0];
      d4 += value[j*30+20] * src[col_idx[j]+0];
      d5 += value[j*30+25] * src[col_idx[j]+0];
      d0 += value[j*30+1] * src[col_idx[j]+1];
      d1 += value[j*30+6] * src[col_idx[j]+1];
      d2 += value[j*30+11] * src[col_idx[j]+1];
      d3 += value[j*30+16] * src[col_idx[j]+1];
      d4 += value[j*30+21] * src[col_idx[j]+1];
      d5 += value[j*30+26] * src[col_idx[j]+1];
      d0 += value[j*30+2] * src[col_idx[j]+2];
      d1 += value[j*30+7] * src[col_idx[j]+2];
      d2 += value[j*30+12] * src[col_idx[j]+2];
      d3 += value[j*30+17] * src[col_idx[j]+2];
      d4 += value[j*30+22] * src[col_idx[j]+2];
      d5 += value[j*30+27] * src[col_idx[j]+2];
      d0 += value[j*30+3] * src[col_idx[j]+3];
      d1 += value[j*30+8] * src[col_idx[j]+3];
      d2 += value[j*30+13] * src[col_idx[j]+3];
      d3 += value[j*30+18] * src[col_idx[j]+3];
      d4 += value[j*30+23] * src[col_idx[j]+3];
      d5 += value[j*30+28] * src[col_idx[j]+3];
      d0 += value[j*30+4] * src[col_idx[j]+4];
      d1 += value[j*30+9] * src[col_idx[j]+4];
      d2 += value[j*30+14] * src[col_idx[j]+4];
      d3 += value[j*30+19] * src[col_idx[j]+4];
      d4 += value[j*30+24] * src[col_idx[j]+4];
      d5 += value[j*30+29] * src[col_idx[j]+4];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*36+0] * src[col_idx[j]+0];
      d1 += value[j*36+6] * src[col_idx[j]+0];
      d2 += value[j*36+12] * src[col_idx[j]+0];
      d3 += value[j*36+18] * src[col_idx[j]+0];
      d4 += value[j*36+24] * src[col_idx[j]+0];
      d5 += value[j*36+30] * src[col_idx[j]+0];
      d0 += value[j*36+1] * src[col_idx[j]+1];
      d1 += value[j*36+7] * src[col_idx[j]+1];
      d2 += value[j*36+13] * src[col_idx[j]+1];
      d3 += value[j*36+19] * src[col_idx[j]+1];
      d4 += value[j*36+25] * src[col_idx[j]+1];
      d5 += value[j*36+31] * src[col_idx[j]+1];
      d0 += value[j*36+2] * src[col_idx[j]+2];
      d1 += value[j*36+8] * src[col_idx[j]+2];
      d2 += value[j*36+14] * src[col_idx[j]+2];
      d3 += value[j*36+20] * src[col_idx[j]+2];
      d4 += value[j*36+26] * src[col_idx[j]+2];
      d5 += value[j*36+32] * src[col_idx[j]+2];
      d0 += value[j*36+3] * src[col_idx[j]+3];
      d1 += value[j*36+9] * src[col_idx[j]+3];
      d2 += value[j*36+15] * src[col_idx[j]+3];
      d3 += value[j*36+21] * src[col_idx[j]+3];
      d4 += value[j*36+27] * src[col_idx[j]+3];
      d5 += value[j*36+33] * src[col_idx[j]+3];
      d0 += value[j*36+4] * src[col_idx[j]+4];
      d1 += value[j*36+10] * src[col_idx[j]+4];
      d2 += value[j*36+16] * src[col_idx[j]+4];
      d3 += value[j*36+22] * src[col_idx[j]+4];
      d4 += value[j*36+28] * src[col_idx[j]+4];
      d5 += value[j*36+34] * src[col_idx[j]+4];
      d0 += value[j*36+5] * src[col_idx[j]+5];
      d1 += value[j*36+11] * src[col_idx[j]+5];
      d2 += value[j*36+17] * src[col_idx[j]+5];
      d3 += value[j*36+23] * src[col_idx[j]+5];
      d4 += value[j*36+29] * src[col_idx[j]+5];
      d5 += value[j*36+35] * src[col_idx[j]+5];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*42+0] * src[col_idx[j]+0];
      d1 += value[j*42+7] * src[col_idx[j]+0];
      d2 += value[j*42+14] * src[col_idx[j]+0];
      d3 += value[j*42+21] * src[col_idx[j]+0];
      d4 += value[j*42+28] * src[col_idx[j]+0];
      d5 += value[j*42+35] * src[col_idx[j]+0];
      d0 += value[j*42+1] * src[col_idx[j]+1];
      d1 += value[j*42+8] * src[col_idx[j]+1];
      d2 += value[j*42+15] * src[col_idx[j]+1];
      d3 += value[j*42+22] * src[col_idx[j]+1];
      d4 += value[j*42+29] * src[col_idx[j]+1];
      d5 += value[j*42+36] * src[col_idx[j]+1];
      d0 += value[j*42+2] * src[col_idx[j]+2];
      d1 += value[j*42+9] * src[col_idx[j]+2];
      d2 += value[j*42+16] * src[col_idx[j]+2];
      d3 += value[j*42+23] * src[col_idx[j]+2];
      d4 += value[j*42+30] * src[col_idx[j]+2];
      d5 += value[j*42+37] * src[col_idx[j]+2];
      d0 += value[j*42+3] * src[col_idx[j]+3];
      d1 += value[j*42+10] * src[col_idx[j]+3];
      d2 += value[j*42+17] * src[col_idx[j]+3];
      d3 += value[j*42+24] * src[col_idx[j]+3];
      d4 += value[j*42+31] * src[col_idx[j]+3];
      d5 += value[j*42+38] * src[col_idx[j]+3];
      d0 += value[j*42+4] * src[col_idx[j]+4];
      d1 += value[j*42+11] * src[col_idx[j]+4];
      d2 += value[j*42+18] * src[col_idx[j]+4];
      d3 += value[j*42+25] * src[col_idx[j]+4];
      d4 += value[j*42+32] * src[col_idx[j]+4];
      d5 += value[j*42+39] * src[col_idx[j]+4];
      d0 += value[j*42+5] * src[col_idx[j]+5];
      d1 += value[j*42+12] * src[col_idx[j]+5];
      d2 += value[j*42+19] * src[col_idx[j]+5];
      d3 += value[j*42+26] * src[col_idx[j]+5];
      d4 += value[j*42+33] * src[col_idx[j]+5];
      d5 += value[j*42+40] * src[col_idx[j]+5];
      d0 += value[j*42+6] * src[col_idx[j]+6];
      d1 += value[j*42+13] * src[col_idx[j]+6];
      d2 += value[j*42+20] * src[col_idx[j]+6];
      d3 += value[j*42+27] * src[col_idx[j]+6];
      d4 += value[j*42+34] * src[col_idx[j]+6];
      d5 += value[j*42+41] * src[col_idx[j]+6];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*48+0] * src[col_idx[j]+0];
      d1 += value[j*48+8] * src[col_idx[j]+0];
      d2 += value[j*48+16] * src[col_idx[j]+0];
      d3 += value[j*48+24] * src[col_idx[j]+0];
      d4 += value[j*48+32] * src[col_idx[j]+0];
      d5 += value[j*48+40] * src[col_idx[j]+0];
      d0 += value[j*48+1] * src[col_idx[j]+1];
      d1 += value[j*48+9] * src[col_idx[j]+1];
      d2 += value[j*48+17] * src[col_idx[j]+1];
      d3 += value[j*48+25] * src[col_idx[j]+1];
      d4 += value[j*48+33] * src[col_idx[j]+1];
      d5 += value[j*48+41] * src[col_idx[j]+1];
      d0 += value[j*48+2] * src[col_idx[j]+2];
      d1 += value[j*48+10] * src[col_idx[j]+2];
      d2 += value[j*48+18] * src[col_idx[j]+2];
      d3 += value[j*48+26] * src[col_idx[j]+2];
      d4 += value[j*48+34] * src[col_idx[j]+2];
      d5 += value[j*48+42] * src[col_idx[j]+2];
      d0 += value[j*48+3] * src[col_idx[j]+3];
      d1 += value[j*48+11] * src[col_idx[j]+3];
      d2 += value[j*48+19] * src[col_idx[j]+3];
      d3 += value[j*48+27] * src[col_idx[j]+3];
      d4 += value[j*48+35] * src[col_idx[j]+3];
      d5 += value[j*48+43] * src[col_idx[j]+3];
      d0 += value[j*48+4] * src[col_idx[j]+4];
      d1 += value[j*48+12] * src[col_idx[j]+4];
      d2 += value[j*48+20] * src[col_idx[j]+4];
      d3 += value[j*48+28] * src[col_idx[j]+4];
      d4 += value[j*48+36] * src[col_idx[j]+4];
      d5 += value[j*48+44] * src[col_idx[j]+4];
      d0 += value[j*48+5] * src[col_idx[j]+5];
      d1 += value[j*48+13] * src[col_idx[j]+5];
      d2 += value[j*48+21] * src[col_idx[j]+5];
      d3 += value[j*48+29] * src[col_idx[j]+5];
      d4 += value[j*48+37] * src[col_idx[j]+5];
      d5 += value[j*48+45] * src[col_idx[j]+5];
      d0 += value[j*48+6] * src[col_idx[j]+6];
      d1 += value[j*48+14] * src[col_idx[j]+6];
      d2 += value[j*48+22] * src[col_idx[j]+6];
      d3 += value[j*48+30] * src[col_idx[j]+6];
      d4 += value[j*48+38] * src[col_idx[j]+6];
      d5 += value[j*48+46] * src[col_idx[j]+6];
      d0 += value[j*48+7] * src[col_idx[j]+7];
      d1 += value[j*48+15] * src[col_idx[j]+7];
      d2 += value[j*48+23] * src[col_idx[j]+7];
      d3 += value[j*48+31] * src[col_idx[j]+7];
      d4 += value[j*48+39] * src[col_idx[j]+7];
      d5 += value[j*48+47] * src[col_idx[j]+7];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*54+0] * src[col_idx[j]+0];
      d1 += value[j*54+9] * src[col_idx[j]+0];
      d2 += value[j*54+18] * src[col_idx[j]+0];
      d3 += value[j*54+27] * src[col_idx[j]+0];
      d4 += value[j*54+36] * src[col_idx[j]+0];
      d5 += value[j*54+45] * src[col_idx[j]+0];
      d0 += value[j*54+1] * src[col_idx[j]+1];
      d1 += value[j*54+10] * src[col_idx[j]+1];
      d2 += value[j*54+19] * src[col_idx[j]+1];
      d3 += value[j*54+28] * src[col_idx[j]+1];
      d4 += value[j*54+37] * src[col_idx[j]+1];
      d5 += value[j*54+46] * src[col_idx[j]+1];
      d0 += value[j*54+2] * src[col_idx[j]+2];
      d1 += value[j*54+11] * src[col_idx[j]+2];
      d2 += value[j*54+20] * src[col_idx[j]+2];
      d3 += value[j*54+29] * src[col_idx[j]+2];
      d4 += value[j*54+38] * src[col_idx[j]+2];
      d5 += value[j*54+47] * src[col_idx[j]+2];
      d0 += value[j*54+3] * src[col_idx[j]+3];
      d1 += value[j*54+12] * src[col_idx[j]+3];
      d2 += value[j*54+21] * src[col_idx[j]+3];
      d3 += value[j*54+30] * src[col_idx[j]+3];
      d4 += value[j*54+39] * src[col_idx[j]+3];
      d5 += value[j*54+48] * src[col_idx[j]+3];
      d0 += value[j*54+4] * src[col_idx[j]+4];
      d1 += value[j*54+13] * src[col_idx[j]+4];
      d2 += value[j*54+22] * src[col_idx[j]+4];
      d3 += value[j*54+31] * src[col_idx[j]+4];
      d4 += value[j*54+40] * src[col_idx[j]+4];
      d5 += value[j*54+49] * src[col_idx[j]+4];
      d0 += value[j*54+5] * src[col_idx[j]+5];
      d1 += value[j*54+14] * src[col_idx[j]+5];
      d2 += value[j*54+23] * src[col_idx[j]+5];
      d3 += value[j*54+32] * src[col_idx[j]+5];
      d4 += value[j*54+41] * src[col_idx[j]+5];
      d5 += value[j*54+50] * src[col_idx[j]+5];
      d0 += value[j*54+6] * src[col_idx[j]+6];
      d1 += value[j*54+15] * src[col_idx[j]+6];
      d2 += value[j*54+24] * src[col_idx[j]+6];
      d3 += value[j*54+33] * src[col_idx[j]+6];
      d4 += value[j*54+42] * src[col_idx[j]+6];
      d5 += value[j*54+51] * src[col_idx[j]+6];
      d0 += value[j*54+7] * src[col_idx[j]+7];
      d1 += value[j*54+16] * src[col_idx[j]+7];
      d2 += value[j*54+25] * src[col_idx[j]+7];
      d3 += value[j*54+34] * src[col_idx[j]+7];
      d4 += value[j*54+43] * src[col_idx[j]+7];
      d5 += value[j*54+52] * src[col_idx[j]+7];
      d0 += value[j*54+8] * src[col_idx[j]+8];
      d1 += value[j*54+17] * src[col_idx[j]+8];
      d2 += value[j*54+26] * src[col_idx[j]+8];
      d3 += value[j*54+35] * src[col_idx[j]+8];
      d4 += value[j*54+44] * src[col_idx[j]+8];
      d5 += value[j*54+53] * src[col_idx[j]+8];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*60+0] * src[col_idx[j]+0];
      d1 += value[j*60+10] * src[col_idx[j]+0];
      d2 += value[j*60+20] * src[col_idx[j]+0];
      d3 += value[j*60+30] * src[col_idx[j]+0];
      d4 += value[j*60+40] * src[col_idx[j]+0];
      d5 += value[j*60+50] * src[col_idx[j]+0];
      d0 += value[j*60+1] * src[col_idx[j]+1];
      d1 += value[j*60+11] * src[col_idx[j]+1];
      d2 += value[j*60+21] * src[col_idx[j]+1];
      d3 += value[j*60+31] * src[col_idx[j]+1];
      d4 += value[j*60+41] * src[col_idx[j]+1];
      d5 += value[j*60+51] * src[col_idx[j]+1];
      d0 += value[j*60+2] * src[col_idx[j]+2];
      d1 += value[j*60+12] * src[col_idx[j]+2];
      d2 += value[j*60+22] * src[col_idx[j]+2];
      d3 += value[j*60+32] * src[col_idx[j]+2];
      d4 += value[j*60+42] * src[col_idx[j]+2];
      d5 += value[j*60+52] * src[col_idx[j]+2];
      d0 += value[j*60+3] * src[col_idx[j]+3];
      d1 += value[j*60+13] * src[col_idx[j]+3];
      d2 += value[j*60+23] * src[col_idx[j]+3];
      d3 += value[j*60+33] * src[col_idx[j]+3];
      d4 += value[j*60+43] * src[col_idx[j]+3];
      d5 += value[j*60+53] * src[col_idx[j]+3];
      d0 += value[j*60+4] * src[col_idx[j]+4];
      d1 += value[j*60+14] * src[col_idx[j]+4];
      d2 += value[j*60+24] * src[col_idx[j]+4];
      d3 += value[j*60+34] * src[col_idx[j]+4];
      d4 += value[j*60+44] * src[col_idx[j]+4];
      d5 += value[j*60+54] * src[col_idx[j]+4];
      d0 += value[j*60+5] * src[col_idx[j]+5];
      d1 += value[j*60+15] * src[col_idx[j]+5];
      d2 += value[j*60+25] * src[col_idx[j]+5];
      d3 += value[j*60+35] * src[col_idx[j]+5];
      d4 += value[j*60+45] * src[col_idx[j]+5];
      d5 += value[j*60+55] * src[col_idx[j]+5];
      d0 += value[j*60+6] * src[col_idx[j]+6];
      d1 += value[j*60+16] * src[col_idx[j]+6];
      d2 += value[j*60+26] * src[col_idx[j]+6];
      d3 += value[j*60+36] * src[col_idx[j]+6];
      d4 += value[j*60+46] * src[col_idx[j]+6];
      d5 += value[j*60+56] * src[col_idx[j]+6];
      d0 += value[j*60+7] * src[col_idx[j]+7];
      d1 += value[j*60+17] * src[col_idx[j]+7];
      d2 += value[j*60+27] * src[col_idx[j]+7];
      d3 += value[j*60+37] * src[col_idx[j]+7];
      d4 += value[j*60+47] * src[col_idx[j]+7];
      d5 += value[j*60+57] * src[col_idx[j]+7];
      d0 += value[j*60+8] * src[col_idx[j]+8];
      d1 += value[j*60+18] * src[col_idx[j]+8];
      d2 += value[j*60+28] * src[col_idx[j]+8];
      d3 += value[j*60+38] * src[col_idx[j]+8];
      d4 += value[j*60+48] * src[col_idx[j]+8];
      d5 += value[j*60+58] * src[col_idx[j]+8];
      d0 += value[j*60+9] * src[col_idx[j]+9];
      d1 += value[j*60+19] * src[col_idx[j]+9];
      d2 += value[j*60+29] * src[col_idx[j]+9];
      d3 += value[j*60+39] * src[col_idx[j]+9];
      d4 += value[j*60+49] * src[col_idx[j]+9];
      d5 += value[j*60+59] * src[col_idx[j]+9];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*66+0] * src[col_idx[j]+0];
      d1 += value[j*66+11] * src[col_idx[j]+0];
      d2 += value[j*66+22] * src[col_idx[j]+0];
      d3 += value[j*66+33] * src[col_idx[j]+0];
      d4 += value[j*66+44] * src[col_idx[j]+0];
      d5 += value[j*66+55] * src[col_idx[j]+0];
      d0 += value[j*66+1] * src[col_idx[j]+1];
      d1 += value[j*66+12] * src[col_idx[j]+1];
      d2 += value[j*66+23] * src[col_idx[j]+1];
      d3 += value[j*66+34] * src[col_idx[j]+1];
      d4 += value[j*66+45] * src[col_idx[j]+1];
      d5 += value[j*66+56] * src[col_idx[j]+1];
      d0 += value[j*66+2] * src[col_idx[j]+2];
      d1 += value[j*66+13] * src[col_idx[j]+2];
      d2 += value[j*66+24] * src[col_idx[j]+2];
      d3 += value[j*66+35] * src[col_idx[j]+2];
      d4 += value[j*66+46] * src[col_idx[j]+2];
      d5 += value[j*66+57] * src[col_idx[j]+2];
      d0 += value[j*66+3] * src[col_idx[j]+3];
      d1 += value[j*66+14] * src[col_idx[j]+3];
      d2 += value[j*66+25] * src[col_idx[j]+3];
      d3 += value[j*66+36] * src[col_idx[j]+3];
      d4 += value[j*66+47] * src[col_idx[j]+3];
      d5 += value[j*66+58] * src[col_idx[j]+3];
      d0 += value[j*66+4] * src[col_idx[j]+4];
      d1 += value[j*66+15] * src[col_idx[j]+4];
      d2 += value[j*66+26] * src[col_idx[j]+4];
      d3 += value[j*66+37] * src[col_idx[j]+4];
      d4 += value[j*66+48] * src[col_idx[j]+4];
      d5 += value[j*66+59] * src[col_idx[j]+4];
      d0 += value[j*66+5] * src[col_idx[j]+5];
      d1 += value[j*66+16] * src[col_idx[j]+5];
      d2 += value[j*66+27] * src[col_idx[j]+5];
      d3 += value[j*66+38] * src[col_idx[j]+5];
      d4 += value[j*66+49] * src[col_idx[j]+5];
      d5 += value[j*66+60] * src[col_idx[j]+5];
      d0 += value[j*66+6] * src[col_idx[j]+6];
      d1 += value[j*66+17] * src[col_idx[j]+6];
      d2 += value[j*66+28] * src[col_idx[j]+6];
      d3 += value[j*66+39] * src[col_idx[j]+6];
      d4 += value[j*66+50] * src[col_idx[j]+6];
      d5 += value[j*66+61] * src[col_idx[j]+6];
      d0 += value[j*66+7] * src[col_idx[j]+7];
      d1 += value[j*66+18] * src[col_idx[j]+7];
      d2 += value[j*66+29] * src[col_idx[j]+7];
      d3 += value[j*66+40] * src[col_idx[j]+7];
      d4 += value[j*66+51] * src[col_idx[j]+7];
      d5 += value[j*66+62] * src[col_idx[j]+7];
      d0 += value[j*66+8] * src[col_idx[j]+8];
      d1 += value[j*66+19] * src[col_idx[j]+8];
      d2 += value[j*66+30] * src[col_idx[j]+8];
      d3 += value[j*66+41] * src[col_idx[j]+8];
      d4 += value[j*66+52] * src[col_idx[j]+8];
      d5 += value[j*66+63] * src[col_idx[j]+8];
      d0 += value[j*66+9] * src[col_idx[j]+9];
      d1 += value[j*66+20] * src[col_idx[j]+9];
      d2 += value[j*66+31] * src[col_idx[j]+9];
      d3 += value[j*66+42] * src[col_idx[j]+9];
      d4 += value[j*66+53] * src[col_idx[j]+9];
      d5 += value[j*66+64] * src[col_idx[j]+9];
      d0 += value[j*66+10] * src[col_idx[j]+10];
      d1 += value[j*66+21] * src[col_idx[j]+10];
      d2 += value[j*66+32] * src[col_idx[j]+10];
      d3 += value[j*66+43] * src[col_idx[j]+10];
      d4 += value[j*66+54] * src[col_idx[j]+10];
      d5 += value[j*66+65] * src[col_idx[j]+10];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_6x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5;
    d0 = dest[6*i+0];
    d1 = dest[6*i+1];
    d2 = dest[6*i+2];
    d3 = dest[6*i+3];
    d4 = dest[6*i+4];
    d5 = dest[6*i+5];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*72+0] * src[col_idx[j]+0];
      d1 += value[j*72+12] * src[col_idx[j]+0];
      d2 += value[j*72+24] * src[col_idx[j]+0];
      d3 += value[j*72+36] * src[col_idx[j]+0];
      d4 += value[j*72+48] * src[col_idx[j]+0];
      d5 += value[j*72+60] * src[col_idx[j]+0];
      d0 += value[j*72+1] * src[col_idx[j]+1];
      d1 += value[j*72+13] * src[col_idx[j]+1];
      d2 += value[j*72+25] * src[col_idx[j]+1];
      d3 += value[j*72+37] * src[col_idx[j]+1];
      d4 += value[j*72+49] * src[col_idx[j]+1];
      d5 += value[j*72+61] * src[col_idx[j]+1];
      d0 += value[j*72+2] * src[col_idx[j]+2];
      d1 += value[j*72+14] * src[col_idx[j]+2];
      d2 += value[j*72+26] * src[col_idx[j]+2];
      d3 += value[j*72+38] * src[col_idx[j]+2];
      d4 += value[j*72+50] * src[col_idx[j]+2];
      d5 += value[j*72+62] * src[col_idx[j]+2];
      d0 += value[j*72+3] * src[col_idx[j]+3];
      d1 += value[j*72+15] * src[col_idx[j]+3];
      d2 += value[j*72+27] * src[col_idx[j]+3];
      d3 += value[j*72+39] * src[col_idx[j]+3];
      d4 += value[j*72+51] * src[col_idx[j]+3];
      d5 += value[j*72+63] * src[col_idx[j]+3];
      d0 += value[j*72+4] * src[col_idx[j]+4];
      d1 += value[j*72+16] * src[col_idx[j]+4];
      d2 += value[j*72+28] * src[col_idx[j]+4];
      d3 += value[j*72+40] * src[col_idx[j]+4];
      d4 += value[j*72+52] * src[col_idx[j]+4];
      d5 += value[j*72+64] * src[col_idx[j]+4];
      d0 += value[j*72+5] * src[col_idx[j]+5];
      d1 += value[j*72+17] * src[col_idx[j]+5];
      d2 += value[j*72+29] * src[col_idx[j]+5];
      d3 += value[j*72+41] * src[col_idx[j]+5];
      d4 += value[j*72+53] * src[col_idx[j]+5];
      d5 += value[j*72+65] * src[col_idx[j]+5];
      d0 += value[j*72+6] * src[col_idx[j]+6];
      d1 += value[j*72+18] * src[col_idx[j]+6];
      d2 += value[j*72+30] * src[col_idx[j]+6];
      d3 += value[j*72+42] * src[col_idx[j]+6];
      d4 += value[j*72+54] * src[col_idx[j]+6];
      d5 += value[j*72+66] * src[col_idx[j]+6];
      d0 += value[j*72+7] * src[col_idx[j]+7];
      d1 += value[j*72+19] * src[col_idx[j]+7];
      d2 += value[j*72+31] * src[col_idx[j]+7];
      d3 += value[j*72+43] * src[col_idx[j]+7];
      d4 += value[j*72+55] * src[col_idx[j]+7];
      d5 += value[j*72+67] * src[col_idx[j]+7];
      d0 += value[j*72+8] * src[col_idx[j]+8];
      d1 += value[j*72+20] * src[col_idx[j]+8];
      d2 += value[j*72+32] * src[col_idx[j]+8];
      d3 += value[j*72+44] * src[col_idx[j]+8];
      d4 += value[j*72+56] * src[col_idx[j]+8];
      d5 += value[j*72+68] * src[col_idx[j]+8];
      d0 += value[j*72+9] * src[col_idx[j]+9];
      d1 += value[j*72+21] * src[col_idx[j]+9];
      d2 += value[j*72+33] * src[col_idx[j]+9];
      d3 += value[j*72+45] * src[col_idx[j]+9];
      d4 += value[j*72+57] * src[col_idx[j]+9];
      d5 += value[j*72+69] * src[col_idx[j]+9];
      d0 += value[j*72+10] * src[col_idx[j]+10];
      d1 += value[j*72+22] * src[col_idx[j]+10];
      d2 += value[j*72+34] * src[col_idx[j]+10];
      d3 += value[j*72+46] * src[col_idx[j]+10];
      d4 += value[j*72+58] * src[col_idx[j]+10];
      d5 += value[j*72+70] * src[col_idx[j]+10];
      d0 += value[j*72+11] * src[col_idx[j]+11];
      d1 += value[j*72+23] * src[col_idx[j]+11];
      d2 += value[j*72+35] * src[col_idx[j]+11];
      d3 += value[j*72+47] * src[col_idx[j]+11];
      d4 += value[j*72+59] * src[col_idx[j]+11];
      d5 += value[j*72+71] * src[col_idx[j]+11];
    }
    dest[6*i+0] = d0;
    dest[6*i+1] = d1;
    dest[6*i+2] = d2;
    dest[6*i+3] = d3;
    dest[6*i+4] = d4;
    dest[6*i+5] = d5;
  }
}
void bsmvm_7x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*7+0] * src[col_idx[j]+0];
      d1 += value[j*7+1] * src[col_idx[j]+0];
      d2 += value[j*7+2] * src[col_idx[j]+0];
      d3 += value[j*7+3] * src[col_idx[j]+0];
      d4 += value[j*7+4] * src[col_idx[j]+0];
      d5 += value[j*7+5] * src[col_idx[j]+0];
      d6 += value[j*7+6] * src[col_idx[j]+0];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*14+0] * src[col_idx[j]+0];
      d1 += value[j*14+2] * src[col_idx[j]+0];
      d2 += value[j*14+4] * src[col_idx[j]+0];
      d3 += value[j*14+6] * src[col_idx[j]+0];
      d4 += value[j*14+8] * src[col_idx[j]+0];
      d5 += value[j*14+10] * src[col_idx[j]+0];
      d6 += value[j*14+12] * src[col_idx[j]+0];
      d0 += value[j*14+1] * src[col_idx[j]+1];
      d1 += value[j*14+3] * src[col_idx[j]+1];
      d2 += value[j*14+5] * src[col_idx[j]+1];
      d3 += value[j*14+7] * src[col_idx[j]+1];
      d4 += value[j*14+9] * src[col_idx[j]+1];
      d5 += value[j*14+11] * src[col_idx[j]+1];
      d6 += value[j*14+13] * src[col_idx[j]+1];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*21+0] * src[col_idx[j]+0];
      d1 += value[j*21+3] * src[col_idx[j]+0];
      d2 += value[j*21+6] * src[col_idx[j]+0];
      d3 += value[j*21+9] * src[col_idx[j]+0];
      d4 += value[j*21+12] * src[col_idx[j]+0];
      d5 += value[j*21+15] * src[col_idx[j]+0];
      d6 += value[j*21+18] * src[col_idx[j]+0];
      d0 += value[j*21+1] * src[col_idx[j]+1];
      d1 += value[j*21+4] * src[col_idx[j]+1];
      d2 += value[j*21+7] * src[col_idx[j]+1];
      d3 += value[j*21+10] * src[col_idx[j]+1];
      d4 += value[j*21+13] * src[col_idx[j]+1];
      d5 += value[j*21+16] * src[col_idx[j]+1];
      d6 += value[j*21+19] * src[col_idx[j]+1];
      d0 += value[j*21+2] * src[col_idx[j]+2];
      d1 += value[j*21+5] * src[col_idx[j]+2];
      d2 += value[j*21+8] * src[col_idx[j]+2];
      d3 += value[j*21+11] * src[col_idx[j]+2];
      d4 += value[j*21+14] * src[col_idx[j]+2];
      d5 += value[j*21+17] * src[col_idx[j]+2];
      d6 += value[j*21+20] * src[col_idx[j]+2];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*28+0] * src[col_idx[j]+0];
      d1 += value[j*28+4] * src[col_idx[j]+0];
      d2 += value[j*28+8] * src[col_idx[j]+0];
      d3 += value[j*28+12] * src[col_idx[j]+0];
      d4 += value[j*28+16] * src[col_idx[j]+0];
      d5 += value[j*28+20] * src[col_idx[j]+0];
      d6 += value[j*28+24] * src[col_idx[j]+0];
      d0 += value[j*28+1] * src[col_idx[j]+1];
      d1 += value[j*28+5] * src[col_idx[j]+1];
      d2 += value[j*28+9] * src[col_idx[j]+1];
      d3 += value[j*28+13] * src[col_idx[j]+1];
      d4 += value[j*28+17] * src[col_idx[j]+1];
      d5 += value[j*28+21] * src[col_idx[j]+1];
      d6 += value[j*28+25] * src[col_idx[j]+1];
      d0 += value[j*28+2] * src[col_idx[j]+2];
      d1 += value[j*28+6] * src[col_idx[j]+2];
      d2 += value[j*28+10] * src[col_idx[j]+2];
      d3 += value[j*28+14] * src[col_idx[j]+2];
      d4 += value[j*28+18] * src[col_idx[j]+2];
      d5 += value[j*28+22] * src[col_idx[j]+2];
      d6 += value[j*28+26] * src[col_idx[j]+2];
      d0 += value[j*28+3] * src[col_idx[j]+3];
      d1 += value[j*28+7] * src[col_idx[j]+3];
      d2 += value[j*28+11] * src[col_idx[j]+3];
      d3 += value[j*28+15] * src[col_idx[j]+3];
      d4 += value[j*28+19] * src[col_idx[j]+3];
      d5 += value[j*28+23] * src[col_idx[j]+3];
      d6 += value[j*28+27] * src[col_idx[j]+3];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*35+0] * src[col_idx[j]+0];
      d1 += value[j*35+5] * src[col_idx[j]+0];
      d2 += value[j*35+10] * src[col_idx[j]+0];
      d3 += value[j*35+15] * src[col_idx[j]+0];
      d4 += value[j*35+20] * src[col_idx[j]+0];
      d5 += value[j*35+25] * src[col_idx[j]+0];
      d6 += value[j*35+30] * src[col_idx[j]+0];
      d0 += value[j*35+1] * src[col_idx[j]+1];
      d1 += value[j*35+6] * src[col_idx[j]+1];
      d2 += value[j*35+11] * src[col_idx[j]+1];
      d3 += value[j*35+16] * src[col_idx[j]+1];
      d4 += value[j*35+21] * src[col_idx[j]+1];
      d5 += value[j*35+26] * src[col_idx[j]+1];
      d6 += value[j*35+31] * src[col_idx[j]+1];
      d0 += value[j*35+2] * src[col_idx[j]+2];
      d1 += value[j*35+7] * src[col_idx[j]+2];
      d2 += value[j*35+12] * src[col_idx[j]+2];
      d3 += value[j*35+17] * src[col_idx[j]+2];
      d4 += value[j*35+22] * src[col_idx[j]+2];
      d5 += value[j*35+27] * src[col_idx[j]+2];
      d6 += value[j*35+32] * src[col_idx[j]+2];
      d0 += value[j*35+3] * src[col_idx[j]+3];
      d1 += value[j*35+8] * src[col_idx[j]+3];
      d2 += value[j*35+13] * src[col_idx[j]+3];
      d3 += value[j*35+18] * src[col_idx[j]+3];
      d4 += value[j*35+23] * src[col_idx[j]+3];
      d5 += value[j*35+28] * src[col_idx[j]+3];
      d6 += value[j*35+33] * src[col_idx[j]+3];
      d0 += value[j*35+4] * src[col_idx[j]+4];
      d1 += value[j*35+9] * src[col_idx[j]+4];
      d2 += value[j*35+14] * src[col_idx[j]+4];
      d3 += value[j*35+19] * src[col_idx[j]+4];
      d4 += value[j*35+24] * src[col_idx[j]+4];
      d5 += value[j*35+29] * src[col_idx[j]+4];
      d6 += value[j*35+34] * src[col_idx[j]+4];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*42+0] * src[col_idx[j]+0];
      d1 += value[j*42+6] * src[col_idx[j]+0];
      d2 += value[j*42+12] * src[col_idx[j]+0];
      d3 += value[j*42+18] * src[col_idx[j]+0];
      d4 += value[j*42+24] * src[col_idx[j]+0];
      d5 += value[j*42+30] * src[col_idx[j]+0];
      d6 += value[j*42+36] * src[col_idx[j]+0];
      d0 += value[j*42+1] * src[col_idx[j]+1];
      d1 += value[j*42+7] * src[col_idx[j]+1];
      d2 += value[j*42+13] * src[col_idx[j]+1];
      d3 += value[j*42+19] * src[col_idx[j]+1];
      d4 += value[j*42+25] * src[col_idx[j]+1];
      d5 += value[j*42+31] * src[col_idx[j]+1];
      d6 += value[j*42+37] * src[col_idx[j]+1];
      d0 += value[j*42+2] * src[col_idx[j]+2];
      d1 += value[j*42+8] * src[col_idx[j]+2];
      d2 += value[j*42+14] * src[col_idx[j]+2];
      d3 += value[j*42+20] * src[col_idx[j]+2];
      d4 += value[j*42+26] * src[col_idx[j]+2];
      d5 += value[j*42+32] * src[col_idx[j]+2];
      d6 += value[j*42+38] * src[col_idx[j]+2];
      d0 += value[j*42+3] * src[col_idx[j]+3];
      d1 += value[j*42+9] * src[col_idx[j]+3];
      d2 += value[j*42+15] * src[col_idx[j]+3];
      d3 += value[j*42+21] * src[col_idx[j]+3];
      d4 += value[j*42+27] * src[col_idx[j]+3];
      d5 += value[j*42+33] * src[col_idx[j]+3];
      d6 += value[j*42+39] * src[col_idx[j]+3];
      d0 += value[j*42+4] * src[col_idx[j]+4];
      d1 += value[j*42+10] * src[col_idx[j]+4];
      d2 += value[j*42+16] * src[col_idx[j]+4];
      d3 += value[j*42+22] * src[col_idx[j]+4];
      d4 += value[j*42+28] * src[col_idx[j]+4];
      d5 += value[j*42+34] * src[col_idx[j]+4];
      d6 += value[j*42+40] * src[col_idx[j]+4];
      d0 += value[j*42+5] * src[col_idx[j]+5];
      d1 += value[j*42+11] * src[col_idx[j]+5];
      d2 += value[j*42+17] * src[col_idx[j]+5];
      d3 += value[j*42+23] * src[col_idx[j]+5];
      d4 += value[j*42+29] * src[col_idx[j]+5];
      d5 += value[j*42+35] * src[col_idx[j]+5];
      d6 += value[j*42+41] * src[col_idx[j]+5];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*49+0] * src[col_idx[j]+0];
      d1 += value[j*49+7] * src[col_idx[j]+0];
      d2 += value[j*49+14] * src[col_idx[j]+0];
      d3 += value[j*49+21] * src[col_idx[j]+0];
      d4 += value[j*49+28] * src[col_idx[j]+0];
      d5 += value[j*49+35] * src[col_idx[j]+0];
      d6 += value[j*49+42] * src[col_idx[j]+0];
      d0 += value[j*49+1] * src[col_idx[j]+1];
      d1 += value[j*49+8] * src[col_idx[j]+1];
      d2 += value[j*49+15] * src[col_idx[j]+1];
      d3 += value[j*49+22] * src[col_idx[j]+1];
      d4 += value[j*49+29] * src[col_idx[j]+1];
      d5 += value[j*49+36] * src[col_idx[j]+1];
      d6 += value[j*49+43] * src[col_idx[j]+1];
      d0 += value[j*49+2] * src[col_idx[j]+2];
      d1 += value[j*49+9] * src[col_idx[j]+2];
      d2 += value[j*49+16] * src[col_idx[j]+2];
      d3 += value[j*49+23] * src[col_idx[j]+2];
      d4 += value[j*49+30] * src[col_idx[j]+2];
      d5 += value[j*49+37] * src[col_idx[j]+2];
      d6 += value[j*49+44] * src[col_idx[j]+2];
      d0 += value[j*49+3] * src[col_idx[j]+3];
      d1 += value[j*49+10] * src[col_idx[j]+3];
      d2 += value[j*49+17] * src[col_idx[j]+3];
      d3 += value[j*49+24] * src[col_idx[j]+3];
      d4 += value[j*49+31] * src[col_idx[j]+3];
      d5 += value[j*49+38] * src[col_idx[j]+3];
      d6 += value[j*49+45] * src[col_idx[j]+3];
      d0 += value[j*49+4] * src[col_idx[j]+4];
      d1 += value[j*49+11] * src[col_idx[j]+4];
      d2 += value[j*49+18] * src[col_idx[j]+4];
      d3 += value[j*49+25] * src[col_idx[j]+4];
      d4 += value[j*49+32] * src[col_idx[j]+4];
      d5 += value[j*49+39] * src[col_idx[j]+4];
      d6 += value[j*49+46] * src[col_idx[j]+4];
      d0 += value[j*49+5] * src[col_idx[j]+5];
      d1 += value[j*49+12] * src[col_idx[j]+5];
      d2 += value[j*49+19] * src[col_idx[j]+5];
      d3 += value[j*49+26] * src[col_idx[j]+5];
      d4 += value[j*49+33] * src[col_idx[j]+5];
      d5 += value[j*49+40] * src[col_idx[j]+5];
      d6 += value[j*49+47] * src[col_idx[j]+5];
      d0 += value[j*49+6] * src[col_idx[j]+6];
      d1 += value[j*49+13] * src[col_idx[j]+6];
      d2 += value[j*49+20] * src[col_idx[j]+6];
      d3 += value[j*49+27] * src[col_idx[j]+6];
      d4 += value[j*49+34] * src[col_idx[j]+6];
      d5 += value[j*49+41] * src[col_idx[j]+6];
      d6 += value[j*49+48] * src[col_idx[j]+6];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*56+0] * src[col_idx[j]+0];
      d1 += value[j*56+8] * src[col_idx[j]+0];
      d2 += value[j*56+16] * src[col_idx[j]+0];
      d3 += value[j*56+24] * src[col_idx[j]+0];
      d4 += value[j*56+32] * src[col_idx[j]+0];
      d5 += value[j*56+40] * src[col_idx[j]+0];
      d6 += value[j*56+48] * src[col_idx[j]+0];
      d0 += value[j*56+1] * src[col_idx[j]+1];
      d1 += value[j*56+9] * src[col_idx[j]+1];
      d2 += value[j*56+17] * src[col_idx[j]+1];
      d3 += value[j*56+25] * src[col_idx[j]+1];
      d4 += value[j*56+33] * src[col_idx[j]+1];
      d5 += value[j*56+41] * src[col_idx[j]+1];
      d6 += value[j*56+49] * src[col_idx[j]+1];
      d0 += value[j*56+2] * src[col_idx[j]+2];
      d1 += value[j*56+10] * src[col_idx[j]+2];
      d2 += value[j*56+18] * src[col_idx[j]+2];
      d3 += value[j*56+26] * src[col_idx[j]+2];
      d4 += value[j*56+34] * src[col_idx[j]+2];
      d5 += value[j*56+42] * src[col_idx[j]+2];
      d6 += value[j*56+50] * src[col_idx[j]+2];
      d0 += value[j*56+3] * src[col_idx[j]+3];
      d1 += value[j*56+11] * src[col_idx[j]+3];
      d2 += value[j*56+19] * src[col_idx[j]+3];
      d3 += value[j*56+27] * src[col_idx[j]+3];
      d4 += value[j*56+35] * src[col_idx[j]+3];
      d5 += value[j*56+43] * src[col_idx[j]+3];
      d6 += value[j*56+51] * src[col_idx[j]+3];
      d0 += value[j*56+4] * src[col_idx[j]+4];
      d1 += value[j*56+12] * src[col_idx[j]+4];
      d2 += value[j*56+20] * src[col_idx[j]+4];
      d3 += value[j*56+28] * src[col_idx[j]+4];
      d4 += value[j*56+36] * src[col_idx[j]+4];
      d5 += value[j*56+44] * src[col_idx[j]+4];
      d6 += value[j*56+52] * src[col_idx[j]+4];
      d0 += value[j*56+5] * src[col_idx[j]+5];
      d1 += value[j*56+13] * src[col_idx[j]+5];
      d2 += value[j*56+21] * src[col_idx[j]+5];
      d3 += value[j*56+29] * src[col_idx[j]+5];
      d4 += value[j*56+37] * src[col_idx[j]+5];
      d5 += value[j*56+45] * src[col_idx[j]+5];
      d6 += value[j*56+53] * src[col_idx[j]+5];
      d0 += value[j*56+6] * src[col_idx[j]+6];
      d1 += value[j*56+14] * src[col_idx[j]+6];
      d2 += value[j*56+22] * src[col_idx[j]+6];
      d3 += value[j*56+30] * src[col_idx[j]+6];
      d4 += value[j*56+38] * src[col_idx[j]+6];
      d5 += value[j*56+46] * src[col_idx[j]+6];
      d6 += value[j*56+54] * src[col_idx[j]+6];
      d0 += value[j*56+7] * src[col_idx[j]+7];
      d1 += value[j*56+15] * src[col_idx[j]+7];
      d2 += value[j*56+23] * src[col_idx[j]+7];
      d3 += value[j*56+31] * src[col_idx[j]+7];
      d4 += value[j*56+39] * src[col_idx[j]+7];
      d5 += value[j*56+47] * src[col_idx[j]+7];
      d6 += value[j*56+55] * src[col_idx[j]+7];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*63+0] * src[col_idx[j]+0];
      d1 += value[j*63+9] * src[col_idx[j]+0];
      d2 += value[j*63+18] * src[col_idx[j]+0];
      d3 += value[j*63+27] * src[col_idx[j]+0];
      d4 += value[j*63+36] * src[col_idx[j]+0];
      d5 += value[j*63+45] * src[col_idx[j]+0];
      d6 += value[j*63+54] * src[col_idx[j]+0];
      d0 += value[j*63+1] * src[col_idx[j]+1];
      d1 += value[j*63+10] * src[col_idx[j]+1];
      d2 += value[j*63+19] * src[col_idx[j]+1];
      d3 += value[j*63+28] * src[col_idx[j]+1];
      d4 += value[j*63+37] * src[col_idx[j]+1];
      d5 += value[j*63+46] * src[col_idx[j]+1];
      d6 += value[j*63+55] * src[col_idx[j]+1];
      d0 += value[j*63+2] * src[col_idx[j]+2];
      d1 += value[j*63+11] * src[col_idx[j]+2];
      d2 += value[j*63+20] * src[col_idx[j]+2];
      d3 += value[j*63+29] * src[col_idx[j]+2];
      d4 += value[j*63+38] * src[col_idx[j]+2];
      d5 += value[j*63+47] * src[col_idx[j]+2];
      d6 += value[j*63+56] * src[col_idx[j]+2];
      d0 += value[j*63+3] * src[col_idx[j]+3];
      d1 += value[j*63+12] * src[col_idx[j]+3];
      d2 += value[j*63+21] * src[col_idx[j]+3];
      d3 += value[j*63+30] * src[col_idx[j]+3];
      d4 += value[j*63+39] * src[col_idx[j]+3];
      d5 += value[j*63+48] * src[col_idx[j]+3];
      d6 += value[j*63+57] * src[col_idx[j]+3];
      d0 += value[j*63+4] * src[col_idx[j]+4];
      d1 += value[j*63+13] * src[col_idx[j]+4];
      d2 += value[j*63+22] * src[col_idx[j]+4];
      d3 += value[j*63+31] * src[col_idx[j]+4];
      d4 += value[j*63+40] * src[col_idx[j]+4];
      d5 += value[j*63+49] * src[col_idx[j]+4];
      d6 += value[j*63+58] * src[col_idx[j]+4];
      d0 += value[j*63+5] * src[col_idx[j]+5];
      d1 += value[j*63+14] * src[col_idx[j]+5];
      d2 += value[j*63+23] * src[col_idx[j]+5];
      d3 += value[j*63+32] * src[col_idx[j]+5];
      d4 += value[j*63+41] * src[col_idx[j]+5];
      d5 += value[j*63+50] * src[col_idx[j]+5];
      d6 += value[j*63+59] * src[col_idx[j]+5];
      d0 += value[j*63+6] * src[col_idx[j]+6];
      d1 += value[j*63+15] * src[col_idx[j]+6];
      d2 += value[j*63+24] * src[col_idx[j]+6];
      d3 += value[j*63+33] * src[col_idx[j]+6];
      d4 += value[j*63+42] * src[col_idx[j]+6];
      d5 += value[j*63+51] * src[col_idx[j]+6];
      d6 += value[j*63+60] * src[col_idx[j]+6];
      d0 += value[j*63+7] * src[col_idx[j]+7];
      d1 += value[j*63+16] * src[col_idx[j]+7];
      d2 += value[j*63+25] * src[col_idx[j]+7];
      d3 += value[j*63+34] * src[col_idx[j]+7];
      d4 += value[j*63+43] * src[col_idx[j]+7];
      d5 += value[j*63+52] * src[col_idx[j]+7];
      d6 += value[j*63+61] * src[col_idx[j]+7];
      d0 += value[j*63+8] * src[col_idx[j]+8];
      d1 += value[j*63+17] * src[col_idx[j]+8];
      d2 += value[j*63+26] * src[col_idx[j]+8];
      d3 += value[j*63+35] * src[col_idx[j]+8];
      d4 += value[j*63+44] * src[col_idx[j]+8];
      d5 += value[j*63+53] * src[col_idx[j]+8];
      d6 += value[j*63+62] * src[col_idx[j]+8];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*70+0] * src[col_idx[j]+0];
      d1 += value[j*70+10] * src[col_idx[j]+0];
      d2 += value[j*70+20] * src[col_idx[j]+0];
      d3 += value[j*70+30] * src[col_idx[j]+0];
      d4 += value[j*70+40] * src[col_idx[j]+0];
      d5 += value[j*70+50] * src[col_idx[j]+0];
      d6 += value[j*70+60] * src[col_idx[j]+0];
      d0 += value[j*70+1] * src[col_idx[j]+1];
      d1 += value[j*70+11] * src[col_idx[j]+1];
      d2 += value[j*70+21] * src[col_idx[j]+1];
      d3 += value[j*70+31] * src[col_idx[j]+1];
      d4 += value[j*70+41] * src[col_idx[j]+1];
      d5 += value[j*70+51] * src[col_idx[j]+1];
      d6 += value[j*70+61] * src[col_idx[j]+1];
      d0 += value[j*70+2] * src[col_idx[j]+2];
      d1 += value[j*70+12] * src[col_idx[j]+2];
      d2 += value[j*70+22] * src[col_idx[j]+2];
      d3 += value[j*70+32] * src[col_idx[j]+2];
      d4 += value[j*70+42] * src[col_idx[j]+2];
      d5 += value[j*70+52] * src[col_idx[j]+2];
      d6 += value[j*70+62] * src[col_idx[j]+2];
      d0 += value[j*70+3] * src[col_idx[j]+3];
      d1 += value[j*70+13] * src[col_idx[j]+3];
      d2 += value[j*70+23] * src[col_idx[j]+3];
      d3 += value[j*70+33] * src[col_idx[j]+3];
      d4 += value[j*70+43] * src[col_idx[j]+3];
      d5 += value[j*70+53] * src[col_idx[j]+3];
      d6 += value[j*70+63] * src[col_idx[j]+3];
      d0 += value[j*70+4] * src[col_idx[j]+4];
      d1 += value[j*70+14] * src[col_idx[j]+4];
      d2 += value[j*70+24] * src[col_idx[j]+4];
      d3 += value[j*70+34] * src[col_idx[j]+4];
      d4 += value[j*70+44] * src[col_idx[j]+4];
      d5 += value[j*70+54] * src[col_idx[j]+4];
      d6 += value[j*70+64] * src[col_idx[j]+4];
      d0 += value[j*70+5] * src[col_idx[j]+5];
      d1 += value[j*70+15] * src[col_idx[j]+5];
      d2 += value[j*70+25] * src[col_idx[j]+5];
      d3 += value[j*70+35] * src[col_idx[j]+5];
      d4 += value[j*70+45] * src[col_idx[j]+5];
      d5 += value[j*70+55] * src[col_idx[j]+5];
      d6 += value[j*70+65] * src[col_idx[j]+5];
      d0 += value[j*70+6] * src[col_idx[j]+6];
      d1 += value[j*70+16] * src[col_idx[j]+6];
      d2 += value[j*70+26] * src[col_idx[j]+6];
      d3 += value[j*70+36] * src[col_idx[j]+6];
      d4 += value[j*70+46] * src[col_idx[j]+6];
      d5 += value[j*70+56] * src[col_idx[j]+6];
      d6 += value[j*70+66] * src[col_idx[j]+6];
      d0 += value[j*70+7] * src[col_idx[j]+7];
      d1 += value[j*70+17] * src[col_idx[j]+7];
      d2 += value[j*70+27] * src[col_idx[j]+7];
      d3 += value[j*70+37] * src[col_idx[j]+7];
      d4 += value[j*70+47] * src[col_idx[j]+7];
      d5 += value[j*70+57] * src[col_idx[j]+7];
      d6 += value[j*70+67] * src[col_idx[j]+7];
      d0 += value[j*70+8] * src[col_idx[j]+8];
      d1 += value[j*70+18] * src[col_idx[j]+8];
      d2 += value[j*70+28] * src[col_idx[j]+8];
      d3 += value[j*70+38] * src[col_idx[j]+8];
      d4 += value[j*70+48] * src[col_idx[j]+8];
      d5 += value[j*70+58] * src[col_idx[j]+8];
      d6 += value[j*70+68] * src[col_idx[j]+8];
      d0 += value[j*70+9] * src[col_idx[j]+9];
      d1 += value[j*70+19] * src[col_idx[j]+9];
      d2 += value[j*70+29] * src[col_idx[j]+9];
      d3 += value[j*70+39] * src[col_idx[j]+9];
      d4 += value[j*70+49] * src[col_idx[j]+9];
      d5 += value[j*70+59] * src[col_idx[j]+9];
      d6 += value[j*70+69] * src[col_idx[j]+9];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*77+0] * src[col_idx[j]+0];
      d1 += value[j*77+11] * src[col_idx[j]+0];
      d2 += value[j*77+22] * src[col_idx[j]+0];
      d3 += value[j*77+33] * src[col_idx[j]+0];
      d4 += value[j*77+44] * src[col_idx[j]+0];
      d5 += value[j*77+55] * src[col_idx[j]+0];
      d6 += value[j*77+66] * src[col_idx[j]+0];
      d0 += value[j*77+1] * src[col_idx[j]+1];
      d1 += value[j*77+12] * src[col_idx[j]+1];
      d2 += value[j*77+23] * src[col_idx[j]+1];
      d3 += value[j*77+34] * src[col_idx[j]+1];
      d4 += value[j*77+45] * src[col_idx[j]+1];
      d5 += value[j*77+56] * src[col_idx[j]+1];
      d6 += value[j*77+67] * src[col_idx[j]+1];
      d0 += value[j*77+2] * src[col_idx[j]+2];
      d1 += value[j*77+13] * src[col_idx[j]+2];
      d2 += value[j*77+24] * src[col_idx[j]+2];
      d3 += value[j*77+35] * src[col_idx[j]+2];
      d4 += value[j*77+46] * src[col_idx[j]+2];
      d5 += value[j*77+57] * src[col_idx[j]+2];
      d6 += value[j*77+68] * src[col_idx[j]+2];
      d0 += value[j*77+3] * src[col_idx[j]+3];
      d1 += value[j*77+14] * src[col_idx[j]+3];
      d2 += value[j*77+25] * src[col_idx[j]+3];
      d3 += value[j*77+36] * src[col_idx[j]+3];
      d4 += value[j*77+47] * src[col_idx[j]+3];
      d5 += value[j*77+58] * src[col_idx[j]+3];
      d6 += value[j*77+69] * src[col_idx[j]+3];
      d0 += value[j*77+4] * src[col_idx[j]+4];
      d1 += value[j*77+15] * src[col_idx[j]+4];
      d2 += value[j*77+26] * src[col_idx[j]+4];
      d3 += value[j*77+37] * src[col_idx[j]+4];
      d4 += value[j*77+48] * src[col_idx[j]+4];
      d5 += value[j*77+59] * src[col_idx[j]+4];
      d6 += value[j*77+70] * src[col_idx[j]+4];
      d0 += value[j*77+5] * src[col_idx[j]+5];
      d1 += value[j*77+16] * src[col_idx[j]+5];
      d2 += value[j*77+27] * src[col_idx[j]+5];
      d3 += value[j*77+38] * src[col_idx[j]+5];
      d4 += value[j*77+49] * src[col_idx[j]+5];
      d5 += value[j*77+60] * src[col_idx[j]+5];
      d6 += value[j*77+71] * src[col_idx[j]+5];
      d0 += value[j*77+6] * src[col_idx[j]+6];
      d1 += value[j*77+17] * src[col_idx[j]+6];
      d2 += value[j*77+28] * src[col_idx[j]+6];
      d3 += value[j*77+39] * src[col_idx[j]+6];
      d4 += value[j*77+50] * src[col_idx[j]+6];
      d5 += value[j*77+61] * src[col_idx[j]+6];
      d6 += value[j*77+72] * src[col_idx[j]+6];
      d0 += value[j*77+7] * src[col_idx[j]+7];
      d1 += value[j*77+18] * src[col_idx[j]+7];
      d2 += value[j*77+29] * src[col_idx[j]+7];
      d3 += value[j*77+40] * src[col_idx[j]+7];
      d4 += value[j*77+51] * src[col_idx[j]+7];
      d5 += value[j*77+62] * src[col_idx[j]+7];
      d6 += value[j*77+73] * src[col_idx[j]+7];
      d0 += value[j*77+8] * src[col_idx[j]+8];
      d1 += value[j*77+19] * src[col_idx[j]+8];
      d2 += value[j*77+30] * src[col_idx[j]+8];
      d3 += value[j*77+41] * src[col_idx[j]+8];
      d4 += value[j*77+52] * src[col_idx[j]+8];
      d5 += value[j*77+63] * src[col_idx[j]+8];
      d6 += value[j*77+74] * src[col_idx[j]+8];
      d0 += value[j*77+9] * src[col_idx[j]+9];
      d1 += value[j*77+20] * src[col_idx[j]+9];
      d2 += value[j*77+31] * src[col_idx[j]+9];
      d3 += value[j*77+42] * src[col_idx[j]+9];
      d4 += value[j*77+53] * src[col_idx[j]+9];
      d5 += value[j*77+64] * src[col_idx[j]+9];
      d6 += value[j*77+75] * src[col_idx[j]+9];
      d0 += value[j*77+10] * src[col_idx[j]+10];
      d1 += value[j*77+21] * src[col_idx[j]+10];
      d2 += value[j*77+32] * src[col_idx[j]+10];
      d3 += value[j*77+43] * src[col_idx[j]+10];
      d4 += value[j*77+54] * src[col_idx[j]+10];
      d5 += value[j*77+65] * src[col_idx[j]+10];
      d6 += value[j*77+76] * src[col_idx[j]+10];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_7x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6;
    d0 = dest[7*i+0];
    d1 = dest[7*i+1];
    d2 = dest[7*i+2];
    d3 = dest[7*i+3];
    d4 = dest[7*i+4];
    d5 = dest[7*i+5];
    d6 = dest[7*i+6];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*84+0] * src[col_idx[j]+0];
      d1 += value[j*84+12] * src[col_idx[j]+0];
      d2 += value[j*84+24] * src[col_idx[j]+0];
      d3 += value[j*84+36] * src[col_idx[j]+0];
      d4 += value[j*84+48] * src[col_idx[j]+0];
      d5 += value[j*84+60] * src[col_idx[j]+0];
      d6 += value[j*84+72] * src[col_idx[j]+0];
      d0 += value[j*84+1] * src[col_idx[j]+1];
      d1 += value[j*84+13] * src[col_idx[j]+1];
      d2 += value[j*84+25] * src[col_idx[j]+1];
      d3 += value[j*84+37] * src[col_idx[j]+1];
      d4 += value[j*84+49] * src[col_idx[j]+1];
      d5 += value[j*84+61] * src[col_idx[j]+1];
      d6 += value[j*84+73] * src[col_idx[j]+1];
      d0 += value[j*84+2] * src[col_idx[j]+2];
      d1 += value[j*84+14] * src[col_idx[j]+2];
      d2 += value[j*84+26] * src[col_idx[j]+2];
      d3 += value[j*84+38] * src[col_idx[j]+2];
      d4 += value[j*84+50] * src[col_idx[j]+2];
      d5 += value[j*84+62] * src[col_idx[j]+2];
      d6 += value[j*84+74] * src[col_idx[j]+2];
      d0 += value[j*84+3] * src[col_idx[j]+3];
      d1 += value[j*84+15] * src[col_idx[j]+3];
      d2 += value[j*84+27] * src[col_idx[j]+3];
      d3 += value[j*84+39] * src[col_idx[j]+3];
      d4 += value[j*84+51] * src[col_idx[j]+3];
      d5 += value[j*84+63] * src[col_idx[j]+3];
      d6 += value[j*84+75] * src[col_idx[j]+3];
      d0 += value[j*84+4] * src[col_idx[j]+4];
      d1 += value[j*84+16] * src[col_idx[j]+4];
      d2 += value[j*84+28] * src[col_idx[j]+4];
      d3 += value[j*84+40] * src[col_idx[j]+4];
      d4 += value[j*84+52] * src[col_idx[j]+4];
      d5 += value[j*84+64] * src[col_idx[j]+4];
      d6 += value[j*84+76] * src[col_idx[j]+4];
      d0 += value[j*84+5] * src[col_idx[j]+5];
      d1 += value[j*84+17] * src[col_idx[j]+5];
      d2 += value[j*84+29] * src[col_idx[j]+5];
      d3 += value[j*84+41] * src[col_idx[j]+5];
      d4 += value[j*84+53] * src[col_idx[j]+5];
      d5 += value[j*84+65] * src[col_idx[j]+5];
      d6 += value[j*84+77] * src[col_idx[j]+5];
      d0 += value[j*84+6] * src[col_idx[j]+6];
      d1 += value[j*84+18] * src[col_idx[j]+6];
      d2 += value[j*84+30] * src[col_idx[j]+6];
      d3 += value[j*84+42] * src[col_idx[j]+6];
      d4 += value[j*84+54] * src[col_idx[j]+6];
      d5 += value[j*84+66] * src[col_idx[j]+6];
      d6 += value[j*84+78] * src[col_idx[j]+6];
      d0 += value[j*84+7] * src[col_idx[j]+7];
      d1 += value[j*84+19] * src[col_idx[j]+7];
      d2 += value[j*84+31] * src[col_idx[j]+7];
      d3 += value[j*84+43] * src[col_idx[j]+7];
      d4 += value[j*84+55] * src[col_idx[j]+7];
      d5 += value[j*84+67] * src[col_idx[j]+7];
      d6 += value[j*84+79] * src[col_idx[j]+7];
      d0 += value[j*84+8] * src[col_idx[j]+8];
      d1 += value[j*84+20] * src[col_idx[j]+8];
      d2 += value[j*84+32] * src[col_idx[j]+8];
      d3 += value[j*84+44] * src[col_idx[j]+8];
      d4 += value[j*84+56] * src[col_idx[j]+8];
      d5 += value[j*84+68] * src[col_idx[j]+8];
      d6 += value[j*84+80] * src[col_idx[j]+8];
      d0 += value[j*84+9] * src[col_idx[j]+9];
      d1 += value[j*84+21] * src[col_idx[j]+9];
      d2 += value[j*84+33] * src[col_idx[j]+9];
      d3 += value[j*84+45] * src[col_idx[j]+9];
      d4 += value[j*84+57] * src[col_idx[j]+9];
      d5 += value[j*84+69] * src[col_idx[j]+9];
      d6 += value[j*84+81] * src[col_idx[j]+9];
      d0 += value[j*84+10] * src[col_idx[j]+10];
      d1 += value[j*84+22] * src[col_idx[j]+10];
      d2 += value[j*84+34] * src[col_idx[j]+10];
      d3 += value[j*84+46] * src[col_idx[j]+10];
      d4 += value[j*84+58] * src[col_idx[j]+10];
      d5 += value[j*84+70] * src[col_idx[j]+10];
      d6 += value[j*84+82] * src[col_idx[j]+10];
      d0 += value[j*84+11] * src[col_idx[j]+11];
      d1 += value[j*84+23] * src[col_idx[j]+11];
      d2 += value[j*84+35] * src[col_idx[j]+11];
      d3 += value[j*84+47] * src[col_idx[j]+11];
      d4 += value[j*84+59] * src[col_idx[j]+11];
      d5 += value[j*84+71] * src[col_idx[j]+11];
      d6 += value[j*84+83] * src[col_idx[j]+11];
    }
    dest[7*i+0] = d0;
    dest[7*i+1] = d1;
    dest[7*i+2] = d2;
    dest[7*i+3] = d3;
    dest[7*i+4] = d4;
    dest[7*i+5] = d5;
    dest[7*i+6] = d6;
  }
}
void bsmvm_8x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*8+0] * src[col_idx[j]+0];
      d1 += value[j*8+1] * src[col_idx[j]+0];
      d2 += value[j*8+2] * src[col_idx[j]+0];
      d3 += value[j*8+3] * src[col_idx[j]+0];
      d4 += value[j*8+4] * src[col_idx[j]+0];
      d5 += value[j*8+5] * src[col_idx[j]+0];
      d6 += value[j*8+6] * src[col_idx[j]+0];
      d7 += value[j*8+7] * src[col_idx[j]+0];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*16+0] * src[col_idx[j]+0];
      d1 += value[j*16+2] * src[col_idx[j]+0];
      d2 += value[j*16+4] * src[col_idx[j]+0];
      d3 += value[j*16+6] * src[col_idx[j]+0];
      d4 += value[j*16+8] * src[col_idx[j]+0];
      d5 += value[j*16+10] * src[col_idx[j]+0];
      d6 += value[j*16+12] * src[col_idx[j]+0];
      d7 += value[j*16+14] * src[col_idx[j]+0];
      d0 += value[j*16+1] * src[col_idx[j]+1];
      d1 += value[j*16+3] * src[col_idx[j]+1];
      d2 += value[j*16+5] * src[col_idx[j]+1];
      d3 += value[j*16+7] * src[col_idx[j]+1];
      d4 += value[j*16+9] * src[col_idx[j]+1];
      d5 += value[j*16+11] * src[col_idx[j]+1];
      d6 += value[j*16+13] * src[col_idx[j]+1];
      d7 += value[j*16+15] * src[col_idx[j]+1];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+3] * src[col_idx[j]+0];
      d2 += value[j*24+6] * src[col_idx[j]+0];
      d3 += value[j*24+9] * src[col_idx[j]+0];
      d4 += value[j*24+12] * src[col_idx[j]+0];
      d5 += value[j*24+15] * src[col_idx[j]+0];
      d6 += value[j*24+18] * src[col_idx[j]+0];
      d7 += value[j*24+21] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+4] * src[col_idx[j]+1];
      d2 += value[j*24+7] * src[col_idx[j]+1];
      d3 += value[j*24+10] * src[col_idx[j]+1];
      d4 += value[j*24+13] * src[col_idx[j]+1];
      d5 += value[j*24+16] * src[col_idx[j]+1];
      d6 += value[j*24+19] * src[col_idx[j]+1];
      d7 += value[j*24+22] * src[col_idx[j]+1];
      d0 += value[j*24+2] * src[col_idx[j]+2];
      d1 += value[j*24+5] * src[col_idx[j]+2];
      d2 += value[j*24+8] * src[col_idx[j]+2];
      d3 += value[j*24+11] * src[col_idx[j]+2];
      d4 += value[j*24+14] * src[col_idx[j]+2];
      d5 += value[j*24+17] * src[col_idx[j]+2];
      d6 += value[j*24+20] * src[col_idx[j]+2];
      d7 += value[j*24+23] * src[col_idx[j]+2];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*32+0] * src[col_idx[j]+0];
      d1 += value[j*32+4] * src[col_idx[j]+0];
      d2 += value[j*32+8] * src[col_idx[j]+0];
      d3 += value[j*32+12] * src[col_idx[j]+0];
      d4 += value[j*32+16] * src[col_idx[j]+0];
      d5 += value[j*32+20] * src[col_idx[j]+0];
      d6 += value[j*32+24] * src[col_idx[j]+0];
      d7 += value[j*32+28] * src[col_idx[j]+0];
      d0 += value[j*32+1] * src[col_idx[j]+1];
      d1 += value[j*32+5] * src[col_idx[j]+1];
      d2 += value[j*32+9] * src[col_idx[j]+1];
      d3 += value[j*32+13] * src[col_idx[j]+1];
      d4 += value[j*32+17] * src[col_idx[j]+1];
      d5 += value[j*32+21] * src[col_idx[j]+1];
      d6 += value[j*32+25] * src[col_idx[j]+1];
      d7 += value[j*32+29] * src[col_idx[j]+1];
      d0 += value[j*32+2] * src[col_idx[j]+2];
      d1 += value[j*32+6] * src[col_idx[j]+2];
      d2 += value[j*32+10] * src[col_idx[j]+2];
      d3 += value[j*32+14] * src[col_idx[j]+2];
      d4 += value[j*32+18] * src[col_idx[j]+2];
      d5 += value[j*32+22] * src[col_idx[j]+2];
      d6 += value[j*32+26] * src[col_idx[j]+2];
      d7 += value[j*32+30] * src[col_idx[j]+2];
      d0 += value[j*32+3] * src[col_idx[j]+3];
      d1 += value[j*32+7] * src[col_idx[j]+3];
      d2 += value[j*32+11] * src[col_idx[j]+3];
      d3 += value[j*32+15] * src[col_idx[j]+3];
      d4 += value[j*32+19] * src[col_idx[j]+3];
      d5 += value[j*32+23] * src[col_idx[j]+3];
      d6 += value[j*32+27] * src[col_idx[j]+3];
      d7 += value[j*32+31] * src[col_idx[j]+3];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*40+0] * src[col_idx[j]+0];
      d1 += value[j*40+5] * src[col_idx[j]+0];
      d2 += value[j*40+10] * src[col_idx[j]+0];
      d3 += value[j*40+15] * src[col_idx[j]+0];
      d4 += value[j*40+20] * src[col_idx[j]+0];
      d5 += value[j*40+25] * src[col_idx[j]+0];
      d6 += value[j*40+30] * src[col_idx[j]+0];
      d7 += value[j*40+35] * src[col_idx[j]+0];
      d0 += value[j*40+1] * src[col_idx[j]+1];
      d1 += value[j*40+6] * src[col_idx[j]+1];
      d2 += value[j*40+11] * src[col_idx[j]+1];
      d3 += value[j*40+16] * src[col_idx[j]+1];
      d4 += value[j*40+21] * src[col_idx[j]+1];
      d5 += value[j*40+26] * src[col_idx[j]+1];
      d6 += value[j*40+31] * src[col_idx[j]+1];
      d7 += value[j*40+36] * src[col_idx[j]+1];
      d0 += value[j*40+2] * src[col_idx[j]+2];
      d1 += value[j*40+7] * src[col_idx[j]+2];
      d2 += value[j*40+12] * src[col_idx[j]+2];
      d3 += value[j*40+17] * src[col_idx[j]+2];
      d4 += value[j*40+22] * src[col_idx[j]+2];
      d5 += value[j*40+27] * src[col_idx[j]+2];
      d6 += value[j*40+32] * src[col_idx[j]+2];
      d7 += value[j*40+37] * src[col_idx[j]+2];
      d0 += value[j*40+3] * src[col_idx[j]+3];
      d1 += value[j*40+8] * src[col_idx[j]+3];
      d2 += value[j*40+13] * src[col_idx[j]+3];
      d3 += value[j*40+18] * src[col_idx[j]+3];
      d4 += value[j*40+23] * src[col_idx[j]+3];
      d5 += value[j*40+28] * src[col_idx[j]+3];
      d6 += value[j*40+33] * src[col_idx[j]+3];
      d7 += value[j*40+38] * src[col_idx[j]+3];
      d0 += value[j*40+4] * src[col_idx[j]+4];
      d1 += value[j*40+9] * src[col_idx[j]+4];
      d2 += value[j*40+14] * src[col_idx[j]+4];
      d3 += value[j*40+19] * src[col_idx[j]+4];
      d4 += value[j*40+24] * src[col_idx[j]+4];
      d5 += value[j*40+29] * src[col_idx[j]+4];
      d6 += value[j*40+34] * src[col_idx[j]+4];
      d7 += value[j*40+39] * src[col_idx[j]+4];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*48+0] * src[col_idx[j]+0];
      d1 += value[j*48+6] * src[col_idx[j]+0];
      d2 += value[j*48+12] * src[col_idx[j]+0];
      d3 += value[j*48+18] * src[col_idx[j]+0];
      d4 += value[j*48+24] * src[col_idx[j]+0];
      d5 += value[j*48+30] * src[col_idx[j]+0];
      d6 += value[j*48+36] * src[col_idx[j]+0];
      d7 += value[j*48+42] * src[col_idx[j]+0];
      d0 += value[j*48+1] * src[col_idx[j]+1];
      d1 += value[j*48+7] * src[col_idx[j]+1];
      d2 += value[j*48+13] * src[col_idx[j]+1];
      d3 += value[j*48+19] * src[col_idx[j]+1];
      d4 += value[j*48+25] * src[col_idx[j]+1];
      d5 += value[j*48+31] * src[col_idx[j]+1];
      d6 += value[j*48+37] * src[col_idx[j]+1];
      d7 += value[j*48+43] * src[col_idx[j]+1];
      d0 += value[j*48+2] * src[col_idx[j]+2];
      d1 += value[j*48+8] * src[col_idx[j]+2];
      d2 += value[j*48+14] * src[col_idx[j]+2];
      d3 += value[j*48+20] * src[col_idx[j]+2];
      d4 += value[j*48+26] * src[col_idx[j]+2];
      d5 += value[j*48+32] * src[col_idx[j]+2];
      d6 += value[j*48+38] * src[col_idx[j]+2];
      d7 += value[j*48+44] * src[col_idx[j]+2];
      d0 += value[j*48+3] * src[col_idx[j]+3];
      d1 += value[j*48+9] * src[col_idx[j]+3];
      d2 += value[j*48+15] * src[col_idx[j]+3];
      d3 += value[j*48+21] * src[col_idx[j]+3];
      d4 += value[j*48+27] * src[col_idx[j]+3];
      d5 += value[j*48+33] * src[col_idx[j]+3];
      d6 += value[j*48+39] * src[col_idx[j]+3];
      d7 += value[j*48+45] * src[col_idx[j]+3];
      d0 += value[j*48+4] * src[col_idx[j]+4];
      d1 += value[j*48+10] * src[col_idx[j]+4];
      d2 += value[j*48+16] * src[col_idx[j]+4];
      d3 += value[j*48+22] * src[col_idx[j]+4];
      d4 += value[j*48+28] * src[col_idx[j]+4];
      d5 += value[j*48+34] * src[col_idx[j]+4];
      d6 += value[j*48+40] * src[col_idx[j]+4];
      d7 += value[j*48+46] * src[col_idx[j]+4];
      d0 += value[j*48+5] * src[col_idx[j]+5];
      d1 += value[j*48+11] * src[col_idx[j]+5];
      d2 += value[j*48+17] * src[col_idx[j]+5];
      d3 += value[j*48+23] * src[col_idx[j]+5];
      d4 += value[j*48+29] * src[col_idx[j]+5];
      d5 += value[j*48+35] * src[col_idx[j]+5];
      d6 += value[j*48+41] * src[col_idx[j]+5];
      d7 += value[j*48+47] * src[col_idx[j]+5];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*56+0] * src[col_idx[j]+0];
      d1 += value[j*56+7] * src[col_idx[j]+0];
      d2 += value[j*56+14] * src[col_idx[j]+0];
      d3 += value[j*56+21] * src[col_idx[j]+0];
      d4 += value[j*56+28] * src[col_idx[j]+0];
      d5 += value[j*56+35] * src[col_idx[j]+0];
      d6 += value[j*56+42] * src[col_idx[j]+0];
      d7 += value[j*56+49] * src[col_idx[j]+0];
      d0 += value[j*56+1] * src[col_idx[j]+1];
      d1 += value[j*56+8] * src[col_idx[j]+1];
      d2 += value[j*56+15] * src[col_idx[j]+1];
      d3 += value[j*56+22] * src[col_idx[j]+1];
      d4 += value[j*56+29] * src[col_idx[j]+1];
      d5 += value[j*56+36] * src[col_idx[j]+1];
      d6 += value[j*56+43] * src[col_idx[j]+1];
      d7 += value[j*56+50] * src[col_idx[j]+1];
      d0 += value[j*56+2] * src[col_idx[j]+2];
      d1 += value[j*56+9] * src[col_idx[j]+2];
      d2 += value[j*56+16] * src[col_idx[j]+2];
      d3 += value[j*56+23] * src[col_idx[j]+2];
      d4 += value[j*56+30] * src[col_idx[j]+2];
      d5 += value[j*56+37] * src[col_idx[j]+2];
      d6 += value[j*56+44] * src[col_idx[j]+2];
      d7 += value[j*56+51] * src[col_idx[j]+2];
      d0 += value[j*56+3] * src[col_idx[j]+3];
      d1 += value[j*56+10] * src[col_idx[j]+3];
      d2 += value[j*56+17] * src[col_idx[j]+3];
      d3 += value[j*56+24] * src[col_idx[j]+3];
      d4 += value[j*56+31] * src[col_idx[j]+3];
      d5 += value[j*56+38] * src[col_idx[j]+3];
      d6 += value[j*56+45] * src[col_idx[j]+3];
      d7 += value[j*56+52] * src[col_idx[j]+3];
      d0 += value[j*56+4] * src[col_idx[j]+4];
      d1 += value[j*56+11] * src[col_idx[j]+4];
      d2 += value[j*56+18] * src[col_idx[j]+4];
      d3 += value[j*56+25] * src[col_idx[j]+4];
      d4 += value[j*56+32] * src[col_idx[j]+4];
      d5 += value[j*56+39] * src[col_idx[j]+4];
      d6 += value[j*56+46] * src[col_idx[j]+4];
      d7 += value[j*56+53] * src[col_idx[j]+4];
      d0 += value[j*56+5] * src[col_idx[j]+5];
      d1 += value[j*56+12] * src[col_idx[j]+5];
      d2 += value[j*56+19] * src[col_idx[j]+5];
      d3 += value[j*56+26] * src[col_idx[j]+5];
      d4 += value[j*56+33] * src[col_idx[j]+5];
      d5 += value[j*56+40] * src[col_idx[j]+5];
      d6 += value[j*56+47] * src[col_idx[j]+5];
      d7 += value[j*56+54] * src[col_idx[j]+5];
      d0 += value[j*56+6] * src[col_idx[j]+6];
      d1 += value[j*56+13] * src[col_idx[j]+6];
      d2 += value[j*56+20] * src[col_idx[j]+6];
      d3 += value[j*56+27] * src[col_idx[j]+6];
      d4 += value[j*56+34] * src[col_idx[j]+6];
      d5 += value[j*56+41] * src[col_idx[j]+6];
      d6 += value[j*56+48] * src[col_idx[j]+6];
      d7 += value[j*56+55] * src[col_idx[j]+6];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*64+0] * src[col_idx[j]+0];
      d1 += value[j*64+8] * src[col_idx[j]+0];
      d2 += value[j*64+16] * src[col_idx[j]+0];
      d3 += value[j*64+24] * src[col_idx[j]+0];
      d4 += value[j*64+32] * src[col_idx[j]+0];
      d5 += value[j*64+40] * src[col_idx[j]+0];
      d6 += value[j*64+48] * src[col_idx[j]+0];
      d7 += value[j*64+56] * src[col_idx[j]+0];
      d0 += value[j*64+1] * src[col_idx[j]+1];
      d1 += value[j*64+9] * src[col_idx[j]+1];
      d2 += value[j*64+17] * src[col_idx[j]+1];
      d3 += value[j*64+25] * src[col_idx[j]+1];
      d4 += value[j*64+33] * src[col_idx[j]+1];
      d5 += value[j*64+41] * src[col_idx[j]+1];
      d6 += value[j*64+49] * src[col_idx[j]+1];
      d7 += value[j*64+57] * src[col_idx[j]+1];
      d0 += value[j*64+2] * src[col_idx[j]+2];
      d1 += value[j*64+10] * src[col_idx[j]+2];
      d2 += value[j*64+18] * src[col_idx[j]+2];
      d3 += value[j*64+26] * src[col_idx[j]+2];
      d4 += value[j*64+34] * src[col_idx[j]+2];
      d5 += value[j*64+42] * src[col_idx[j]+2];
      d6 += value[j*64+50] * src[col_idx[j]+2];
      d7 += value[j*64+58] * src[col_idx[j]+2];
      d0 += value[j*64+3] * src[col_idx[j]+3];
      d1 += value[j*64+11] * src[col_idx[j]+3];
      d2 += value[j*64+19] * src[col_idx[j]+3];
      d3 += value[j*64+27] * src[col_idx[j]+3];
      d4 += value[j*64+35] * src[col_idx[j]+3];
      d5 += value[j*64+43] * src[col_idx[j]+3];
      d6 += value[j*64+51] * src[col_idx[j]+3];
      d7 += value[j*64+59] * src[col_idx[j]+3];
      d0 += value[j*64+4] * src[col_idx[j]+4];
      d1 += value[j*64+12] * src[col_idx[j]+4];
      d2 += value[j*64+20] * src[col_idx[j]+4];
      d3 += value[j*64+28] * src[col_idx[j]+4];
      d4 += value[j*64+36] * src[col_idx[j]+4];
      d5 += value[j*64+44] * src[col_idx[j]+4];
      d6 += value[j*64+52] * src[col_idx[j]+4];
      d7 += value[j*64+60] * src[col_idx[j]+4];
      d0 += value[j*64+5] * src[col_idx[j]+5];
      d1 += value[j*64+13] * src[col_idx[j]+5];
      d2 += value[j*64+21] * src[col_idx[j]+5];
      d3 += value[j*64+29] * src[col_idx[j]+5];
      d4 += value[j*64+37] * src[col_idx[j]+5];
      d5 += value[j*64+45] * src[col_idx[j]+5];
      d6 += value[j*64+53] * src[col_idx[j]+5];
      d7 += value[j*64+61] * src[col_idx[j]+5];
      d0 += value[j*64+6] * src[col_idx[j]+6];
      d1 += value[j*64+14] * src[col_idx[j]+6];
      d2 += value[j*64+22] * src[col_idx[j]+6];
      d3 += value[j*64+30] * src[col_idx[j]+6];
      d4 += value[j*64+38] * src[col_idx[j]+6];
      d5 += value[j*64+46] * src[col_idx[j]+6];
      d6 += value[j*64+54] * src[col_idx[j]+6];
      d7 += value[j*64+62] * src[col_idx[j]+6];
      d0 += value[j*64+7] * src[col_idx[j]+7];
      d1 += value[j*64+15] * src[col_idx[j]+7];
      d2 += value[j*64+23] * src[col_idx[j]+7];
      d3 += value[j*64+31] * src[col_idx[j]+7];
      d4 += value[j*64+39] * src[col_idx[j]+7];
      d5 += value[j*64+47] * src[col_idx[j]+7];
      d6 += value[j*64+55] * src[col_idx[j]+7];
      d7 += value[j*64+63] * src[col_idx[j]+7];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*72+0] * src[col_idx[j]+0];
      d1 += value[j*72+9] * src[col_idx[j]+0];
      d2 += value[j*72+18] * src[col_idx[j]+0];
      d3 += value[j*72+27] * src[col_idx[j]+0];
      d4 += value[j*72+36] * src[col_idx[j]+0];
      d5 += value[j*72+45] * src[col_idx[j]+0];
      d6 += value[j*72+54] * src[col_idx[j]+0];
      d7 += value[j*72+63] * src[col_idx[j]+0];
      d0 += value[j*72+1] * src[col_idx[j]+1];
      d1 += value[j*72+10] * src[col_idx[j]+1];
      d2 += value[j*72+19] * src[col_idx[j]+1];
      d3 += value[j*72+28] * src[col_idx[j]+1];
      d4 += value[j*72+37] * src[col_idx[j]+1];
      d5 += value[j*72+46] * src[col_idx[j]+1];
      d6 += value[j*72+55] * src[col_idx[j]+1];
      d7 += value[j*72+64] * src[col_idx[j]+1];
      d0 += value[j*72+2] * src[col_idx[j]+2];
      d1 += value[j*72+11] * src[col_idx[j]+2];
      d2 += value[j*72+20] * src[col_idx[j]+2];
      d3 += value[j*72+29] * src[col_idx[j]+2];
      d4 += value[j*72+38] * src[col_idx[j]+2];
      d5 += value[j*72+47] * src[col_idx[j]+2];
      d6 += value[j*72+56] * src[col_idx[j]+2];
      d7 += value[j*72+65] * src[col_idx[j]+2];
      d0 += value[j*72+3] * src[col_idx[j]+3];
      d1 += value[j*72+12] * src[col_idx[j]+3];
      d2 += value[j*72+21] * src[col_idx[j]+3];
      d3 += value[j*72+30] * src[col_idx[j]+3];
      d4 += value[j*72+39] * src[col_idx[j]+3];
      d5 += value[j*72+48] * src[col_idx[j]+3];
      d6 += value[j*72+57] * src[col_idx[j]+3];
      d7 += value[j*72+66] * src[col_idx[j]+3];
      d0 += value[j*72+4] * src[col_idx[j]+4];
      d1 += value[j*72+13] * src[col_idx[j]+4];
      d2 += value[j*72+22] * src[col_idx[j]+4];
      d3 += value[j*72+31] * src[col_idx[j]+4];
      d4 += value[j*72+40] * src[col_idx[j]+4];
      d5 += value[j*72+49] * src[col_idx[j]+4];
      d6 += value[j*72+58] * src[col_idx[j]+4];
      d7 += value[j*72+67] * src[col_idx[j]+4];
      d0 += value[j*72+5] * src[col_idx[j]+5];
      d1 += value[j*72+14] * src[col_idx[j]+5];
      d2 += value[j*72+23] * src[col_idx[j]+5];
      d3 += value[j*72+32] * src[col_idx[j]+5];
      d4 += value[j*72+41] * src[col_idx[j]+5];
      d5 += value[j*72+50] * src[col_idx[j]+5];
      d6 += value[j*72+59] * src[col_idx[j]+5];
      d7 += value[j*72+68] * src[col_idx[j]+5];
      d0 += value[j*72+6] * src[col_idx[j]+6];
      d1 += value[j*72+15] * src[col_idx[j]+6];
      d2 += value[j*72+24] * src[col_idx[j]+6];
      d3 += value[j*72+33] * src[col_idx[j]+6];
      d4 += value[j*72+42] * src[col_idx[j]+6];
      d5 += value[j*72+51] * src[col_idx[j]+6];
      d6 += value[j*72+60] * src[col_idx[j]+6];
      d7 += value[j*72+69] * src[col_idx[j]+6];
      d0 += value[j*72+7] * src[col_idx[j]+7];
      d1 += value[j*72+16] * src[col_idx[j]+7];
      d2 += value[j*72+25] * src[col_idx[j]+7];
      d3 += value[j*72+34] * src[col_idx[j]+7];
      d4 += value[j*72+43] * src[col_idx[j]+7];
      d5 += value[j*72+52] * src[col_idx[j]+7];
      d6 += value[j*72+61] * src[col_idx[j]+7];
      d7 += value[j*72+70] * src[col_idx[j]+7];
      d0 += value[j*72+8] * src[col_idx[j]+8];
      d1 += value[j*72+17] * src[col_idx[j]+8];
      d2 += value[j*72+26] * src[col_idx[j]+8];
      d3 += value[j*72+35] * src[col_idx[j]+8];
      d4 += value[j*72+44] * src[col_idx[j]+8];
      d5 += value[j*72+53] * src[col_idx[j]+8];
      d6 += value[j*72+62] * src[col_idx[j]+8];
      d7 += value[j*72+71] * src[col_idx[j]+8];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*80+0] * src[col_idx[j]+0];
      d1 += value[j*80+10] * src[col_idx[j]+0];
      d2 += value[j*80+20] * src[col_idx[j]+0];
      d3 += value[j*80+30] * src[col_idx[j]+0];
      d4 += value[j*80+40] * src[col_idx[j]+0];
      d5 += value[j*80+50] * src[col_idx[j]+0];
      d6 += value[j*80+60] * src[col_idx[j]+0];
      d7 += value[j*80+70] * src[col_idx[j]+0];
      d0 += value[j*80+1] * src[col_idx[j]+1];
      d1 += value[j*80+11] * src[col_idx[j]+1];
      d2 += value[j*80+21] * src[col_idx[j]+1];
      d3 += value[j*80+31] * src[col_idx[j]+1];
      d4 += value[j*80+41] * src[col_idx[j]+1];
      d5 += value[j*80+51] * src[col_idx[j]+1];
      d6 += value[j*80+61] * src[col_idx[j]+1];
      d7 += value[j*80+71] * src[col_idx[j]+1];
      d0 += value[j*80+2] * src[col_idx[j]+2];
      d1 += value[j*80+12] * src[col_idx[j]+2];
      d2 += value[j*80+22] * src[col_idx[j]+2];
      d3 += value[j*80+32] * src[col_idx[j]+2];
      d4 += value[j*80+42] * src[col_idx[j]+2];
      d5 += value[j*80+52] * src[col_idx[j]+2];
      d6 += value[j*80+62] * src[col_idx[j]+2];
      d7 += value[j*80+72] * src[col_idx[j]+2];
      d0 += value[j*80+3] * src[col_idx[j]+3];
      d1 += value[j*80+13] * src[col_idx[j]+3];
      d2 += value[j*80+23] * src[col_idx[j]+3];
      d3 += value[j*80+33] * src[col_idx[j]+3];
      d4 += value[j*80+43] * src[col_idx[j]+3];
      d5 += value[j*80+53] * src[col_idx[j]+3];
      d6 += value[j*80+63] * src[col_idx[j]+3];
      d7 += value[j*80+73] * src[col_idx[j]+3];
      d0 += value[j*80+4] * src[col_idx[j]+4];
      d1 += value[j*80+14] * src[col_idx[j]+4];
      d2 += value[j*80+24] * src[col_idx[j]+4];
      d3 += value[j*80+34] * src[col_idx[j]+4];
      d4 += value[j*80+44] * src[col_idx[j]+4];
      d5 += value[j*80+54] * src[col_idx[j]+4];
      d6 += value[j*80+64] * src[col_idx[j]+4];
      d7 += value[j*80+74] * src[col_idx[j]+4];
      d0 += value[j*80+5] * src[col_idx[j]+5];
      d1 += value[j*80+15] * src[col_idx[j]+5];
      d2 += value[j*80+25] * src[col_idx[j]+5];
      d3 += value[j*80+35] * src[col_idx[j]+5];
      d4 += value[j*80+45] * src[col_idx[j]+5];
      d5 += value[j*80+55] * src[col_idx[j]+5];
      d6 += value[j*80+65] * src[col_idx[j]+5];
      d7 += value[j*80+75] * src[col_idx[j]+5];
      d0 += value[j*80+6] * src[col_idx[j]+6];
      d1 += value[j*80+16] * src[col_idx[j]+6];
      d2 += value[j*80+26] * src[col_idx[j]+6];
      d3 += value[j*80+36] * src[col_idx[j]+6];
      d4 += value[j*80+46] * src[col_idx[j]+6];
      d5 += value[j*80+56] * src[col_idx[j]+6];
      d6 += value[j*80+66] * src[col_idx[j]+6];
      d7 += value[j*80+76] * src[col_idx[j]+6];
      d0 += value[j*80+7] * src[col_idx[j]+7];
      d1 += value[j*80+17] * src[col_idx[j]+7];
      d2 += value[j*80+27] * src[col_idx[j]+7];
      d3 += value[j*80+37] * src[col_idx[j]+7];
      d4 += value[j*80+47] * src[col_idx[j]+7];
      d5 += value[j*80+57] * src[col_idx[j]+7];
      d6 += value[j*80+67] * src[col_idx[j]+7];
      d7 += value[j*80+77] * src[col_idx[j]+7];
      d0 += value[j*80+8] * src[col_idx[j]+8];
      d1 += value[j*80+18] * src[col_idx[j]+8];
      d2 += value[j*80+28] * src[col_idx[j]+8];
      d3 += value[j*80+38] * src[col_idx[j]+8];
      d4 += value[j*80+48] * src[col_idx[j]+8];
      d5 += value[j*80+58] * src[col_idx[j]+8];
      d6 += value[j*80+68] * src[col_idx[j]+8];
      d7 += value[j*80+78] * src[col_idx[j]+8];
      d0 += value[j*80+9] * src[col_idx[j]+9];
      d1 += value[j*80+19] * src[col_idx[j]+9];
      d2 += value[j*80+29] * src[col_idx[j]+9];
      d3 += value[j*80+39] * src[col_idx[j]+9];
      d4 += value[j*80+49] * src[col_idx[j]+9];
      d5 += value[j*80+59] * src[col_idx[j]+9];
      d6 += value[j*80+69] * src[col_idx[j]+9];
      d7 += value[j*80+79] * src[col_idx[j]+9];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*88+0] * src[col_idx[j]+0];
      d1 += value[j*88+11] * src[col_idx[j]+0];
      d2 += value[j*88+22] * src[col_idx[j]+0];
      d3 += value[j*88+33] * src[col_idx[j]+0];
      d4 += value[j*88+44] * src[col_idx[j]+0];
      d5 += value[j*88+55] * src[col_idx[j]+0];
      d6 += value[j*88+66] * src[col_idx[j]+0];
      d7 += value[j*88+77] * src[col_idx[j]+0];
      d0 += value[j*88+1] * src[col_idx[j]+1];
      d1 += value[j*88+12] * src[col_idx[j]+1];
      d2 += value[j*88+23] * src[col_idx[j]+1];
      d3 += value[j*88+34] * src[col_idx[j]+1];
      d4 += value[j*88+45] * src[col_idx[j]+1];
      d5 += value[j*88+56] * src[col_idx[j]+1];
      d6 += value[j*88+67] * src[col_idx[j]+1];
      d7 += value[j*88+78] * src[col_idx[j]+1];
      d0 += value[j*88+2] * src[col_idx[j]+2];
      d1 += value[j*88+13] * src[col_idx[j]+2];
      d2 += value[j*88+24] * src[col_idx[j]+2];
      d3 += value[j*88+35] * src[col_idx[j]+2];
      d4 += value[j*88+46] * src[col_idx[j]+2];
      d5 += value[j*88+57] * src[col_idx[j]+2];
      d6 += value[j*88+68] * src[col_idx[j]+2];
      d7 += value[j*88+79] * src[col_idx[j]+2];
      d0 += value[j*88+3] * src[col_idx[j]+3];
      d1 += value[j*88+14] * src[col_idx[j]+3];
      d2 += value[j*88+25] * src[col_idx[j]+3];
      d3 += value[j*88+36] * src[col_idx[j]+3];
      d4 += value[j*88+47] * src[col_idx[j]+3];
      d5 += value[j*88+58] * src[col_idx[j]+3];
      d6 += value[j*88+69] * src[col_idx[j]+3];
      d7 += value[j*88+80] * src[col_idx[j]+3];
      d0 += value[j*88+4] * src[col_idx[j]+4];
      d1 += value[j*88+15] * src[col_idx[j]+4];
      d2 += value[j*88+26] * src[col_idx[j]+4];
      d3 += value[j*88+37] * src[col_idx[j]+4];
      d4 += value[j*88+48] * src[col_idx[j]+4];
      d5 += value[j*88+59] * src[col_idx[j]+4];
      d6 += value[j*88+70] * src[col_idx[j]+4];
      d7 += value[j*88+81] * src[col_idx[j]+4];
      d0 += value[j*88+5] * src[col_idx[j]+5];
      d1 += value[j*88+16] * src[col_idx[j]+5];
      d2 += value[j*88+27] * src[col_idx[j]+5];
      d3 += value[j*88+38] * src[col_idx[j]+5];
      d4 += value[j*88+49] * src[col_idx[j]+5];
      d5 += value[j*88+60] * src[col_idx[j]+5];
      d6 += value[j*88+71] * src[col_idx[j]+5];
      d7 += value[j*88+82] * src[col_idx[j]+5];
      d0 += value[j*88+6] * src[col_idx[j]+6];
      d1 += value[j*88+17] * src[col_idx[j]+6];
      d2 += value[j*88+28] * src[col_idx[j]+6];
      d3 += value[j*88+39] * src[col_idx[j]+6];
      d4 += value[j*88+50] * src[col_idx[j]+6];
      d5 += value[j*88+61] * src[col_idx[j]+6];
      d6 += value[j*88+72] * src[col_idx[j]+6];
      d7 += value[j*88+83] * src[col_idx[j]+6];
      d0 += value[j*88+7] * src[col_idx[j]+7];
      d1 += value[j*88+18] * src[col_idx[j]+7];
      d2 += value[j*88+29] * src[col_idx[j]+7];
      d3 += value[j*88+40] * src[col_idx[j]+7];
      d4 += value[j*88+51] * src[col_idx[j]+7];
      d5 += value[j*88+62] * src[col_idx[j]+7];
      d6 += value[j*88+73] * src[col_idx[j]+7];
      d7 += value[j*88+84] * src[col_idx[j]+7];
      d0 += value[j*88+8] * src[col_idx[j]+8];
      d1 += value[j*88+19] * src[col_idx[j]+8];
      d2 += value[j*88+30] * src[col_idx[j]+8];
      d3 += value[j*88+41] * src[col_idx[j]+8];
      d4 += value[j*88+52] * src[col_idx[j]+8];
      d5 += value[j*88+63] * src[col_idx[j]+8];
      d6 += value[j*88+74] * src[col_idx[j]+8];
      d7 += value[j*88+85] * src[col_idx[j]+8];
      d0 += value[j*88+9] * src[col_idx[j]+9];
      d1 += value[j*88+20] * src[col_idx[j]+9];
      d2 += value[j*88+31] * src[col_idx[j]+9];
      d3 += value[j*88+42] * src[col_idx[j]+9];
      d4 += value[j*88+53] * src[col_idx[j]+9];
      d5 += value[j*88+64] * src[col_idx[j]+9];
      d6 += value[j*88+75] * src[col_idx[j]+9];
      d7 += value[j*88+86] * src[col_idx[j]+9];
      d0 += value[j*88+10] * src[col_idx[j]+10];
      d1 += value[j*88+21] * src[col_idx[j]+10];
      d2 += value[j*88+32] * src[col_idx[j]+10];
      d3 += value[j*88+43] * src[col_idx[j]+10];
      d4 += value[j*88+54] * src[col_idx[j]+10];
      d5 += value[j*88+65] * src[col_idx[j]+10];
      d6 += value[j*88+76] * src[col_idx[j]+10];
      d7 += value[j*88+87] * src[col_idx[j]+10];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_8x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = dest[8*i+0];
    d1 = dest[8*i+1];
    d2 = dest[8*i+2];
    d3 = dest[8*i+3];
    d4 = dest[8*i+4];
    d5 = dest[8*i+5];
    d6 = dest[8*i+6];
    d7 = dest[8*i+7];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*96+0] * src[col_idx[j]+0];
      d1 += value[j*96+12] * src[col_idx[j]+0];
      d2 += value[j*96+24] * src[col_idx[j]+0];
      d3 += value[j*96+36] * src[col_idx[j]+0];
      d4 += value[j*96+48] * src[col_idx[j]+0];
      d5 += value[j*96+60] * src[col_idx[j]+0];
      d6 += value[j*96+72] * src[col_idx[j]+0];
      d7 += value[j*96+84] * src[col_idx[j]+0];
      d0 += value[j*96+1] * src[col_idx[j]+1];
      d1 += value[j*96+13] * src[col_idx[j]+1];
      d2 += value[j*96+25] * src[col_idx[j]+1];
      d3 += value[j*96+37] * src[col_idx[j]+1];
      d4 += value[j*96+49] * src[col_idx[j]+1];
      d5 += value[j*96+61] * src[col_idx[j]+1];
      d6 += value[j*96+73] * src[col_idx[j]+1];
      d7 += value[j*96+85] * src[col_idx[j]+1];
      d0 += value[j*96+2] * src[col_idx[j]+2];
      d1 += value[j*96+14] * src[col_idx[j]+2];
      d2 += value[j*96+26] * src[col_idx[j]+2];
      d3 += value[j*96+38] * src[col_idx[j]+2];
      d4 += value[j*96+50] * src[col_idx[j]+2];
      d5 += value[j*96+62] * src[col_idx[j]+2];
      d6 += value[j*96+74] * src[col_idx[j]+2];
      d7 += value[j*96+86] * src[col_idx[j]+2];
      d0 += value[j*96+3] * src[col_idx[j]+3];
      d1 += value[j*96+15] * src[col_idx[j]+3];
      d2 += value[j*96+27] * src[col_idx[j]+3];
      d3 += value[j*96+39] * src[col_idx[j]+3];
      d4 += value[j*96+51] * src[col_idx[j]+3];
      d5 += value[j*96+63] * src[col_idx[j]+3];
      d6 += value[j*96+75] * src[col_idx[j]+3];
      d7 += value[j*96+87] * src[col_idx[j]+3];
      d0 += value[j*96+4] * src[col_idx[j]+4];
      d1 += value[j*96+16] * src[col_idx[j]+4];
      d2 += value[j*96+28] * src[col_idx[j]+4];
      d3 += value[j*96+40] * src[col_idx[j]+4];
      d4 += value[j*96+52] * src[col_idx[j]+4];
      d5 += value[j*96+64] * src[col_idx[j]+4];
      d6 += value[j*96+76] * src[col_idx[j]+4];
      d7 += value[j*96+88] * src[col_idx[j]+4];
      d0 += value[j*96+5] * src[col_idx[j]+5];
      d1 += value[j*96+17] * src[col_idx[j]+5];
      d2 += value[j*96+29] * src[col_idx[j]+5];
      d3 += value[j*96+41] * src[col_idx[j]+5];
      d4 += value[j*96+53] * src[col_idx[j]+5];
      d5 += value[j*96+65] * src[col_idx[j]+5];
      d6 += value[j*96+77] * src[col_idx[j]+5];
      d7 += value[j*96+89] * src[col_idx[j]+5];
      d0 += value[j*96+6] * src[col_idx[j]+6];
      d1 += value[j*96+18] * src[col_idx[j]+6];
      d2 += value[j*96+30] * src[col_idx[j]+6];
      d3 += value[j*96+42] * src[col_idx[j]+6];
      d4 += value[j*96+54] * src[col_idx[j]+6];
      d5 += value[j*96+66] * src[col_idx[j]+6];
      d6 += value[j*96+78] * src[col_idx[j]+6];
      d7 += value[j*96+90] * src[col_idx[j]+6];
      d0 += value[j*96+7] * src[col_idx[j]+7];
      d1 += value[j*96+19] * src[col_idx[j]+7];
      d2 += value[j*96+31] * src[col_idx[j]+7];
      d3 += value[j*96+43] * src[col_idx[j]+7];
      d4 += value[j*96+55] * src[col_idx[j]+7];
      d5 += value[j*96+67] * src[col_idx[j]+7];
      d6 += value[j*96+79] * src[col_idx[j]+7];
      d7 += value[j*96+91] * src[col_idx[j]+7];
      d0 += value[j*96+8] * src[col_idx[j]+8];
      d1 += value[j*96+20] * src[col_idx[j]+8];
      d2 += value[j*96+32] * src[col_idx[j]+8];
      d3 += value[j*96+44] * src[col_idx[j]+8];
      d4 += value[j*96+56] * src[col_idx[j]+8];
      d5 += value[j*96+68] * src[col_idx[j]+8];
      d6 += value[j*96+80] * src[col_idx[j]+8];
      d7 += value[j*96+92] * src[col_idx[j]+8];
      d0 += value[j*96+9] * src[col_idx[j]+9];
      d1 += value[j*96+21] * src[col_idx[j]+9];
      d2 += value[j*96+33] * src[col_idx[j]+9];
      d3 += value[j*96+45] * src[col_idx[j]+9];
      d4 += value[j*96+57] * src[col_idx[j]+9];
      d5 += value[j*96+69] * src[col_idx[j]+9];
      d6 += value[j*96+81] * src[col_idx[j]+9];
      d7 += value[j*96+93] * src[col_idx[j]+9];
      d0 += value[j*96+10] * src[col_idx[j]+10];
      d1 += value[j*96+22] * src[col_idx[j]+10];
      d2 += value[j*96+34] * src[col_idx[j]+10];
      d3 += value[j*96+46] * src[col_idx[j]+10];
      d4 += value[j*96+58] * src[col_idx[j]+10];
      d5 += value[j*96+70] * src[col_idx[j]+10];
      d6 += value[j*96+82] * src[col_idx[j]+10];
      d7 += value[j*96+94] * src[col_idx[j]+10];
      d0 += value[j*96+11] * src[col_idx[j]+11];
      d1 += value[j*96+23] * src[col_idx[j]+11];
      d2 += value[j*96+35] * src[col_idx[j]+11];
      d3 += value[j*96+47] * src[col_idx[j]+11];
      d4 += value[j*96+59] * src[col_idx[j]+11];
      d5 += value[j*96+71] * src[col_idx[j]+11];
      d6 += value[j*96+83] * src[col_idx[j]+11];
      d7 += value[j*96+95] * src[col_idx[j]+11];
    }
    dest[8*i+0] = d0;
    dest[8*i+1] = d1;
    dest[8*i+2] = d2;
    dest[8*i+3] = d3;
    dest[8*i+4] = d4;
    dest[8*i+5] = d5;
    dest[8*i+6] = d6;
    dest[8*i+7] = d7;
  }
}
void bsmvm_9x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*9+0] * src[col_idx[j]+0];
      d1 += value[j*9+1] * src[col_idx[j]+0];
      d2 += value[j*9+2] * src[col_idx[j]+0];
      d3 += value[j*9+3] * src[col_idx[j]+0];
      d4 += value[j*9+4] * src[col_idx[j]+0];
      d5 += value[j*9+5] * src[col_idx[j]+0];
      d6 += value[j*9+6] * src[col_idx[j]+0];
      d7 += value[j*9+7] * src[col_idx[j]+0];
      d8 += value[j*9+8] * src[col_idx[j]+0];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*18+0] * src[col_idx[j]+0];
      d1 += value[j*18+2] * src[col_idx[j]+0];
      d2 += value[j*18+4] * src[col_idx[j]+0];
      d3 += value[j*18+6] * src[col_idx[j]+0];
      d4 += value[j*18+8] * src[col_idx[j]+0];
      d5 += value[j*18+10] * src[col_idx[j]+0];
      d6 += value[j*18+12] * src[col_idx[j]+0];
      d7 += value[j*18+14] * src[col_idx[j]+0];
      d8 += value[j*18+16] * src[col_idx[j]+0];
      d0 += value[j*18+1] * src[col_idx[j]+1];
      d1 += value[j*18+3] * src[col_idx[j]+1];
      d2 += value[j*18+5] * src[col_idx[j]+1];
      d3 += value[j*18+7] * src[col_idx[j]+1];
      d4 += value[j*18+9] * src[col_idx[j]+1];
      d5 += value[j*18+11] * src[col_idx[j]+1];
      d6 += value[j*18+13] * src[col_idx[j]+1];
      d7 += value[j*18+15] * src[col_idx[j]+1];
      d8 += value[j*18+17] * src[col_idx[j]+1];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*27+0] * src[col_idx[j]+0];
      d1 += value[j*27+3] * src[col_idx[j]+0];
      d2 += value[j*27+6] * src[col_idx[j]+0];
      d3 += value[j*27+9] * src[col_idx[j]+0];
      d4 += value[j*27+12] * src[col_idx[j]+0];
      d5 += value[j*27+15] * src[col_idx[j]+0];
      d6 += value[j*27+18] * src[col_idx[j]+0];
      d7 += value[j*27+21] * src[col_idx[j]+0];
      d8 += value[j*27+24] * src[col_idx[j]+0];
      d0 += value[j*27+1] * src[col_idx[j]+1];
      d1 += value[j*27+4] * src[col_idx[j]+1];
      d2 += value[j*27+7] * src[col_idx[j]+1];
      d3 += value[j*27+10] * src[col_idx[j]+1];
      d4 += value[j*27+13] * src[col_idx[j]+1];
      d5 += value[j*27+16] * src[col_idx[j]+1];
      d6 += value[j*27+19] * src[col_idx[j]+1];
      d7 += value[j*27+22] * src[col_idx[j]+1];
      d8 += value[j*27+25] * src[col_idx[j]+1];
      d0 += value[j*27+2] * src[col_idx[j]+2];
      d1 += value[j*27+5] * src[col_idx[j]+2];
      d2 += value[j*27+8] * src[col_idx[j]+2];
      d3 += value[j*27+11] * src[col_idx[j]+2];
      d4 += value[j*27+14] * src[col_idx[j]+2];
      d5 += value[j*27+17] * src[col_idx[j]+2];
      d6 += value[j*27+20] * src[col_idx[j]+2];
      d7 += value[j*27+23] * src[col_idx[j]+2];
      d8 += value[j*27+26] * src[col_idx[j]+2];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*36+0] * src[col_idx[j]+0];
      d1 += value[j*36+4] * src[col_idx[j]+0];
      d2 += value[j*36+8] * src[col_idx[j]+0];
      d3 += value[j*36+12] * src[col_idx[j]+0];
      d4 += value[j*36+16] * src[col_idx[j]+0];
      d5 += value[j*36+20] * src[col_idx[j]+0];
      d6 += value[j*36+24] * src[col_idx[j]+0];
      d7 += value[j*36+28] * src[col_idx[j]+0];
      d8 += value[j*36+32] * src[col_idx[j]+0];
      d0 += value[j*36+1] * src[col_idx[j]+1];
      d1 += value[j*36+5] * src[col_idx[j]+1];
      d2 += value[j*36+9] * src[col_idx[j]+1];
      d3 += value[j*36+13] * src[col_idx[j]+1];
      d4 += value[j*36+17] * src[col_idx[j]+1];
      d5 += value[j*36+21] * src[col_idx[j]+1];
      d6 += value[j*36+25] * src[col_idx[j]+1];
      d7 += value[j*36+29] * src[col_idx[j]+1];
      d8 += value[j*36+33] * src[col_idx[j]+1];
      d0 += value[j*36+2] * src[col_idx[j]+2];
      d1 += value[j*36+6] * src[col_idx[j]+2];
      d2 += value[j*36+10] * src[col_idx[j]+2];
      d3 += value[j*36+14] * src[col_idx[j]+2];
      d4 += value[j*36+18] * src[col_idx[j]+2];
      d5 += value[j*36+22] * src[col_idx[j]+2];
      d6 += value[j*36+26] * src[col_idx[j]+2];
      d7 += value[j*36+30] * src[col_idx[j]+2];
      d8 += value[j*36+34] * src[col_idx[j]+2];
      d0 += value[j*36+3] * src[col_idx[j]+3];
      d1 += value[j*36+7] * src[col_idx[j]+3];
      d2 += value[j*36+11] * src[col_idx[j]+3];
      d3 += value[j*36+15] * src[col_idx[j]+3];
      d4 += value[j*36+19] * src[col_idx[j]+3];
      d5 += value[j*36+23] * src[col_idx[j]+3];
      d6 += value[j*36+27] * src[col_idx[j]+3];
      d7 += value[j*36+31] * src[col_idx[j]+3];
      d8 += value[j*36+35] * src[col_idx[j]+3];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*45+0] * src[col_idx[j]+0];
      d1 += value[j*45+5] * src[col_idx[j]+0];
      d2 += value[j*45+10] * src[col_idx[j]+0];
      d3 += value[j*45+15] * src[col_idx[j]+0];
      d4 += value[j*45+20] * src[col_idx[j]+0];
      d5 += value[j*45+25] * src[col_idx[j]+0];
      d6 += value[j*45+30] * src[col_idx[j]+0];
      d7 += value[j*45+35] * src[col_idx[j]+0];
      d8 += value[j*45+40] * src[col_idx[j]+0];
      d0 += value[j*45+1] * src[col_idx[j]+1];
      d1 += value[j*45+6] * src[col_idx[j]+1];
      d2 += value[j*45+11] * src[col_idx[j]+1];
      d3 += value[j*45+16] * src[col_idx[j]+1];
      d4 += value[j*45+21] * src[col_idx[j]+1];
      d5 += value[j*45+26] * src[col_idx[j]+1];
      d6 += value[j*45+31] * src[col_idx[j]+1];
      d7 += value[j*45+36] * src[col_idx[j]+1];
      d8 += value[j*45+41] * src[col_idx[j]+1];
      d0 += value[j*45+2] * src[col_idx[j]+2];
      d1 += value[j*45+7] * src[col_idx[j]+2];
      d2 += value[j*45+12] * src[col_idx[j]+2];
      d3 += value[j*45+17] * src[col_idx[j]+2];
      d4 += value[j*45+22] * src[col_idx[j]+2];
      d5 += value[j*45+27] * src[col_idx[j]+2];
      d6 += value[j*45+32] * src[col_idx[j]+2];
      d7 += value[j*45+37] * src[col_idx[j]+2];
      d8 += value[j*45+42] * src[col_idx[j]+2];
      d0 += value[j*45+3] * src[col_idx[j]+3];
      d1 += value[j*45+8] * src[col_idx[j]+3];
      d2 += value[j*45+13] * src[col_idx[j]+3];
      d3 += value[j*45+18] * src[col_idx[j]+3];
      d4 += value[j*45+23] * src[col_idx[j]+3];
      d5 += value[j*45+28] * src[col_idx[j]+3];
      d6 += value[j*45+33] * src[col_idx[j]+3];
      d7 += value[j*45+38] * src[col_idx[j]+3];
      d8 += value[j*45+43] * src[col_idx[j]+3];
      d0 += value[j*45+4] * src[col_idx[j]+4];
      d1 += value[j*45+9] * src[col_idx[j]+4];
      d2 += value[j*45+14] * src[col_idx[j]+4];
      d3 += value[j*45+19] * src[col_idx[j]+4];
      d4 += value[j*45+24] * src[col_idx[j]+4];
      d5 += value[j*45+29] * src[col_idx[j]+4];
      d6 += value[j*45+34] * src[col_idx[j]+4];
      d7 += value[j*45+39] * src[col_idx[j]+4];
      d8 += value[j*45+44] * src[col_idx[j]+4];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*54+0] * src[col_idx[j]+0];
      d1 += value[j*54+6] * src[col_idx[j]+0];
      d2 += value[j*54+12] * src[col_idx[j]+0];
      d3 += value[j*54+18] * src[col_idx[j]+0];
      d4 += value[j*54+24] * src[col_idx[j]+0];
      d5 += value[j*54+30] * src[col_idx[j]+0];
      d6 += value[j*54+36] * src[col_idx[j]+0];
      d7 += value[j*54+42] * src[col_idx[j]+0];
      d8 += value[j*54+48] * src[col_idx[j]+0];
      d0 += value[j*54+1] * src[col_idx[j]+1];
      d1 += value[j*54+7] * src[col_idx[j]+1];
      d2 += value[j*54+13] * src[col_idx[j]+1];
      d3 += value[j*54+19] * src[col_idx[j]+1];
      d4 += value[j*54+25] * src[col_idx[j]+1];
      d5 += value[j*54+31] * src[col_idx[j]+1];
      d6 += value[j*54+37] * src[col_idx[j]+1];
      d7 += value[j*54+43] * src[col_idx[j]+1];
      d8 += value[j*54+49] * src[col_idx[j]+1];
      d0 += value[j*54+2] * src[col_idx[j]+2];
      d1 += value[j*54+8] * src[col_idx[j]+2];
      d2 += value[j*54+14] * src[col_idx[j]+2];
      d3 += value[j*54+20] * src[col_idx[j]+2];
      d4 += value[j*54+26] * src[col_idx[j]+2];
      d5 += value[j*54+32] * src[col_idx[j]+2];
      d6 += value[j*54+38] * src[col_idx[j]+2];
      d7 += value[j*54+44] * src[col_idx[j]+2];
      d8 += value[j*54+50] * src[col_idx[j]+2];
      d0 += value[j*54+3] * src[col_idx[j]+3];
      d1 += value[j*54+9] * src[col_idx[j]+3];
      d2 += value[j*54+15] * src[col_idx[j]+3];
      d3 += value[j*54+21] * src[col_idx[j]+3];
      d4 += value[j*54+27] * src[col_idx[j]+3];
      d5 += value[j*54+33] * src[col_idx[j]+3];
      d6 += value[j*54+39] * src[col_idx[j]+3];
      d7 += value[j*54+45] * src[col_idx[j]+3];
      d8 += value[j*54+51] * src[col_idx[j]+3];
      d0 += value[j*54+4] * src[col_idx[j]+4];
      d1 += value[j*54+10] * src[col_idx[j]+4];
      d2 += value[j*54+16] * src[col_idx[j]+4];
      d3 += value[j*54+22] * src[col_idx[j]+4];
      d4 += value[j*54+28] * src[col_idx[j]+4];
      d5 += value[j*54+34] * src[col_idx[j]+4];
      d6 += value[j*54+40] * src[col_idx[j]+4];
      d7 += value[j*54+46] * src[col_idx[j]+4];
      d8 += value[j*54+52] * src[col_idx[j]+4];
      d0 += value[j*54+5] * src[col_idx[j]+5];
      d1 += value[j*54+11] * src[col_idx[j]+5];
      d2 += value[j*54+17] * src[col_idx[j]+5];
      d3 += value[j*54+23] * src[col_idx[j]+5];
      d4 += value[j*54+29] * src[col_idx[j]+5];
      d5 += value[j*54+35] * src[col_idx[j]+5];
      d6 += value[j*54+41] * src[col_idx[j]+5];
      d7 += value[j*54+47] * src[col_idx[j]+5];
      d8 += value[j*54+53] * src[col_idx[j]+5];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*63+0] * src[col_idx[j]+0];
      d1 += value[j*63+7] * src[col_idx[j]+0];
      d2 += value[j*63+14] * src[col_idx[j]+0];
      d3 += value[j*63+21] * src[col_idx[j]+0];
      d4 += value[j*63+28] * src[col_idx[j]+0];
      d5 += value[j*63+35] * src[col_idx[j]+0];
      d6 += value[j*63+42] * src[col_idx[j]+0];
      d7 += value[j*63+49] * src[col_idx[j]+0];
      d8 += value[j*63+56] * src[col_idx[j]+0];
      d0 += value[j*63+1] * src[col_idx[j]+1];
      d1 += value[j*63+8] * src[col_idx[j]+1];
      d2 += value[j*63+15] * src[col_idx[j]+1];
      d3 += value[j*63+22] * src[col_idx[j]+1];
      d4 += value[j*63+29] * src[col_idx[j]+1];
      d5 += value[j*63+36] * src[col_idx[j]+1];
      d6 += value[j*63+43] * src[col_idx[j]+1];
      d7 += value[j*63+50] * src[col_idx[j]+1];
      d8 += value[j*63+57] * src[col_idx[j]+1];
      d0 += value[j*63+2] * src[col_idx[j]+2];
      d1 += value[j*63+9] * src[col_idx[j]+2];
      d2 += value[j*63+16] * src[col_idx[j]+2];
      d3 += value[j*63+23] * src[col_idx[j]+2];
      d4 += value[j*63+30] * src[col_idx[j]+2];
      d5 += value[j*63+37] * src[col_idx[j]+2];
      d6 += value[j*63+44] * src[col_idx[j]+2];
      d7 += value[j*63+51] * src[col_idx[j]+2];
      d8 += value[j*63+58] * src[col_idx[j]+2];
      d0 += value[j*63+3] * src[col_idx[j]+3];
      d1 += value[j*63+10] * src[col_idx[j]+3];
      d2 += value[j*63+17] * src[col_idx[j]+3];
      d3 += value[j*63+24] * src[col_idx[j]+3];
      d4 += value[j*63+31] * src[col_idx[j]+3];
      d5 += value[j*63+38] * src[col_idx[j]+3];
      d6 += value[j*63+45] * src[col_idx[j]+3];
      d7 += value[j*63+52] * src[col_idx[j]+3];
      d8 += value[j*63+59] * src[col_idx[j]+3];
      d0 += value[j*63+4] * src[col_idx[j]+4];
      d1 += value[j*63+11] * src[col_idx[j]+4];
      d2 += value[j*63+18] * src[col_idx[j]+4];
      d3 += value[j*63+25] * src[col_idx[j]+4];
      d4 += value[j*63+32] * src[col_idx[j]+4];
      d5 += value[j*63+39] * src[col_idx[j]+4];
      d6 += value[j*63+46] * src[col_idx[j]+4];
      d7 += value[j*63+53] * src[col_idx[j]+4];
      d8 += value[j*63+60] * src[col_idx[j]+4];
      d0 += value[j*63+5] * src[col_idx[j]+5];
      d1 += value[j*63+12] * src[col_idx[j]+5];
      d2 += value[j*63+19] * src[col_idx[j]+5];
      d3 += value[j*63+26] * src[col_idx[j]+5];
      d4 += value[j*63+33] * src[col_idx[j]+5];
      d5 += value[j*63+40] * src[col_idx[j]+5];
      d6 += value[j*63+47] * src[col_idx[j]+5];
      d7 += value[j*63+54] * src[col_idx[j]+5];
      d8 += value[j*63+61] * src[col_idx[j]+5];
      d0 += value[j*63+6] * src[col_idx[j]+6];
      d1 += value[j*63+13] * src[col_idx[j]+6];
      d2 += value[j*63+20] * src[col_idx[j]+6];
      d3 += value[j*63+27] * src[col_idx[j]+6];
      d4 += value[j*63+34] * src[col_idx[j]+6];
      d5 += value[j*63+41] * src[col_idx[j]+6];
      d6 += value[j*63+48] * src[col_idx[j]+6];
      d7 += value[j*63+55] * src[col_idx[j]+6];
      d8 += value[j*63+62] * src[col_idx[j]+6];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*72+0] * src[col_idx[j]+0];
      d1 += value[j*72+8] * src[col_idx[j]+0];
      d2 += value[j*72+16] * src[col_idx[j]+0];
      d3 += value[j*72+24] * src[col_idx[j]+0];
      d4 += value[j*72+32] * src[col_idx[j]+0];
      d5 += value[j*72+40] * src[col_idx[j]+0];
      d6 += value[j*72+48] * src[col_idx[j]+0];
      d7 += value[j*72+56] * src[col_idx[j]+0];
      d8 += value[j*72+64] * src[col_idx[j]+0];
      d0 += value[j*72+1] * src[col_idx[j]+1];
      d1 += value[j*72+9] * src[col_idx[j]+1];
      d2 += value[j*72+17] * src[col_idx[j]+1];
      d3 += value[j*72+25] * src[col_idx[j]+1];
      d4 += value[j*72+33] * src[col_idx[j]+1];
      d5 += value[j*72+41] * src[col_idx[j]+1];
      d6 += value[j*72+49] * src[col_idx[j]+1];
      d7 += value[j*72+57] * src[col_idx[j]+1];
      d8 += value[j*72+65] * src[col_idx[j]+1];
      d0 += value[j*72+2] * src[col_idx[j]+2];
      d1 += value[j*72+10] * src[col_idx[j]+2];
      d2 += value[j*72+18] * src[col_idx[j]+2];
      d3 += value[j*72+26] * src[col_idx[j]+2];
      d4 += value[j*72+34] * src[col_idx[j]+2];
      d5 += value[j*72+42] * src[col_idx[j]+2];
      d6 += value[j*72+50] * src[col_idx[j]+2];
      d7 += value[j*72+58] * src[col_idx[j]+2];
      d8 += value[j*72+66] * src[col_idx[j]+2];
      d0 += value[j*72+3] * src[col_idx[j]+3];
      d1 += value[j*72+11] * src[col_idx[j]+3];
      d2 += value[j*72+19] * src[col_idx[j]+3];
      d3 += value[j*72+27] * src[col_idx[j]+3];
      d4 += value[j*72+35] * src[col_idx[j]+3];
      d5 += value[j*72+43] * src[col_idx[j]+3];
      d6 += value[j*72+51] * src[col_idx[j]+3];
      d7 += value[j*72+59] * src[col_idx[j]+3];
      d8 += value[j*72+67] * src[col_idx[j]+3];
      d0 += value[j*72+4] * src[col_idx[j]+4];
      d1 += value[j*72+12] * src[col_idx[j]+4];
      d2 += value[j*72+20] * src[col_idx[j]+4];
      d3 += value[j*72+28] * src[col_idx[j]+4];
      d4 += value[j*72+36] * src[col_idx[j]+4];
      d5 += value[j*72+44] * src[col_idx[j]+4];
      d6 += value[j*72+52] * src[col_idx[j]+4];
      d7 += value[j*72+60] * src[col_idx[j]+4];
      d8 += value[j*72+68] * src[col_idx[j]+4];
      d0 += value[j*72+5] * src[col_idx[j]+5];
      d1 += value[j*72+13] * src[col_idx[j]+5];
      d2 += value[j*72+21] * src[col_idx[j]+5];
      d3 += value[j*72+29] * src[col_idx[j]+5];
      d4 += value[j*72+37] * src[col_idx[j]+5];
      d5 += value[j*72+45] * src[col_idx[j]+5];
      d6 += value[j*72+53] * src[col_idx[j]+5];
      d7 += value[j*72+61] * src[col_idx[j]+5];
      d8 += value[j*72+69] * src[col_idx[j]+5];
      d0 += value[j*72+6] * src[col_idx[j]+6];
      d1 += value[j*72+14] * src[col_idx[j]+6];
      d2 += value[j*72+22] * src[col_idx[j]+6];
      d3 += value[j*72+30] * src[col_idx[j]+6];
      d4 += value[j*72+38] * src[col_idx[j]+6];
      d5 += value[j*72+46] * src[col_idx[j]+6];
      d6 += value[j*72+54] * src[col_idx[j]+6];
      d7 += value[j*72+62] * src[col_idx[j]+6];
      d8 += value[j*72+70] * src[col_idx[j]+6];
      d0 += value[j*72+7] * src[col_idx[j]+7];
      d1 += value[j*72+15] * src[col_idx[j]+7];
      d2 += value[j*72+23] * src[col_idx[j]+7];
      d3 += value[j*72+31] * src[col_idx[j]+7];
      d4 += value[j*72+39] * src[col_idx[j]+7];
      d5 += value[j*72+47] * src[col_idx[j]+7];
      d6 += value[j*72+55] * src[col_idx[j]+7];
      d7 += value[j*72+63] * src[col_idx[j]+7];
      d8 += value[j*72+71] * src[col_idx[j]+7];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*81+0] * src[col_idx[j]+0];
      d1 += value[j*81+9] * src[col_idx[j]+0];
      d2 += value[j*81+18] * src[col_idx[j]+0];
      d3 += value[j*81+27] * src[col_idx[j]+0];
      d4 += value[j*81+36] * src[col_idx[j]+0];
      d5 += value[j*81+45] * src[col_idx[j]+0];
      d6 += value[j*81+54] * src[col_idx[j]+0];
      d7 += value[j*81+63] * src[col_idx[j]+0];
      d8 += value[j*81+72] * src[col_idx[j]+0];
      d0 += value[j*81+1] * src[col_idx[j]+1];
      d1 += value[j*81+10] * src[col_idx[j]+1];
      d2 += value[j*81+19] * src[col_idx[j]+1];
      d3 += value[j*81+28] * src[col_idx[j]+1];
      d4 += value[j*81+37] * src[col_idx[j]+1];
      d5 += value[j*81+46] * src[col_idx[j]+1];
      d6 += value[j*81+55] * src[col_idx[j]+1];
      d7 += value[j*81+64] * src[col_idx[j]+1];
      d8 += value[j*81+73] * src[col_idx[j]+1];
      d0 += value[j*81+2] * src[col_idx[j]+2];
      d1 += value[j*81+11] * src[col_idx[j]+2];
      d2 += value[j*81+20] * src[col_idx[j]+2];
      d3 += value[j*81+29] * src[col_idx[j]+2];
      d4 += value[j*81+38] * src[col_idx[j]+2];
      d5 += value[j*81+47] * src[col_idx[j]+2];
      d6 += value[j*81+56] * src[col_idx[j]+2];
      d7 += value[j*81+65] * src[col_idx[j]+2];
      d8 += value[j*81+74] * src[col_idx[j]+2];
      d0 += value[j*81+3] * src[col_idx[j]+3];
      d1 += value[j*81+12] * src[col_idx[j]+3];
      d2 += value[j*81+21] * src[col_idx[j]+3];
      d3 += value[j*81+30] * src[col_idx[j]+3];
      d4 += value[j*81+39] * src[col_idx[j]+3];
      d5 += value[j*81+48] * src[col_idx[j]+3];
      d6 += value[j*81+57] * src[col_idx[j]+3];
      d7 += value[j*81+66] * src[col_idx[j]+3];
      d8 += value[j*81+75] * src[col_idx[j]+3];
      d0 += value[j*81+4] * src[col_idx[j]+4];
      d1 += value[j*81+13] * src[col_idx[j]+4];
      d2 += value[j*81+22] * src[col_idx[j]+4];
      d3 += value[j*81+31] * src[col_idx[j]+4];
      d4 += value[j*81+40] * src[col_idx[j]+4];
      d5 += value[j*81+49] * src[col_idx[j]+4];
      d6 += value[j*81+58] * src[col_idx[j]+4];
      d7 += value[j*81+67] * src[col_idx[j]+4];
      d8 += value[j*81+76] * src[col_idx[j]+4];
      d0 += value[j*81+5] * src[col_idx[j]+5];
      d1 += value[j*81+14] * src[col_idx[j]+5];
      d2 += value[j*81+23] * src[col_idx[j]+5];
      d3 += value[j*81+32] * src[col_idx[j]+5];
      d4 += value[j*81+41] * src[col_idx[j]+5];
      d5 += value[j*81+50] * src[col_idx[j]+5];
      d6 += value[j*81+59] * src[col_idx[j]+5];
      d7 += value[j*81+68] * src[col_idx[j]+5];
      d8 += value[j*81+77] * src[col_idx[j]+5];
      d0 += value[j*81+6] * src[col_idx[j]+6];
      d1 += value[j*81+15] * src[col_idx[j]+6];
      d2 += value[j*81+24] * src[col_idx[j]+6];
      d3 += value[j*81+33] * src[col_idx[j]+6];
      d4 += value[j*81+42] * src[col_idx[j]+6];
      d5 += value[j*81+51] * src[col_idx[j]+6];
      d6 += value[j*81+60] * src[col_idx[j]+6];
      d7 += value[j*81+69] * src[col_idx[j]+6];
      d8 += value[j*81+78] * src[col_idx[j]+6];
      d0 += value[j*81+7] * src[col_idx[j]+7];
      d1 += value[j*81+16] * src[col_idx[j]+7];
      d2 += value[j*81+25] * src[col_idx[j]+7];
      d3 += value[j*81+34] * src[col_idx[j]+7];
      d4 += value[j*81+43] * src[col_idx[j]+7];
      d5 += value[j*81+52] * src[col_idx[j]+7];
      d6 += value[j*81+61] * src[col_idx[j]+7];
      d7 += value[j*81+70] * src[col_idx[j]+7];
      d8 += value[j*81+79] * src[col_idx[j]+7];
      d0 += value[j*81+8] * src[col_idx[j]+8];
      d1 += value[j*81+17] * src[col_idx[j]+8];
      d2 += value[j*81+26] * src[col_idx[j]+8];
      d3 += value[j*81+35] * src[col_idx[j]+8];
      d4 += value[j*81+44] * src[col_idx[j]+8];
      d5 += value[j*81+53] * src[col_idx[j]+8];
      d6 += value[j*81+62] * src[col_idx[j]+8];
      d7 += value[j*81+71] * src[col_idx[j]+8];
      d8 += value[j*81+80] * src[col_idx[j]+8];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*90+0] * src[col_idx[j]+0];
      d1 += value[j*90+10] * src[col_idx[j]+0];
      d2 += value[j*90+20] * src[col_idx[j]+0];
      d3 += value[j*90+30] * src[col_idx[j]+0];
      d4 += value[j*90+40] * src[col_idx[j]+0];
      d5 += value[j*90+50] * src[col_idx[j]+0];
      d6 += value[j*90+60] * src[col_idx[j]+0];
      d7 += value[j*90+70] * src[col_idx[j]+0];
      d8 += value[j*90+80] * src[col_idx[j]+0];
      d0 += value[j*90+1] * src[col_idx[j]+1];
      d1 += value[j*90+11] * src[col_idx[j]+1];
      d2 += value[j*90+21] * src[col_idx[j]+1];
      d3 += value[j*90+31] * src[col_idx[j]+1];
      d4 += value[j*90+41] * src[col_idx[j]+1];
      d5 += value[j*90+51] * src[col_idx[j]+1];
      d6 += value[j*90+61] * src[col_idx[j]+1];
      d7 += value[j*90+71] * src[col_idx[j]+1];
      d8 += value[j*90+81] * src[col_idx[j]+1];
      d0 += value[j*90+2] * src[col_idx[j]+2];
      d1 += value[j*90+12] * src[col_idx[j]+2];
      d2 += value[j*90+22] * src[col_idx[j]+2];
      d3 += value[j*90+32] * src[col_idx[j]+2];
      d4 += value[j*90+42] * src[col_idx[j]+2];
      d5 += value[j*90+52] * src[col_idx[j]+2];
      d6 += value[j*90+62] * src[col_idx[j]+2];
      d7 += value[j*90+72] * src[col_idx[j]+2];
      d8 += value[j*90+82] * src[col_idx[j]+2];
      d0 += value[j*90+3] * src[col_idx[j]+3];
      d1 += value[j*90+13] * src[col_idx[j]+3];
      d2 += value[j*90+23] * src[col_idx[j]+3];
      d3 += value[j*90+33] * src[col_idx[j]+3];
      d4 += value[j*90+43] * src[col_idx[j]+3];
      d5 += value[j*90+53] * src[col_idx[j]+3];
      d6 += value[j*90+63] * src[col_idx[j]+3];
      d7 += value[j*90+73] * src[col_idx[j]+3];
      d8 += value[j*90+83] * src[col_idx[j]+3];
      d0 += value[j*90+4] * src[col_idx[j]+4];
      d1 += value[j*90+14] * src[col_idx[j]+4];
      d2 += value[j*90+24] * src[col_idx[j]+4];
      d3 += value[j*90+34] * src[col_idx[j]+4];
      d4 += value[j*90+44] * src[col_idx[j]+4];
      d5 += value[j*90+54] * src[col_idx[j]+4];
      d6 += value[j*90+64] * src[col_idx[j]+4];
      d7 += value[j*90+74] * src[col_idx[j]+4];
      d8 += value[j*90+84] * src[col_idx[j]+4];
      d0 += value[j*90+5] * src[col_idx[j]+5];
      d1 += value[j*90+15] * src[col_idx[j]+5];
      d2 += value[j*90+25] * src[col_idx[j]+5];
      d3 += value[j*90+35] * src[col_idx[j]+5];
      d4 += value[j*90+45] * src[col_idx[j]+5];
      d5 += value[j*90+55] * src[col_idx[j]+5];
      d6 += value[j*90+65] * src[col_idx[j]+5];
      d7 += value[j*90+75] * src[col_idx[j]+5];
      d8 += value[j*90+85] * src[col_idx[j]+5];
      d0 += value[j*90+6] * src[col_idx[j]+6];
      d1 += value[j*90+16] * src[col_idx[j]+6];
      d2 += value[j*90+26] * src[col_idx[j]+6];
      d3 += value[j*90+36] * src[col_idx[j]+6];
      d4 += value[j*90+46] * src[col_idx[j]+6];
      d5 += value[j*90+56] * src[col_idx[j]+6];
      d6 += value[j*90+66] * src[col_idx[j]+6];
      d7 += value[j*90+76] * src[col_idx[j]+6];
      d8 += value[j*90+86] * src[col_idx[j]+6];
      d0 += value[j*90+7] * src[col_idx[j]+7];
      d1 += value[j*90+17] * src[col_idx[j]+7];
      d2 += value[j*90+27] * src[col_idx[j]+7];
      d3 += value[j*90+37] * src[col_idx[j]+7];
      d4 += value[j*90+47] * src[col_idx[j]+7];
      d5 += value[j*90+57] * src[col_idx[j]+7];
      d6 += value[j*90+67] * src[col_idx[j]+7];
      d7 += value[j*90+77] * src[col_idx[j]+7];
      d8 += value[j*90+87] * src[col_idx[j]+7];
      d0 += value[j*90+8] * src[col_idx[j]+8];
      d1 += value[j*90+18] * src[col_idx[j]+8];
      d2 += value[j*90+28] * src[col_idx[j]+8];
      d3 += value[j*90+38] * src[col_idx[j]+8];
      d4 += value[j*90+48] * src[col_idx[j]+8];
      d5 += value[j*90+58] * src[col_idx[j]+8];
      d6 += value[j*90+68] * src[col_idx[j]+8];
      d7 += value[j*90+78] * src[col_idx[j]+8];
      d8 += value[j*90+88] * src[col_idx[j]+8];
      d0 += value[j*90+9] * src[col_idx[j]+9];
      d1 += value[j*90+19] * src[col_idx[j]+9];
      d2 += value[j*90+29] * src[col_idx[j]+9];
      d3 += value[j*90+39] * src[col_idx[j]+9];
      d4 += value[j*90+49] * src[col_idx[j]+9];
      d5 += value[j*90+59] * src[col_idx[j]+9];
      d6 += value[j*90+69] * src[col_idx[j]+9];
      d7 += value[j*90+79] * src[col_idx[j]+9];
      d8 += value[j*90+89] * src[col_idx[j]+9];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*99+0] * src[col_idx[j]+0];
      d1 += value[j*99+11] * src[col_idx[j]+0];
      d2 += value[j*99+22] * src[col_idx[j]+0];
      d3 += value[j*99+33] * src[col_idx[j]+0];
      d4 += value[j*99+44] * src[col_idx[j]+0];
      d5 += value[j*99+55] * src[col_idx[j]+0];
      d6 += value[j*99+66] * src[col_idx[j]+0];
      d7 += value[j*99+77] * src[col_idx[j]+0];
      d8 += value[j*99+88] * src[col_idx[j]+0];
      d0 += value[j*99+1] * src[col_idx[j]+1];
      d1 += value[j*99+12] * src[col_idx[j]+1];
      d2 += value[j*99+23] * src[col_idx[j]+1];
      d3 += value[j*99+34] * src[col_idx[j]+1];
      d4 += value[j*99+45] * src[col_idx[j]+1];
      d5 += value[j*99+56] * src[col_idx[j]+1];
      d6 += value[j*99+67] * src[col_idx[j]+1];
      d7 += value[j*99+78] * src[col_idx[j]+1];
      d8 += value[j*99+89] * src[col_idx[j]+1];
      d0 += value[j*99+2] * src[col_idx[j]+2];
      d1 += value[j*99+13] * src[col_idx[j]+2];
      d2 += value[j*99+24] * src[col_idx[j]+2];
      d3 += value[j*99+35] * src[col_idx[j]+2];
      d4 += value[j*99+46] * src[col_idx[j]+2];
      d5 += value[j*99+57] * src[col_idx[j]+2];
      d6 += value[j*99+68] * src[col_idx[j]+2];
      d7 += value[j*99+79] * src[col_idx[j]+2];
      d8 += value[j*99+90] * src[col_idx[j]+2];
      d0 += value[j*99+3] * src[col_idx[j]+3];
      d1 += value[j*99+14] * src[col_idx[j]+3];
      d2 += value[j*99+25] * src[col_idx[j]+3];
      d3 += value[j*99+36] * src[col_idx[j]+3];
      d4 += value[j*99+47] * src[col_idx[j]+3];
      d5 += value[j*99+58] * src[col_idx[j]+3];
      d6 += value[j*99+69] * src[col_idx[j]+3];
      d7 += value[j*99+80] * src[col_idx[j]+3];
      d8 += value[j*99+91] * src[col_idx[j]+3];
      d0 += value[j*99+4] * src[col_idx[j]+4];
      d1 += value[j*99+15] * src[col_idx[j]+4];
      d2 += value[j*99+26] * src[col_idx[j]+4];
      d3 += value[j*99+37] * src[col_idx[j]+4];
      d4 += value[j*99+48] * src[col_idx[j]+4];
      d5 += value[j*99+59] * src[col_idx[j]+4];
      d6 += value[j*99+70] * src[col_idx[j]+4];
      d7 += value[j*99+81] * src[col_idx[j]+4];
      d8 += value[j*99+92] * src[col_idx[j]+4];
      d0 += value[j*99+5] * src[col_idx[j]+5];
      d1 += value[j*99+16] * src[col_idx[j]+5];
      d2 += value[j*99+27] * src[col_idx[j]+5];
      d3 += value[j*99+38] * src[col_idx[j]+5];
      d4 += value[j*99+49] * src[col_idx[j]+5];
      d5 += value[j*99+60] * src[col_idx[j]+5];
      d6 += value[j*99+71] * src[col_idx[j]+5];
      d7 += value[j*99+82] * src[col_idx[j]+5];
      d8 += value[j*99+93] * src[col_idx[j]+5];
      d0 += value[j*99+6] * src[col_idx[j]+6];
      d1 += value[j*99+17] * src[col_idx[j]+6];
      d2 += value[j*99+28] * src[col_idx[j]+6];
      d3 += value[j*99+39] * src[col_idx[j]+6];
      d4 += value[j*99+50] * src[col_idx[j]+6];
      d5 += value[j*99+61] * src[col_idx[j]+6];
      d6 += value[j*99+72] * src[col_idx[j]+6];
      d7 += value[j*99+83] * src[col_idx[j]+6];
      d8 += value[j*99+94] * src[col_idx[j]+6];
      d0 += value[j*99+7] * src[col_idx[j]+7];
      d1 += value[j*99+18] * src[col_idx[j]+7];
      d2 += value[j*99+29] * src[col_idx[j]+7];
      d3 += value[j*99+40] * src[col_idx[j]+7];
      d4 += value[j*99+51] * src[col_idx[j]+7];
      d5 += value[j*99+62] * src[col_idx[j]+7];
      d6 += value[j*99+73] * src[col_idx[j]+7];
      d7 += value[j*99+84] * src[col_idx[j]+7];
      d8 += value[j*99+95] * src[col_idx[j]+7];
      d0 += value[j*99+8] * src[col_idx[j]+8];
      d1 += value[j*99+19] * src[col_idx[j]+8];
      d2 += value[j*99+30] * src[col_idx[j]+8];
      d3 += value[j*99+41] * src[col_idx[j]+8];
      d4 += value[j*99+52] * src[col_idx[j]+8];
      d5 += value[j*99+63] * src[col_idx[j]+8];
      d6 += value[j*99+74] * src[col_idx[j]+8];
      d7 += value[j*99+85] * src[col_idx[j]+8];
      d8 += value[j*99+96] * src[col_idx[j]+8];
      d0 += value[j*99+9] * src[col_idx[j]+9];
      d1 += value[j*99+20] * src[col_idx[j]+9];
      d2 += value[j*99+31] * src[col_idx[j]+9];
      d3 += value[j*99+42] * src[col_idx[j]+9];
      d4 += value[j*99+53] * src[col_idx[j]+9];
      d5 += value[j*99+64] * src[col_idx[j]+9];
      d6 += value[j*99+75] * src[col_idx[j]+9];
      d7 += value[j*99+86] * src[col_idx[j]+9];
      d8 += value[j*99+97] * src[col_idx[j]+9];
      d0 += value[j*99+10] * src[col_idx[j]+10];
      d1 += value[j*99+21] * src[col_idx[j]+10];
      d2 += value[j*99+32] * src[col_idx[j]+10];
      d3 += value[j*99+43] * src[col_idx[j]+10];
      d4 += value[j*99+54] * src[col_idx[j]+10];
      d5 += value[j*99+65] * src[col_idx[j]+10];
      d6 += value[j*99+76] * src[col_idx[j]+10];
      d7 += value[j*99+87] * src[col_idx[j]+10];
      d8 += value[j*99+98] * src[col_idx[j]+10];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_9x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8;
    d0 = dest[9*i+0];
    d1 = dest[9*i+1];
    d2 = dest[9*i+2];
    d3 = dest[9*i+3];
    d4 = dest[9*i+4];
    d5 = dest[9*i+5];
    d6 = dest[9*i+6];
    d7 = dest[9*i+7];
    d8 = dest[9*i+8];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*108+0] * src[col_idx[j]+0];
      d1 += value[j*108+12] * src[col_idx[j]+0];
      d2 += value[j*108+24] * src[col_idx[j]+0];
      d3 += value[j*108+36] * src[col_idx[j]+0];
      d4 += value[j*108+48] * src[col_idx[j]+0];
      d5 += value[j*108+60] * src[col_idx[j]+0];
      d6 += value[j*108+72] * src[col_idx[j]+0];
      d7 += value[j*108+84] * src[col_idx[j]+0];
      d8 += value[j*108+96] * src[col_idx[j]+0];
      d0 += value[j*108+1] * src[col_idx[j]+1];
      d1 += value[j*108+13] * src[col_idx[j]+1];
      d2 += value[j*108+25] * src[col_idx[j]+1];
      d3 += value[j*108+37] * src[col_idx[j]+1];
      d4 += value[j*108+49] * src[col_idx[j]+1];
      d5 += value[j*108+61] * src[col_idx[j]+1];
      d6 += value[j*108+73] * src[col_idx[j]+1];
      d7 += value[j*108+85] * src[col_idx[j]+1];
      d8 += value[j*108+97] * src[col_idx[j]+1];
      d0 += value[j*108+2] * src[col_idx[j]+2];
      d1 += value[j*108+14] * src[col_idx[j]+2];
      d2 += value[j*108+26] * src[col_idx[j]+2];
      d3 += value[j*108+38] * src[col_idx[j]+2];
      d4 += value[j*108+50] * src[col_idx[j]+2];
      d5 += value[j*108+62] * src[col_idx[j]+2];
      d6 += value[j*108+74] * src[col_idx[j]+2];
      d7 += value[j*108+86] * src[col_idx[j]+2];
      d8 += value[j*108+98] * src[col_idx[j]+2];
      d0 += value[j*108+3] * src[col_idx[j]+3];
      d1 += value[j*108+15] * src[col_idx[j]+3];
      d2 += value[j*108+27] * src[col_idx[j]+3];
      d3 += value[j*108+39] * src[col_idx[j]+3];
      d4 += value[j*108+51] * src[col_idx[j]+3];
      d5 += value[j*108+63] * src[col_idx[j]+3];
      d6 += value[j*108+75] * src[col_idx[j]+3];
      d7 += value[j*108+87] * src[col_idx[j]+3];
      d8 += value[j*108+99] * src[col_idx[j]+3];
      d0 += value[j*108+4] * src[col_idx[j]+4];
      d1 += value[j*108+16] * src[col_idx[j]+4];
      d2 += value[j*108+28] * src[col_idx[j]+4];
      d3 += value[j*108+40] * src[col_idx[j]+4];
      d4 += value[j*108+52] * src[col_idx[j]+4];
      d5 += value[j*108+64] * src[col_idx[j]+4];
      d6 += value[j*108+76] * src[col_idx[j]+4];
      d7 += value[j*108+88] * src[col_idx[j]+4];
      d8 += value[j*108+100] * src[col_idx[j]+4];
      d0 += value[j*108+5] * src[col_idx[j]+5];
      d1 += value[j*108+17] * src[col_idx[j]+5];
      d2 += value[j*108+29] * src[col_idx[j]+5];
      d3 += value[j*108+41] * src[col_idx[j]+5];
      d4 += value[j*108+53] * src[col_idx[j]+5];
      d5 += value[j*108+65] * src[col_idx[j]+5];
      d6 += value[j*108+77] * src[col_idx[j]+5];
      d7 += value[j*108+89] * src[col_idx[j]+5];
      d8 += value[j*108+101] * src[col_idx[j]+5];
      d0 += value[j*108+6] * src[col_idx[j]+6];
      d1 += value[j*108+18] * src[col_idx[j]+6];
      d2 += value[j*108+30] * src[col_idx[j]+6];
      d3 += value[j*108+42] * src[col_idx[j]+6];
      d4 += value[j*108+54] * src[col_idx[j]+6];
      d5 += value[j*108+66] * src[col_idx[j]+6];
      d6 += value[j*108+78] * src[col_idx[j]+6];
      d7 += value[j*108+90] * src[col_idx[j]+6];
      d8 += value[j*108+102] * src[col_idx[j]+6];
      d0 += value[j*108+7] * src[col_idx[j]+7];
      d1 += value[j*108+19] * src[col_idx[j]+7];
      d2 += value[j*108+31] * src[col_idx[j]+7];
      d3 += value[j*108+43] * src[col_idx[j]+7];
      d4 += value[j*108+55] * src[col_idx[j]+7];
      d5 += value[j*108+67] * src[col_idx[j]+7];
      d6 += value[j*108+79] * src[col_idx[j]+7];
      d7 += value[j*108+91] * src[col_idx[j]+7];
      d8 += value[j*108+103] * src[col_idx[j]+7];
      d0 += value[j*108+8] * src[col_idx[j]+8];
      d1 += value[j*108+20] * src[col_idx[j]+8];
      d2 += value[j*108+32] * src[col_idx[j]+8];
      d3 += value[j*108+44] * src[col_idx[j]+8];
      d4 += value[j*108+56] * src[col_idx[j]+8];
      d5 += value[j*108+68] * src[col_idx[j]+8];
      d6 += value[j*108+80] * src[col_idx[j]+8];
      d7 += value[j*108+92] * src[col_idx[j]+8];
      d8 += value[j*108+104] * src[col_idx[j]+8];
      d0 += value[j*108+9] * src[col_idx[j]+9];
      d1 += value[j*108+21] * src[col_idx[j]+9];
      d2 += value[j*108+33] * src[col_idx[j]+9];
      d3 += value[j*108+45] * src[col_idx[j]+9];
      d4 += value[j*108+57] * src[col_idx[j]+9];
      d5 += value[j*108+69] * src[col_idx[j]+9];
      d6 += value[j*108+81] * src[col_idx[j]+9];
      d7 += value[j*108+93] * src[col_idx[j]+9];
      d8 += value[j*108+105] * src[col_idx[j]+9];
      d0 += value[j*108+10] * src[col_idx[j]+10];
      d1 += value[j*108+22] * src[col_idx[j]+10];
      d2 += value[j*108+34] * src[col_idx[j]+10];
      d3 += value[j*108+46] * src[col_idx[j]+10];
      d4 += value[j*108+58] * src[col_idx[j]+10];
      d5 += value[j*108+70] * src[col_idx[j]+10];
      d6 += value[j*108+82] * src[col_idx[j]+10];
      d7 += value[j*108+94] * src[col_idx[j]+10];
      d8 += value[j*108+106] * src[col_idx[j]+10];
      d0 += value[j*108+11] * src[col_idx[j]+11];
      d1 += value[j*108+23] * src[col_idx[j]+11];
      d2 += value[j*108+35] * src[col_idx[j]+11];
      d3 += value[j*108+47] * src[col_idx[j]+11];
      d4 += value[j*108+59] * src[col_idx[j]+11];
      d5 += value[j*108+71] * src[col_idx[j]+11];
      d6 += value[j*108+83] * src[col_idx[j]+11];
      d7 += value[j*108+95] * src[col_idx[j]+11];
      d8 += value[j*108+107] * src[col_idx[j]+11];
    }
    dest[9*i+0] = d0;
    dest[9*i+1] = d1;
    dest[9*i+2] = d2;
    dest[9*i+3] = d3;
    dest[9*i+4] = d4;
    dest[9*i+5] = d5;
    dest[9*i+6] = d6;
    dest[9*i+7] = d7;
    dest[9*i+8] = d8;
  }
}
void bsmvm_10x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*10+0] * src[col_idx[j]+0];
      d1 += value[j*10+1] * src[col_idx[j]+0];
      d2 += value[j*10+2] * src[col_idx[j]+0];
      d3 += value[j*10+3] * src[col_idx[j]+0];
      d4 += value[j*10+4] * src[col_idx[j]+0];
      d5 += value[j*10+5] * src[col_idx[j]+0];
      d6 += value[j*10+6] * src[col_idx[j]+0];
      d7 += value[j*10+7] * src[col_idx[j]+0];
      d8 += value[j*10+8] * src[col_idx[j]+0];
      d9 += value[j*10+9] * src[col_idx[j]+0];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*20+0] * src[col_idx[j]+0];
      d1 += value[j*20+2] * src[col_idx[j]+0];
      d2 += value[j*20+4] * src[col_idx[j]+0];
      d3 += value[j*20+6] * src[col_idx[j]+0];
      d4 += value[j*20+8] * src[col_idx[j]+0];
      d5 += value[j*20+10] * src[col_idx[j]+0];
      d6 += value[j*20+12] * src[col_idx[j]+0];
      d7 += value[j*20+14] * src[col_idx[j]+0];
      d8 += value[j*20+16] * src[col_idx[j]+0];
      d9 += value[j*20+18] * src[col_idx[j]+0];
      d0 += value[j*20+1] * src[col_idx[j]+1];
      d1 += value[j*20+3] * src[col_idx[j]+1];
      d2 += value[j*20+5] * src[col_idx[j]+1];
      d3 += value[j*20+7] * src[col_idx[j]+1];
      d4 += value[j*20+9] * src[col_idx[j]+1];
      d5 += value[j*20+11] * src[col_idx[j]+1];
      d6 += value[j*20+13] * src[col_idx[j]+1];
      d7 += value[j*20+15] * src[col_idx[j]+1];
      d8 += value[j*20+17] * src[col_idx[j]+1];
      d9 += value[j*20+19] * src[col_idx[j]+1];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*30+0] * src[col_idx[j]+0];
      d1 += value[j*30+3] * src[col_idx[j]+0];
      d2 += value[j*30+6] * src[col_idx[j]+0];
      d3 += value[j*30+9] * src[col_idx[j]+0];
      d4 += value[j*30+12] * src[col_idx[j]+0];
      d5 += value[j*30+15] * src[col_idx[j]+0];
      d6 += value[j*30+18] * src[col_idx[j]+0];
      d7 += value[j*30+21] * src[col_idx[j]+0];
      d8 += value[j*30+24] * src[col_idx[j]+0];
      d9 += value[j*30+27] * src[col_idx[j]+0];
      d0 += value[j*30+1] * src[col_idx[j]+1];
      d1 += value[j*30+4] * src[col_idx[j]+1];
      d2 += value[j*30+7] * src[col_idx[j]+1];
      d3 += value[j*30+10] * src[col_idx[j]+1];
      d4 += value[j*30+13] * src[col_idx[j]+1];
      d5 += value[j*30+16] * src[col_idx[j]+1];
      d6 += value[j*30+19] * src[col_idx[j]+1];
      d7 += value[j*30+22] * src[col_idx[j]+1];
      d8 += value[j*30+25] * src[col_idx[j]+1];
      d9 += value[j*30+28] * src[col_idx[j]+1];
      d0 += value[j*30+2] * src[col_idx[j]+2];
      d1 += value[j*30+5] * src[col_idx[j]+2];
      d2 += value[j*30+8] * src[col_idx[j]+2];
      d3 += value[j*30+11] * src[col_idx[j]+2];
      d4 += value[j*30+14] * src[col_idx[j]+2];
      d5 += value[j*30+17] * src[col_idx[j]+2];
      d6 += value[j*30+20] * src[col_idx[j]+2];
      d7 += value[j*30+23] * src[col_idx[j]+2];
      d8 += value[j*30+26] * src[col_idx[j]+2];
      d9 += value[j*30+29] * src[col_idx[j]+2];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*40+0] * src[col_idx[j]+0];
      d1 += value[j*40+4] * src[col_idx[j]+0];
      d2 += value[j*40+8] * src[col_idx[j]+0];
      d3 += value[j*40+12] * src[col_idx[j]+0];
      d4 += value[j*40+16] * src[col_idx[j]+0];
      d5 += value[j*40+20] * src[col_idx[j]+0];
      d6 += value[j*40+24] * src[col_idx[j]+0];
      d7 += value[j*40+28] * src[col_idx[j]+0];
      d8 += value[j*40+32] * src[col_idx[j]+0];
      d9 += value[j*40+36] * src[col_idx[j]+0];
      d0 += value[j*40+1] * src[col_idx[j]+1];
      d1 += value[j*40+5] * src[col_idx[j]+1];
      d2 += value[j*40+9] * src[col_idx[j]+1];
      d3 += value[j*40+13] * src[col_idx[j]+1];
      d4 += value[j*40+17] * src[col_idx[j]+1];
      d5 += value[j*40+21] * src[col_idx[j]+1];
      d6 += value[j*40+25] * src[col_idx[j]+1];
      d7 += value[j*40+29] * src[col_idx[j]+1];
      d8 += value[j*40+33] * src[col_idx[j]+1];
      d9 += value[j*40+37] * src[col_idx[j]+1];
      d0 += value[j*40+2] * src[col_idx[j]+2];
      d1 += value[j*40+6] * src[col_idx[j]+2];
      d2 += value[j*40+10] * src[col_idx[j]+2];
      d3 += value[j*40+14] * src[col_idx[j]+2];
      d4 += value[j*40+18] * src[col_idx[j]+2];
      d5 += value[j*40+22] * src[col_idx[j]+2];
      d6 += value[j*40+26] * src[col_idx[j]+2];
      d7 += value[j*40+30] * src[col_idx[j]+2];
      d8 += value[j*40+34] * src[col_idx[j]+2];
      d9 += value[j*40+38] * src[col_idx[j]+2];
      d0 += value[j*40+3] * src[col_idx[j]+3];
      d1 += value[j*40+7] * src[col_idx[j]+3];
      d2 += value[j*40+11] * src[col_idx[j]+3];
      d3 += value[j*40+15] * src[col_idx[j]+3];
      d4 += value[j*40+19] * src[col_idx[j]+3];
      d5 += value[j*40+23] * src[col_idx[j]+3];
      d6 += value[j*40+27] * src[col_idx[j]+3];
      d7 += value[j*40+31] * src[col_idx[j]+3];
      d8 += value[j*40+35] * src[col_idx[j]+3];
      d9 += value[j*40+39] * src[col_idx[j]+3];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*50+0] * src[col_idx[j]+0];
      d1 += value[j*50+5] * src[col_idx[j]+0];
      d2 += value[j*50+10] * src[col_idx[j]+0];
      d3 += value[j*50+15] * src[col_idx[j]+0];
      d4 += value[j*50+20] * src[col_idx[j]+0];
      d5 += value[j*50+25] * src[col_idx[j]+0];
      d6 += value[j*50+30] * src[col_idx[j]+0];
      d7 += value[j*50+35] * src[col_idx[j]+0];
      d8 += value[j*50+40] * src[col_idx[j]+0];
      d9 += value[j*50+45] * src[col_idx[j]+0];
      d0 += value[j*50+1] * src[col_idx[j]+1];
      d1 += value[j*50+6] * src[col_idx[j]+1];
      d2 += value[j*50+11] * src[col_idx[j]+1];
      d3 += value[j*50+16] * src[col_idx[j]+1];
      d4 += value[j*50+21] * src[col_idx[j]+1];
      d5 += value[j*50+26] * src[col_idx[j]+1];
      d6 += value[j*50+31] * src[col_idx[j]+1];
      d7 += value[j*50+36] * src[col_idx[j]+1];
      d8 += value[j*50+41] * src[col_idx[j]+1];
      d9 += value[j*50+46] * src[col_idx[j]+1];
      d0 += value[j*50+2] * src[col_idx[j]+2];
      d1 += value[j*50+7] * src[col_idx[j]+2];
      d2 += value[j*50+12] * src[col_idx[j]+2];
      d3 += value[j*50+17] * src[col_idx[j]+2];
      d4 += value[j*50+22] * src[col_idx[j]+2];
      d5 += value[j*50+27] * src[col_idx[j]+2];
      d6 += value[j*50+32] * src[col_idx[j]+2];
      d7 += value[j*50+37] * src[col_idx[j]+2];
      d8 += value[j*50+42] * src[col_idx[j]+2];
      d9 += value[j*50+47] * src[col_idx[j]+2];
      d0 += value[j*50+3] * src[col_idx[j]+3];
      d1 += value[j*50+8] * src[col_idx[j]+3];
      d2 += value[j*50+13] * src[col_idx[j]+3];
      d3 += value[j*50+18] * src[col_idx[j]+3];
      d4 += value[j*50+23] * src[col_idx[j]+3];
      d5 += value[j*50+28] * src[col_idx[j]+3];
      d6 += value[j*50+33] * src[col_idx[j]+3];
      d7 += value[j*50+38] * src[col_idx[j]+3];
      d8 += value[j*50+43] * src[col_idx[j]+3];
      d9 += value[j*50+48] * src[col_idx[j]+3];
      d0 += value[j*50+4] * src[col_idx[j]+4];
      d1 += value[j*50+9] * src[col_idx[j]+4];
      d2 += value[j*50+14] * src[col_idx[j]+4];
      d3 += value[j*50+19] * src[col_idx[j]+4];
      d4 += value[j*50+24] * src[col_idx[j]+4];
      d5 += value[j*50+29] * src[col_idx[j]+4];
      d6 += value[j*50+34] * src[col_idx[j]+4];
      d7 += value[j*50+39] * src[col_idx[j]+4];
      d8 += value[j*50+44] * src[col_idx[j]+4];
      d9 += value[j*50+49] * src[col_idx[j]+4];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*60+0] * src[col_idx[j]+0];
      d1 += value[j*60+6] * src[col_idx[j]+0];
      d2 += value[j*60+12] * src[col_idx[j]+0];
      d3 += value[j*60+18] * src[col_idx[j]+0];
      d4 += value[j*60+24] * src[col_idx[j]+0];
      d5 += value[j*60+30] * src[col_idx[j]+0];
      d6 += value[j*60+36] * src[col_idx[j]+0];
      d7 += value[j*60+42] * src[col_idx[j]+0];
      d8 += value[j*60+48] * src[col_idx[j]+0];
      d9 += value[j*60+54] * src[col_idx[j]+0];
      d0 += value[j*60+1] * src[col_idx[j]+1];
      d1 += value[j*60+7] * src[col_idx[j]+1];
      d2 += value[j*60+13] * src[col_idx[j]+1];
      d3 += value[j*60+19] * src[col_idx[j]+1];
      d4 += value[j*60+25] * src[col_idx[j]+1];
      d5 += value[j*60+31] * src[col_idx[j]+1];
      d6 += value[j*60+37] * src[col_idx[j]+1];
      d7 += value[j*60+43] * src[col_idx[j]+1];
      d8 += value[j*60+49] * src[col_idx[j]+1];
      d9 += value[j*60+55] * src[col_idx[j]+1];
      d0 += value[j*60+2] * src[col_idx[j]+2];
      d1 += value[j*60+8] * src[col_idx[j]+2];
      d2 += value[j*60+14] * src[col_idx[j]+2];
      d3 += value[j*60+20] * src[col_idx[j]+2];
      d4 += value[j*60+26] * src[col_idx[j]+2];
      d5 += value[j*60+32] * src[col_idx[j]+2];
      d6 += value[j*60+38] * src[col_idx[j]+2];
      d7 += value[j*60+44] * src[col_idx[j]+2];
      d8 += value[j*60+50] * src[col_idx[j]+2];
      d9 += value[j*60+56] * src[col_idx[j]+2];
      d0 += value[j*60+3] * src[col_idx[j]+3];
      d1 += value[j*60+9] * src[col_idx[j]+3];
      d2 += value[j*60+15] * src[col_idx[j]+3];
      d3 += value[j*60+21] * src[col_idx[j]+3];
      d4 += value[j*60+27] * src[col_idx[j]+3];
      d5 += value[j*60+33] * src[col_idx[j]+3];
      d6 += value[j*60+39] * src[col_idx[j]+3];
      d7 += value[j*60+45] * src[col_idx[j]+3];
      d8 += value[j*60+51] * src[col_idx[j]+3];
      d9 += value[j*60+57] * src[col_idx[j]+3];
      d0 += value[j*60+4] * src[col_idx[j]+4];
      d1 += value[j*60+10] * src[col_idx[j]+4];
      d2 += value[j*60+16] * src[col_idx[j]+4];
      d3 += value[j*60+22] * src[col_idx[j]+4];
      d4 += value[j*60+28] * src[col_idx[j]+4];
      d5 += value[j*60+34] * src[col_idx[j]+4];
      d6 += value[j*60+40] * src[col_idx[j]+4];
      d7 += value[j*60+46] * src[col_idx[j]+4];
      d8 += value[j*60+52] * src[col_idx[j]+4];
      d9 += value[j*60+58] * src[col_idx[j]+4];
      d0 += value[j*60+5] * src[col_idx[j]+5];
      d1 += value[j*60+11] * src[col_idx[j]+5];
      d2 += value[j*60+17] * src[col_idx[j]+5];
      d3 += value[j*60+23] * src[col_idx[j]+5];
      d4 += value[j*60+29] * src[col_idx[j]+5];
      d5 += value[j*60+35] * src[col_idx[j]+5];
      d6 += value[j*60+41] * src[col_idx[j]+5];
      d7 += value[j*60+47] * src[col_idx[j]+5];
      d8 += value[j*60+53] * src[col_idx[j]+5];
      d9 += value[j*60+59] * src[col_idx[j]+5];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*70+0] * src[col_idx[j]+0];
      d1 += value[j*70+7] * src[col_idx[j]+0];
      d2 += value[j*70+14] * src[col_idx[j]+0];
      d3 += value[j*70+21] * src[col_idx[j]+0];
      d4 += value[j*70+28] * src[col_idx[j]+0];
      d5 += value[j*70+35] * src[col_idx[j]+0];
      d6 += value[j*70+42] * src[col_idx[j]+0];
      d7 += value[j*70+49] * src[col_idx[j]+0];
      d8 += value[j*70+56] * src[col_idx[j]+0];
      d9 += value[j*70+63] * src[col_idx[j]+0];
      d0 += value[j*70+1] * src[col_idx[j]+1];
      d1 += value[j*70+8] * src[col_idx[j]+1];
      d2 += value[j*70+15] * src[col_idx[j]+1];
      d3 += value[j*70+22] * src[col_idx[j]+1];
      d4 += value[j*70+29] * src[col_idx[j]+1];
      d5 += value[j*70+36] * src[col_idx[j]+1];
      d6 += value[j*70+43] * src[col_idx[j]+1];
      d7 += value[j*70+50] * src[col_idx[j]+1];
      d8 += value[j*70+57] * src[col_idx[j]+1];
      d9 += value[j*70+64] * src[col_idx[j]+1];
      d0 += value[j*70+2] * src[col_idx[j]+2];
      d1 += value[j*70+9] * src[col_idx[j]+2];
      d2 += value[j*70+16] * src[col_idx[j]+2];
      d3 += value[j*70+23] * src[col_idx[j]+2];
      d4 += value[j*70+30] * src[col_idx[j]+2];
      d5 += value[j*70+37] * src[col_idx[j]+2];
      d6 += value[j*70+44] * src[col_idx[j]+2];
      d7 += value[j*70+51] * src[col_idx[j]+2];
      d8 += value[j*70+58] * src[col_idx[j]+2];
      d9 += value[j*70+65] * src[col_idx[j]+2];
      d0 += value[j*70+3] * src[col_idx[j]+3];
      d1 += value[j*70+10] * src[col_idx[j]+3];
      d2 += value[j*70+17] * src[col_idx[j]+3];
      d3 += value[j*70+24] * src[col_idx[j]+3];
      d4 += value[j*70+31] * src[col_idx[j]+3];
      d5 += value[j*70+38] * src[col_idx[j]+3];
      d6 += value[j*70+45] * src[col_idx[j]+3];
      d7 += value[j*70+52] * src[col_idx[j]+3];
      d8 += value[j*70+59] * src[col_idx[j]+3];
      d9 += value[j*70+66] * src[col_idx[j]+3];
      d0 += value[j*70+4] * src[col_idx[j]+4];
      d1 += value[j*70+11] * src[col_idx[j]+4];
      d2 += value[j*70+18] * src[col_idx[j]+4];
      d3 += value[j*70+25] * src[col_idx[j]+4];
      d4 += value[j*70+32] * src[col_idx[j]+4];
      d5 += value[j*70+39] * src[col_idx[j]+4];
      d6 += value[j*70+46] * src[col_idx[j]+4];
      d7 += value[j*70+53] * src[col_idx[j]+4];
      d8 += value[j*70+60] * src[col_idx[j]+4];
      d9 += value[j*70+67] * src[col_idx[j]+4];
      d0 += value[j*70+5] * src[col_idx[j]+5];
      d1 += value[j*70+12] * src[col_idx[j]+5];
      d2 += value[j*70+19] * src[col_idx[j]+5];
      d3 += value[j*70+26] * src[col_idx[j]+5];
      d4 += value[j*70+33] * src[col_idx[j]+5];
      d5 += value[j*70+40] * src[col_idx[j]+5];
      d6 += value[j*70+47] * src[col_idx[j]+5];
      d7 += value[j*70+54] * src[col_idx[j]+5];
      d8 += value[j*70+61] * src[col_idx[j]+5];
      d9 += value[j*70+68] * src[col_idx[j]+5];
      d0 += value[j*70+6] * src[col_idx[j]+6];
      d1 += value[j*70+13] * src[col_idx[j]+6];
      d2 += value[j*70+20] * src[col_idx[j]+6];
      d3 += value[j*70+27] * src[col_idx[j]+6];
      d4 += value[j*70+34] * src[col_idx[j]+6];
      d5 += value[j*70+41] * src[col_idx[j]+6];
      d6 += value[j*70+48] * src[col_idx[j]+6];
      d7 += value[j*70+55] * src[col_idx[j]+6];
      d8 += value[j*70+62] * src[col_idx[j]+6];
      d9 += value[j*70+69] * src[col_idx[j]+6];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*80+0] * src[col_idx[j]+0];
      d1 += value[j*80+8] * src[col_idx[j]+0];
      d2 += value[j*80+16] * src[col_idx[j]+0];
      d3 += value[j*80+24] * src[col_idx[j]+0];
      d4 += value[j*80+32] * src[col_idx[j]+0];
      d5 += value[j*80+40] * src[col_idx[j]+0];
      d6 += value[j*80+48] * src[col_idx[j]+0];
      d7 += value[j*80+56] * src[col_idx[j]+0];
      d8 += value[j*80+64] * src[col_idx[j]+0];
      d9 += value[j*80+72] * src[col_idx[j]+0];
      d0 += value[j*80+1] * src[col_idx[j]+1];
      d1 += value[j*80+9] * src[col_idx[j]+1];
      d2 += value[j*80+17] * src[col_idx[j]+1];
      d3 += value[j*80+25] * src[col_idx[j]+1];
      d4 += value[j*80+33] * src[col_idx[j]+1];
      d5 += value[j*80+41] * src[col_idx[j]+1];
      d6 += value[j*80+49] * src[col_idx[j]+1];
      d7 += value[j*80+57] * src[col_idx[j]+1];
      d8 += value[j*80+65] * src[col_idx[j]+1];
      d9 += value[j*80+73] * src[col_idx[j]+1];
      d0 += value[j*80+2] * src[col_idx[j]+2];
      d1 += value[j*80+10] * src[col_idx[j]+2];
      d2 += value[j*80+18] * src[col_idx[j]+2];
      d3 += value[j*80+26] * src[col_idx[j]+2];
      d4 += value[j*80+34] * src[col_idx[j]+2];
      d5 += value[j*80+42] * src[col_idx[j]+2];
      d6 += value[j*80+50] * src[col_idx[j]+2];
      d7 += value[j*80+58] * src[col_idx[j]+2];
      d8 += value[j*80+66] * src[col_idx[j]+2];
      d9 += value[j*80+74] * src[col_idx[j]+2];
      d0 += value[j*80+3] * src[col_idx[j]+3];
      d1 += value[j*80+11] * src[col_idx[j]+3];
      d2 += value[j*80+19] * src[col_idx[j]+3];
      d3 += value[j*80+27] * src[col_idx[j]+3];
      d4 += value[j*80+35] * src[col_idx[j]+3];
      d5 += value[j*80+43] * src[col_idx[j]+3];
      d6 += value[j*80+51] * src[col_idx[j]+3];
      d7 += value[j*80+59] * src[col_idx[j]+3];
      d8 += value[j*80+67] * src[col_idx[j]+3];
      d9 += value[j*80+75] * src[col_idx[j]+3];
      d0 += value[j*80+4] * src[col_idx[j]+4];
      d1 += value[j*80+12] * src[col_idx[j]+4];
      d2 += value[j*80+20] * src[col_idx[j]+4];
      d3 += value[j*80+28] * src[col_idx[j]+4];
      d4 += value[j*80+36] * src[col_idx[j]+4];
      d5 += value[j*80+44] * src[col_idx[j]+4];
      d6 += value[j*80+52] * src[col_idx[j]+4];
      d7 += value[j*80+60] * src[col_idx[j]+4];
      d8 += value[j*80+68] * src[col_idx[j]+4];
      d9 += value[j*80+76] * src[col_idx[j]+4];
      d0 += value[j*80+5] * src[col_idx[j]+5];
      d1 += value[j*80+13] * src[col_idx[j]+5];
      d2 += value[j*80+21] * src[col_idx[j]+5];
      d3 += value[j*80+29] * src[col_idx[j]+5];
      d4 += value[j*80+37] * src[col_idx[j]+5];
      d5 += value[j*80+45] * src[col_idx[j]+5];
      d6 += value[j*80+53] * src[col_idx[j]+5];
      d7 += value[j*80+61] * src[col_idx[j]+5];
      d8 += value[j*80+69] * src[col_idx[j]+5];
      d9 += value[j*80+77] * src[col_idx[j]+5];
      d0 += value[j*80+6] * src[col_idx[j]+6];
      d1 += value[j*80+14] * src[col_idx[j]+6];
      d2 += value[j*80+22] * src[col_idx[j]+6];
      d3 += value[j*80+30] * src[col_idx[j]+6];
      d4 += value[j*80+38] * src[col_idx[j]+6];
      d5 += value[j*80+46] * src[col_idx[j]+6];
      d6 += value[j*80+54] * src[col_idx[j]+6];
      d7 += value[j*80+62] * src[col_idx[j]+6];
      d8 += value[j*80+70] * src[col_idx[j]+6];
      d9 += value[j*80+78] * src[col_idx[j]+6];
      d0 += value[j*80+7] * src[col_idx[j]+7];
      d1 += value[j*80+15] * src[col_idx[j]+7];
      d2 += value[j*80+23] * src[col_idx[j]+7];
      d3 += value[j*80+31] * src[col_idx[j]+7];
      d4 += value[j*80+39] * src[col_idx[j]+7];
      d5 += value[j*80+47] * src[col_idx[j]+7];
      d6 += value[j*80+55] * src[col_idx[j]+7];
      d7 += value[j*80+63] * src[col_idx[j]+7];
      d8 += value[j*80+71] * src[col_idx[j]+7];
      d9 += value[j*80+79] * src[col_idx[j]+7];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*90+0] * src[col_idx[j]+0];
      d1 += value[j*90+9] * src[col_idx[j]+0];
      d2 += value[j*90+18] * src[col_idx[j]+0];
      d3 += value[j*90+27] * src[col_idx[j]+0];
      d4 += value[j*90+36] * src[col_idx[j]+0];
      d5 += value[j*90+45] * src[col_idx[j]+0];
      d6 += value[j*90+54] * src[col_idx[j]+0];
      d7 += value[j*90+63] * src[col_idx[j]+0];
      d8 += value[j*90+72] * src[col_idx[j]+0];
      d9 += value[j*90+81] * src[col_idx[j]+0];
      d0 += value[j*90+1] * src[col_idx[j]+1];
      d1 += value[j*90+10] * src[col_idx[j]+1];
      d2 += value[j*90+19] * src[col_idx[j]+1];
      d3 += value[j*90+28] * src[col_idx[j]+1];
      d4 += value[j*90+37] * src[col_idx[j]+1];
      d5 += value[j*90+46] * src[col_idx[j]+1];
      d6 += value[j*90+55] * src[col_idx[j]+1];
      d7 += value[j*90+64] * src[col_idx[j]+1];
      d8 += value[j*90+73] * src[col_idx[j]+1];
      d9 += value[j*90+82] * src[col_idx[j]+1];
      d0 += value[j*90+2] * src[col_idx[j]+2];
      d1 += value[j*90+11] * src[col_idx[j]+2];
      d2 += value[j*90+20] * src[col_idx[j]+2];
      d3 += value[j*90+29] * src[col_idx[j]+2];
      d4 += value[j*90+38] * src[col_idx[j]+2];
      d5 += value[j*90+47] * src[col_idx[j]+2];
      d6 += value[j*90+56] * src[col_idx[j]+2];
      d7 += value[j*90+65] * src[col_idx[j]+2];
      d8 += value[j*90+74] * src[col_idx[j]+2];
      d9 += value[j*90+83] * src[col_idx[j]+2];
      d0 += value[j*90+3] * src[col_idx[j]+3];
      d1 += value[j*90+12] * src[col_idx[j]+3];
      d2 += value[j*90+21] * src[col_idx[j]+3];
      d3 += value[j*90+30] * src[col_idx[j]+3];
      d4 += value[j*90+39] * src[col_idx[j]+3];
      d5 += value[j*90+48] * src[col_idx[j]+3];
      d6 += value[j*90+57] * src[col_idx[j]+3];
      d7 += value[j*90+66] * src[col_idx[j]+3];
      d8 += value[j*90+75] * src[col_idx[j]+3];
      d9 += value[j*90+84] * src[col_idx[j]+3];
      d0 += value[j*90+4] * src[col_idx[j]+4];
      d1 += value[j*90+13] * src[col_idx[j]+4];
      d2 += value[j*90+22] * src[col_idx[j]+4];
      d3 += value[j*90+31] * src[col_idx[j]+4];
      d4 += value[j*90+40] * src[col_idx[j]+4];
      d5 += value[j*90+49] * src[col_idx[j]+4];
      d6 += value[j*90+58] * src[col_idx[j]+4];
      d7 += value[j*90+67] * src[col_idx[j]+4];
      d8 += value[j*90+76] * src[col_idx[j]+4];
      d9 += value[j*90+85] * src[col_idx[j]+4];
      d0 += value[j*90+5] * src[col_idx[j]+5];
      d1 += value[j*90+14] * src[col_idx[j]+5];
      d2 += value[j*90+23] * src[col_idx[j]+5];
      d3 += value[j*90+32] * src[col_idx[j]+5];
      d4 += value[j*90+41] * src[col_idx[j]+5];
      d5 += value[j*90+50] * src[col_idx[j]+5];
      d6 += value[j*90+59] * src[col_idx[j]+5];
      d7 += value[j*90+68] * src[col_idx[j]+5];
      d8 += value[j*90+77] * src[col_idx[j]+5];
      d9 += value[j*90+86] * src[col_idx[j]+5];
      d0 += value[j*90+6] * src[col_idx[j]+6];
      d1 += value[j*90+15] * src[col_idx[j]+6];
      d2 += value[j*90+24] * src[col_idx[j]+6];
      d3 += value[j*90+33] * src[col_idx[j]+6];
      d4 += value[j*90+42] * src[col_idx[j]+6];
      d5 += value[j*90+51] * src[col_idx[j]+6];
      d6 += value[j*90+60] * src[col_idx[j]+6];
      d7 += value[j*90+69] * src[col_idx[j]+6];
      d8 += value[j*90+78] * src[col_idx[j]+6];
      d9 += value[j*90+87] * src[col_idx[j]+6];
      d0 += value[j*90+7] * src[col_idx[j]+7];
      d1 += value[j*90+16] * src[col_idx[j]+7];
      d2 += value[j*90+25] * src[col_idx[j]+7];
      d3 += value[j*90+34] * src[col_idx[j]+7];
      d4 += value[j*90+43] * src[col_idx[j]+7];
      d5 += value[j*90+52] * src[col_idx[j]+7];
      d6 += value[j*90+61] * src[col_idx[j]+7];
      d7 += value[j*90+70] * src[col_idx[j]+7];
      d8 += value[j*90+79] * src[col_idx[j]+7];
      d9 += value[j*90+88] * src[col_idx[j]+7];
      d0 += value[j*90+8] * src[col_idx[j]+8];
      d1 += value[j*90+17] * src[col_idx[j]+8];
      d2 += value[j*90+26] * src[col_idx[j]+8];
      d3 += value[j*90+35] * src[col_idx[j]+8];
      d4 += value[j*90+44] * src[col_idx[j]+8];
      d5 += value[j*90+53] * src[col_idx[j]+8];
      d6 += value[j*90+62] * src[col_idx[j]+8];
      d7 += value[j*90+71] * src[col_idx[j]+8];
      d8 += value[j*90+80] * src[col_idx[j]+8];
      d9 += value[j*90+89] * src[col_idx[j]+8];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*100+0] * src[col_idx[j]+0];
      d1 += value[j*100+10] * src[col_idx[j]+0];
      d2 += value[j*100+20] * src[col_idx[j]+0];
      d3 += value[j*100+30] * src[col_idx[j]+0];
      d4 += value[j*100+40] * src[col_idx[j]+0];
      d5 += value[j*100+50] * src[col_idx[j]+0];
      d6 += value[j*100+60] * src[col_idx[j]+0];
      d7 += value[j*100+70] * src[col_idx[j]+0];
      d8 += value[j*100+80] * src[col_idx[j]+0];
      d9 += value[j*100+90] * src[col_idx[j]+0];
      d0 += value[j*100+1] * src[col_idx[j]+1];
      d1 += value[j*100+11] * src[col_idx[j]+1];
      d2 += value[j*100+21] * src[col_idx[j]+1];
      d3 += value[j*100+31] * src[col_idx[j]+1];
      d4 += value[j*100+41] * src[col_idx[j]+1];
      d5 += value[j*100+51] * src[col_idx[j]+1];
      d6 += value[j*100+61] * src[col_idx[j]+1];
      d7 += value[j*100+71] * src[col_idx[j]+1];
      d8 += value[j*100+81] * src[col_idx[j]+1];
      d9 += value[j*100+91] * src[col_idx[j]+1];
      d0 += value[j*100+2] * src[col_idx[j]+2];
      d1 += value[j*100+12] * src[col_idx[j]+2];
      d2 += value[j*100+22] * src[col_idx[j]+2];
      d3 += value[j*100+32] * src[col_idx[j]+2];
      d4 += value[j*100+42] * src[col_idx[j]+2];
      d5 += value[j*100+52] * src[col_idx[j]+2];
      d6 += value[j*100+62] * src[col_idx[j]+2];
      d7 += value[j*100+72] * src[col_idx[j]+2];
      d8 += value[j*100+82] * src[col_idx[j]+2];
      d9 += value[j*100+92] * src[col_idx[j]+2];
      d0 += value[j*100+3] * src[col_idx[j]+3];
      d1 += value[j*100+13] * src[col_idx[j]+3];
      d2 += value[j*100+23] * src[col_idx[j]+3];
      d3 += value[j*100+33] * src[col_idx[j]+3];
      d4 += value[j*100+43] * src[col_idx[j]+3];
      d5 += value[j*100+53] * src[col_idx[j]+3];
      d6 += value[j*100+63] * src[col_idx[j]+3];
      d7 += value[j*100+73] * src[col_idx[j]+3];
      d8 += value[j*100+83] * src[col_idx[j]+3];
      d9 += value[j*100+93] * src[col_idx[j]+3];
      d0 += value[j*100+4] * src[col_idx[j]+4];
      d1 += value[j*100+14] * src[col_idx[j]+4];
      d2 += value[j*100+24] * src[col_idx[j]+4];
      d3 += value[j*100+34] * src[col_idx[j]+4];
      d4 += value[j*100+44] * src[col_idx[j]+4];
      d5 += value[j*100+54] * src[col_idx[j]+4];
      d6 += value[j*100+64] * src[col_idx[j]+4];
      d7 += value[j*100+74] * src[col_idx[j]+4];
      d8 += value[j*100+84] * src[col_idx[j]+4];
      d9 += value[j*100+94] * src[col_idx[j]+4];
      d0 += value[j*100+5] * src[col_idx[j]+5];
      d1 += value[j*100+15] * src[col_idx[j]+5];
      d2 += value[j*100+25] * src[col_idx[j]+5];
      d3 += value[j*100+35] * src[col_idx[j]+5];
      d4 += value[j*100+45] * src[col_idx[j]+5];
      d5 += value[j*100+55] * src[col_idx[j]+5];
      d6 += value[j*100+65] * src[col_idx[j]+5];
      d7 += value[j*100+75] * src[col_idx[j]+5];
      d8 += value[j*100+85] * src[col_idx[j]+5];
      d9 += value[j*100+95] * src[col_idx[j]+5];
      d0 += value[j*100+6] * src[col_idx[j]+6];
      d1 += value[j*100+16] * src[col_idx[j]+6];
      d2 += value[j*100+26] * src[col_idx[j]+6];
      d3 += value[j*100+36] * src[col_idx[j]+6];
      d4 += value[j*100+46] * src[col_idx[j]+6];
      d5 += value[j*100+56] * src[col_idx[j]+6];
      d6 += value[j*100+66] * src[col_idx[j]+6];
      d7 += value[j*100+76] * src[col_idx[j]+6];
      d8 += value[j*100+86] * src[col_idx[j]+6];
      d9 += value[j*100+96] * src[col_idx[j]+6];
      d0 += value[j*100+7] * src[col_idx[j]+7];
      d1 += value[j*100+17] * src[col_idx[j]+7];
      d2 += value[j*100+27] * src[col_idx[j]+7];
      d3 += value[j*100+37] * src[col_idx[j]+7];
      d4 += value[j*100+47] * src[col_idx[j]+7];
      d5 += value[j*100+57] * src[col_idx[j]+7];
      d6 += value[j*100+67] * src[col_idx[j]+7];
      d7 += value[j*100+77] * src[col_idx[j]+7];
      d8 += value[j*100+87] * src[col_idx[j]+7];
      d9 += value[j*100+97] * src[col_idx[j]+7];
      d0 += value[j*100+8] * src[col_idx[j]+8];
      d1 += value[j*100+18] * src[col_idx[j]+8];
      d2 += value[j*100+28] * src[col_idx[j]+8];
      d3 += value[j*100+38] * src[col_idx[j]+8];
      d4 += value[j*100+48] * src[col_idx[j]+8];
      d5 += value[j*100+58] * src[col_idx[j]+8];
      d6 += value[j*100+68] * src[col_idx[j]+8];
      d7 += value[j*100+78] * src[col_idx[j]+8];
      d8 += value[j*100+88] * src[col_idx[j]+8];
      d9 += value[j*100+98] * src[col_idx[j]+8];
      d0 += value[j*100+9] * src[col_idx[j]+9];
      d1 += value[j*100+19] * src[col_idx[j]+9];
      d2 += value[j*100+29] * src[col_idx[j]+9];
      d3 += value[j*100+39] * src[col_idx[j]+9];
      d4 += value[j*100+49] * src[col_idx[j]+9];
      d5 += value[j*100+59] * src[col_idx[j]+9];
      d6 += value[j*100+69] * src[col_idx[j]+9];
      d7 += value[j*100+79] * src[col_idx[j]+9];
      d8 += value[j*100+89] * src[col_idx[j]+9];
      d9 += value[j*100+99] * src[col_idx[j]+9];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*110+0] * src[col_idx[j]+0];
      d1 += value[j*110+11] * src[col_idx[j]+0];
      d2 += value[j*110+22] * src[col_idx[j]+0];
      d3 += value[j*110+33] * src[col_idx[j]+0];
      d4 += value[j*110+44] * src[col_idx[j]+0];
      d5 += value[j*110+55] * src[col_idx[j]+0];
      d6 += value[j*110+66] * src[col_idx[j]+0];
      d7 += value[j*110+77] * src[col_idx[j]+0];
      d8 += value[j*110+88] * src[col_idx[j]+0];
      d9 += value[j*110+99] * src[col_idx[j]+0];
      d0 += value[j*110+1] * src[col_idx[j]+1];
      d1 += value[j*110+12] * src[col_idx[j]+1];
      d2 += value[j*110+23] * src[col_idx[j]+1];
      d3 += value[j*110+34] * src[col_idx[j]+1];
      d4 += value[j*110+45] * src[col_idx[j]+1];
      d5 += value[j*110+56] * src[col_idx[j]+1];
      d6 += value[j*110+67] * src[col_idx[j]+1];
      d7 += value[j*110+78] * src[col_idx[j]+1];
      d8 += value[j*110+89] * src[col_idx[j]+1];
      d9 += value[j*110+100] * src[col_idx[j]+1];
      d0 += value[j*110+2] * src[col_idx[j]+2];
      d1 += value[j*110+13] * src[col_idx[j]+2];
      d2 += value[j*110+24] * src[col_idx[j]+2];
      d3 += value[j*110+35] * src[col_idx[j]+2];
      d4 += value[j*110+46] * src[col_idx[j]+2];
      d5 += value[j*110+57] * src[col_idx[j]+2];
      d6 += value[j*110+68] * src[col_idx[j]+2];
      d7 += value[j*110+79] * src[col_idx[j]+2];
      d8 += value[j*110+90] * src[col_idx[j]+2];
      d9 += value[j*110+101] * src[col_idx[j]+2];
      d0 += value[j*110+3] * src[col_idx[j]+3];
      d1 += value[j*110+14] * src[col_idx[j]+3];
      d2 += value[j*110+25] * src[col_idx[j]+3];
      d3 += value[j*110+36] * src[col_idx[j]+3];
      d4 += value[j*110+47] * src[col_idx[j]+3];
      d5 += value[j*110+58] * src[col_idx[j]+3];
      d6 += value[j*110+69] * src[col_idx[j]+3];
      d7 += value[j*110+80] * src[col_idx[j]+3];
      d8 += value[j*110+91] * src[col_idx[j]+3];
      d9 += value[j*110+102] * src[col_idx[j]+3];
      d0 += value[j*110+4] * src[col_idx[j]+4];
      d1 += value[j*110+15] * src[col_idx[j]+4];
      d2 += value[j*110+26] * src[col_idx[j]+4];
      d3 += value[j*110+37] * src[col_idx[j]+4];
      d4 += value[j*110+48] * src[col_idx[j]+4];
      d5 += value[j*110+59] * src[col_idx[j]+4];
      d6 += value[j*110+70] * src[col_idx[j]+4];
      d7 += value[j*110+81] * src[col_idx[j]+4];
      d8 += value[j*110+92] * src[col_idx[j]+4];
      d9 += value[j*110+103] * src[col_idx[j]+4];
      d0 += value[j*110+5] * src[col_idx[j]+5];
      d1 += value[j*110+16] * src[col_idx[j]+5];
      d2 += value[j*110+27] * src[col_idx[j]+5];
      d3 += value[j*110+38] * src[col_idx[j]+5];
      d4 += value[j*110+49] * src[col_idx[j]+5];
      d5 += value[j*110+60] * src[col_idx[j]+5];
      d6 += value[j*110+71] * src[col_idx[j]+5];
      d7 += value[j*110+82] * src[col_idx[j]+5];
      d8 += value[j*110+93] * src[col_idx[j]+5];
      d9 += value[j*110+104] * src[col_idx[j]+5];
      d0 += value[j*110+6] * src[col_idx[j]+6];
      d1 += value[j*110+17] * src[col_idx[j]+6];
      d2 += value[j*110+28] * src[col_idx[j]+6];
      d3 += value[j*110+39] * src[col_idx[j]+6];
      d4 += value[j*110+50] * src[col_idx[j]+6];
      d5 += value[j*110+61] * src[col_idx[j]+6];
      d6 += value[j*110+72] * src[col_idx[j]+6];
      d7 += value[j*110+83] * src[col_idx[j]+6];
      d8 += value[j*110+94] * src[col_idx[j]+6];
      d9 += value[j*110+105] * src[col_idx[j]+6];
      d0 += value[j*110+7] * src[col_idx[j]+7];
      d1 += value[j*110+18] * src[col_idx[j]+7];
      d2 += value[j*110+29] * src[col_idx[j]+7];
      d3 += value[j*110+40] * src[col_idx[j]+7];
      d4 += value[j*110+51] * src[col_idx[j]+7];
      d5 += value[j*110+62] * src[col_idx[j]+7];
      d6 += value[j*110+73] * src[col_idx[j]+7];
      d7 += value[j*110+84] * src[col_idx[j]+7];
      d8 += value[j*110+95] * src[col_idx[j]+7];
      d9 += value[j*110+106] * src[col_idx[j]+7];
      d0 += value[j*110+8] * src[col_idx[j]+8];
      d1 += value[j*110+19] * src[col_idx[j]+8];
      d2 += value[j*110+30] * src[col_idx[j]+8];
      d3 += value[j*110+41] * src[col_idx[j]+8];
      d4 += value[j*110+52] * src[col_idx[j]+8];
      d5 += value[j*110+63] * src[col_idx[j]+8];
      d6 += value[j*110+74] * src[col_idx[j]+8];
      d7 += value[j*110+85] * src[col_idx[j]+8];
      d8 += value[j*110+96] * src[col_idx[j]+8];
      d9 += value[j*110+107] * src[col_idx[j]+8];
      d0 += value[j*110+9] * src[col_idx[j]+9];
      d1 += value[j*110+20] * src[col_idx[j]+9];
      d2 += value[j*110+31] * src[col_idx[j]+9];
      d3 += value[j*110+42] * src[col_idx[j]+9];
      d4 += value[j*110+53] * src[col_idx[j]+9];
      d5 += value[j*110+64] * src[col_idx[j]+9];
      d6 += value[j*110+75] * src[col_idx[j]+9];
      d7 += value[j*110+86] * src[col_idx[j]+9];
      d8 += value[j*110+97] * src[col_idx[j]+9];
      d9 += value[j*110+108] * src[col_idx[j]+9];
      d0 += value[j*110+10] * src[col_idx[j]+10];
      d1 += value[j*110+21] * src[col_idx[j]+10];
      d2 += value[j*110+32] * src[col_idx[j]+10];
      d3 += value[j*110+43] * src[col_idx[j]+10];
      d4 += value[j*110+54] * src[col_idx[j]+10];
      d5 += value[j*110+65] * src[col_idx[j]+10];
      d6 += value[j*110+76] * src[col_idx[j]+10];
      d7 += value[j*110+87] * src[col_idx[j]+10];
      d8 += value[j*110+98] * src[col_idx[j]+10];
      d9 += value[j*110+109] * src[col_idx[j]+10];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_10x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    d0 = dest[10*i+0];
    d1 = dest[10*i+1];
    d2 = dest[10*i+2];
    d3 = dest[10*i+3];
    d4 = dest[10*i+4];
    d5 = dest[10*i+5];
    d6 = dest[10*i+6];
    d7 = dest[10*i+7];
    d8 = dest[10*i+8];
    d9 = dest[10*i+9];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*120+0] * src[col_idx[j]+0];
      d1 += value[j*120+12] * src[col_idx[j]+0];
      d2 += value[j*120+24] * src[col_idx[j]+0];
      d3 += value[j*120+36] * src[col_idx[j]+0];
      d4 += value[j*120+48] * src[col_idx[j]+0];
      d5 += value[j*120+60] * src[col_idx[j]+0];
      d6 += value[j*120+72] * src[col_idx[j]+0];
      d7 += value[j*120+84] * src[col_idx[j]+0];
      d8 += value[j*120+96] * src[col_idx[j]+0];
      d9 += value[j*120+108] * src[col_idx[j]+0];
      d0 += value[j*120+1] * src[col_idx[j]+1];
      d1 += value[j*120+13] * src[col_idx[j]+1];
      d2 += value[j*120+25] * src[col_idx[j]+1];
      d3 += value[j*120+37] * src[col_idx[j]+1];
      d4 += value[j*120+49] * src[col_idx[j]+1];
      d5 += value[j*120+61] * src[col_idx[j]+1];
      d6 += value[j*120+73] * src[col_idx[j]+1];
      d7 += value[j*120+85] * src[col_idx[j]+1];
      d8 += value[j*120+97] * src[col_idx[j]+1];
      d9 += value[j*120+109] * src[col_idx[j]+1];
      d0 += value[j*120+2] * src[col_idx[j]+2];
      d1 += value[j*120+14] * src[col_idx[j]+2];
      d2 += value[j*120+26] * src[col_idx[j]+2];
      d3 += value[j*120+38] * src[col_idx[j]+2];
      d4 += value[j*120+50] * src[col_idx[j]+2];
      d5 += value[j*120+62] * src[col_idx[j]+2];
      d6 += value[j*120+74] * src[col_idx[j]+2];
      d7 += value[j*120+86] * src[col_idx[j]+2];
      d8 += value[j*120+98] * src[col_idx[j]+2];
      d9 += value[j*120+110] * src[col_idx[j]+2];
      d0 += value[j*120+3] * src[col_idx[j]+3];
      d1 += value[j*120+15] * src[col_idx[j]+3];
      d2 += value[j*120+27] * src[col_idx[j]+3];
      d3 += value[j*120+39] * src[col_idx[j]+3];
      d4 += value[j*120+51] * src[col_idx[j]+3];
      d5 += value[j*120+63] * src[col_idx[j]+3];
      d6 += value[j*120+75] * src[col_idx[j]+3];
      d7 += value[j*120+87] * src[col_idx[j]+3];
      d8 += value[j*120+99] * src[col_idx[j]+3];
      d9 += value[j*120+111] * src[col_idx[j]+3];
      d0 += value[j*120+4] * src[col_idx[j]+4];
      d1 += value[j*120+16] * src[col_idx[j]+4];
      d2 += value[j*120+28] * src[col_idx[j]+4];
      d3 += value[j*120+40] * src[col_idx[j]+4];
      d4 += value[j*120+52] * src[col_idx[j]+4];
      d5 += value[j*120+64] * src[col_idx[j]+4];
      d6 += value[j*120+76] * src[col_idx[j]+4];
      d7 += value[j*120+88] * src[col_idx[j]+4];
      d8 += value[j*120+100] * src[col_idx[j]+4];
      d9 += value[j*120+112] * src[col_idx[j]+4];
      d0 += value[j*120+5] * src[col_idx[j]+5];
      d1 += value[j*120+17] * src[col_idx[j]+5];
      d2 += value[j*120+29] * src[col_idx[j]+5];
      d3 += value[j*120+41] * src[col_idx[j]+5];
      d4 += value[j*120+53] * src[col_idx[j]+5];
      d5 += value[j*120+65] * src[col_idx[j]+5];
      d6 += value[j*120+77] * src[col_idx[j]+5];
      d7 += value[j*120+89] * src[col_idx[j]+5];
      d8 += value[j*120+101] * src[col_idx[j]+5];
      d9 += value[j*120+113] * src[col_idx[j]+5];
      d0 += value[j*120+6] * src[col_idx[j]+6];
      d1 += value[j*120+18] * src[col_idx[j]+6];
      d2 += value[j*120+30] * src[col_idx[j]+6];
      d3 += value[j*120+42] * src[col_idx[j]+6];
      d4 += value[j*120+54] * src[col_idx[j]+6];
      d5 += value[j*120+66] * src[col_idx[j]+6];
      d6 += value[j*120+78] * src[col_idx[j]+6];
      d7 += value[j*120+90] * src[col_idx[j]+6];
      d8 += value[j*120+102] * src[col_idx[j]+6];
      d9 += value[j*120+114] * src[col_idx[j]+6];
      d0 += value[j*120+7] * src[col_idx[j]+7];
      d1 += value[j*120+19] * src[col_idx[j]+7];
      d2 += value[j*120+31] * src[col_idx[j]+7];
      d3 += value[j*120+43] * src[col_idx[j]+7];
      d4 += value[j*120+55] * src[col_idx[j]+7];
      d5 += value[j*120+67] * src[col_idx[j]+7];
      d6 += value[j*120+79] * src[col_idx[j]+7];
      d7 += value[j*120+91] * src[col_idx[j]+7];
      d8 += value[j*120+103] * src[col_idx[j]+7];
      d9 += value[j*120+115] * src[col_idx[j]+7];
      d0 += value[j*120+8] * src[col_idx[j]+8];
      d1 += value[j*120+20] * src[col_idx[j]+8];
      d2 += value[j*120+32] * src[col_idx[j]+8];
      d3 += value[j*120+44] * src[col_idx[j]+8];
      d4 += value[j*120+56] * src[col_idx[j]+8];
      d5 += value[j*120+68] * src[col_idx[j]+8];
      d6 += value[j*120+80] * src[col_idx[j]+8];
      d7 += value[j*120+92] * src[col_idx[j]+8];
      d8 += value[j*120+104] * src[col_idx[j]+8];
      d9 += value[j*120+116] * src[col_idx[j]+8];
      d0 += value[j*120+9] * src[col_idx[j]+9];
      d1 += value[j*120+21] * src[col_idx[j]+9];
      d2 += value[j*120+33] * src[col_idx[j]+9];
      d3 += value[j*120+45] * src[col_idx[j]+9];
      d4 += value[j*120+57] * src[col_idx[j]+9];
      d5 += value[j*120+69] * src[col_idx[j]+9];
      d6 += value[j*120+81] * src[col_idx[j]+9];
      d7 += value[j*120+93] * src[col_idx[j]+9];
      d8 += value[j*120+105] * src[col_idx[j]+9];
      d9 += value[j*120+117] * src[col_idx[j]+9];
      d0 += value[j*120+10] * src[col_idx[j]+10];
      d1 += value[j*120+22] * src[col_idx[j]+10];
      d2 += value[j*120+34] * src[col_idx[j]+10];
      d3 += value[j*120+46] * src[col_idx[j]+10];
      d4 += value[j*120+58] * src[col_idx[j]+10];
      d5 += value[j*120+70] * src[col_idx[j]+10];
      d6 += value[j*120+82] * src[col_idx[j]+10];
      d7 += value[j*120+94] * src[col_idx[j]+10];
      d8 += value[j*120+106] * src[col_idx[j]+10];
      d9 += value[j*120+118] * src[col_idx[j]+10];
      d0 += value[j*120+11] * src[col_idx[j]+11];
      d1 += value[j*120+23] * src[col_idx[j]+11];
      d2 += value[j*120+35] * src[col_idx[j]+11];
      d3 += value[j*120+47] * src[col_idx[j]+11];
      d4 += value[j*120+59] * src[col_idx[j]+11];
      d5 += value[j*120+71] * src[col_idx[j]+11];
      d6 += value[j*120+83] * src[col_idx[j]+11];
      d7 += value[j*120+95] * src[col_idx[j]+11];
      d8 += value[j*120+107] * src[col_idx[j]+11];
      d9 += value[j*120+119] * src[col_idx[j]+11];
    }
    dest[10*i+0] = d0;
    dest[10*i+1] = d1;
    dest[10*i+2] = d2;
    dest[10*i+3] = d3;
    dest[10*i+4] = d4;
    dest[10*i+5] = d5;
    dest[10*i+6] = d6;
    dest[10*i+7] = d7;
    dest[10*i+8] = d8;
    dest[10*i+9] = d9;
  }
}
void bsmvm_11x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*11+0] * src[col_idx[j]+0];
      d1 += value[j*11+1] * src[col_idx[j]+0];
      d2 += value[j*11+2] * src[col_idx[j]+0];
      d3 += value[j*11+3] * src[col_idx[j]+0];
      d4 += value[j*11+4] * src[col_idx[j]+0];
      d5 += value[j*11+5] * src[col_idx[j]+0];
      d6 += value[j*11+6] * src[col_idx[j]+0];
      d7 += value[j*11+7] * src[col_idx[j]+0];
      d8 += value[j*11+8] * src[col_idx[j]+0];
      d9 += value[j*11+9] * src[col_idx[j]+0];
      d10 += value[j*11+10] * src[col_idx[j]+0];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*22+0] * src[col_idx[j]+0];
      d1 += value[j*22+2] * src[col_idx[j]+0];
      d2 += value[j*22+4] * src[col_idx[j]+0];
      d3 += value[j*22+6] * src[col_idx[j]+0];
      d4 += value[j*22+8] * src[col_idx[j]+0];
      d5 += value[j*22+10] * src[col_idx[j]+0];
      d6 += value[j*22+12] * src[col_idx[j]+0];
      d7 += value[j*22+14] * src[col_idx[j]+0];
      d8 += value[j*22+16] * src[col_idx[j]+0];
      d9 += value[j*22+18] * src[col_idx[j]+0];
      d10 += value[j*22+20] * src[col_idx[j]+0];
      d0 += value[j*22+1] * src[col_idx[j]+1];
      d1 += value[j*22+3] * src[col_idx[j]+1];
      d2 += value[j*22+5] * src[col_idx[j]+1];
      d3 += value[j*22+7] * src[col_idx[j]+1];
      d4 += value[j*22+9] * src[col_idx[j]+1];
      d5 += value[j*22+11] * src[col_idx[j]+1];
      d6 += value[j*22+13] * src[col_idx[j]+1];
      d7 += value[j*22+15] * src[col_idx[j]+1];
      d8 += value[j*22+17] * src[col_idx[j]+1];
      d9 += value[j*22+19] * src[col_idx[j]+1];
      d10 += value[j*22+21] * src[col_idx[j]+1];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*33+0] * src[col_idx[j]+0];
      d1 += value[j*33+3] * src[col_idx[j]+0];
      d2 += value[j*33+6] * src[col_idx[j]+0];
      d3 += value[j*33+9] * src[col_idx[j]+0];
      d4 += value[j*33+12] * src[col_idx[j]+0];
      d5 += value[j*33+15] * src[col_idx[j]+0];
      d6 += value[j*33+18] * src[col_idx[j]+0];
      d7 += value[j*33+21] * src[col_idx[j]+0];
      d8 += value[j*33+24] * src[col_idx[j]+0];
      d9 += value[j*33+27] * src[col_idx[j]+0];
      d10 += value[j*33+30] * src[col_idx[j]+0];
      d0 += value[j*33+1] * src[col_idx[j]+1];
      d1 += value[j*33+4] * src[col_idx[j]+1];
      d2 += value[j*33+7] * src[col_idx[j]+1];
      d3 += value[j*33+10] * src[col_idx[j]+1];
      d4 += value[j*33+13] * src[col_idx[j]+1];
      d5 += value[j*33+16] * src[col_idx[j]+1];
      d6 += value[j*33+19] * src[col_idx[j]+1];
      d7 += value[j*33+22] * src[col_idx[j]+1];
      d8 += value[j*33+25] * src[col_idx[j]+1];
      d9 += value[j*33+28] * src[col_idx[j]+1];
      d10 += value[j*33+31] * src[col_idx[j]+1];
      d0 += value[j*33+2] * src[col_idx[j]+2];
      d1 += value[j*33+5] * src[col_idx[j]+2];
      d2 += value[j*33+8] * src[col_idx[j]+2];
      d3 += value[j*33+11] * src[col_idx[j]+2];
      d4 += value[j*33+14] * src[col_idx[j]+2];
      d5 += value[j*33+17] * src[col_idx[j]+2];
      d6 += value[j*33+20] * src[col_idx[j]+2];
      d7 += value[j*33+23] * src[col_idx[j]+2];
      d8 += value[j*33+26] * src[col_idx[j]+2];
      d9 += value[j*33+29] * src[col_idx[j]+2];
      d10 += value[j*33+32] * src[col_idx[j]+2];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*44+0] * src[col_idx[j]+0];
      d1 += value[j*44+4] * src[col_idx[j]+0];
      d2 += value[j*44+8] * src[col_idx[j]+0];
      d3 += value[j*44+12] * src[col_idx[j]+0];
      d4 += value[j*44+16] * src[col_idx[j]+0];
      d5 += value[j*44+20] * src[col_idx[j]+0];
      d6 += value[j*44+24] * src[col_idx[j]+0];
      d7 += value[j*44+28] * src[col_idx[j]+0];
      d8 += value[j*44+32] * src[col_idx[j]+0];
      d9 += value[j*44+36] * src[col_idx[j]+0];
      d10 += value[j*44+40] * src[col_idx[j]+0];
      d0 += value[j*44+1] * src[col_idx[j]+1];
      d1 += value[j*44+5] * src[col_idx[j]+1];
      d2 += value[j*44+9] * src[col_idx[j]+1];
      d3 += value[j*44+13] * src[col_idx[j]+1];
      d4 += value[j*44+17] * src[col_idx[j]+1];
      d5 += value[j*44+21] * src[col_idx[j]+1];
      d6 += value[j*44+25] * src[col_idx[j]+1];
      d7 += value[j*44+29] * src[col_idx[j]+1];
      d8 += value[j*44+33] * src[col_idx[j]+1];
      d9 += value[j*44+37] * src[col_idx[j]+1];
      d10 += value[j*44+41] * src[col_idx[j]+1];
      d0 += value[j*44+2] * src[col_idx[j]+2];
      d1 += value[j*44+6] * src[col_idx[j]+2];
      d2 += value[j*44+10] * src[col_idx[j]+2];
      d3 += value[j*44+14] * src[col_idx[j]+2];
      d4 += value[j*44+18] * src[col_idx[j]+2];
      d5 += value[j*44+22] * src[col_idx[j]+2];
      d6 += value[j*44+26] * src[col_idx[j]+2];
      d7 += value[j*44+30] * src[col_idx[j]+2];
      d8 += value[j*44+34] * src[col_idx[j]+2];
      d9 += value[j*44+38] * src[col_idx[j]+2];
      d10 += value[j*44+42] * src[col_idx[j]+2];
      d0 += value[j*44+3] * src[col_idx[j]+3];
      d1 += value[j*44+7] * src[col_idx[j]+3];
      d2 += value[j*44+11] * src[col_idx[j]+3];
      d3 += value[j*44+15] * src[col_idx[j]+3];
      d4 += value[j*44+19] * src[col_idx[j]+3];
      d5 += value[j*44+23] * src[col_idx[j]+3];
      d6 += value[j*44+27] * src[col_idx[j]+3];
      d7 += value[j*44+31] * src[col_idx[j]+3];
      d8 += value[j*44+35] * src[col_idx[j]+3];
      d9 += value[j*44+39] * src[col_idx[j]+3];
      d10 += value[j*44+43] * src[col_idx[j]+3];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*55+0] * src[col_idx[j]+0];
      d1 += value[j*55+5] * src[col_idx[j]+0];
      d2 += value[j*55+10] * src[col_idx[j]+0];
      d3 += value[j*55+15] * src[col_idx[j]+0];
      d4 += value[j*55+20] * src[col_idx[j]+0];
      d5 += value[j*55+25] * src[col_idx[j]+0];
      d6 += value[j*55+30] * src[col_idx[j]+0];
      d7 += value[j*55+35] * src[col_idx[j]+0];
      d8 += value[j*55+40] * src[col_idx[j]+0];
      d9 += value[j*55+45] * src[col_idx[j]+0];
      d10 += value[j*55+50] * src[col_idx[j]+0];
      d0 += value[j*55+1] * src[col_idx[j]+1];
      d1 += value[j*55+6] * src[col_idx[j]+1];
      d2 += value[j*55+11] * src[col_idx[j]+1];
      d3 += value[j*55+16] * src[col_idx[j]+1];
      d4 += value[j*55+21] * src[col_idx[j]+1];
      d5 += value[j*55+26] * src[col_idx[j]+1];
      d6 += value[j*55+31] * src[col_idx[j]+1];
      d7 += value[j*55+36] * src[col_idx[j]+1];
      d8 += value[j*55+41] * src[col_idx[j]+1];
      d9 += value[j*55+46] * src[col_idx[j]+1];
      d10 += value[j*55+51] * src[col_idx[j]+1];
      d0 += value[j*55+2] * src[col_idx[j]+2];
      d1 += value[j*55+7] * src[col_idx[j]+2];
      d2 += value[j*55+12] * src[col_idx[j]+2];
      d3 += value[j*55+17] * src[col_idx[j]+2];
      d4 += value[j*55+22] * src[col_idx[j]+2];
      d5 += value[j*55+27] * src[col_idx[j]+2];
      d6 += value[j*55+32] * src[col_idx[j]+2];
      d7 += value[j*55+37] * src[col_idx[j]+2];
      d8 += value[j*55+42] * src[col_idx[j]+2];
      d9 += value[j*55+47] * src[col_idx[j]+2];
      d10 += value[j*55+52] * src[col_idx[j]+2];
      d0 += value[j*55+3] * src[col_idx[j]+3];
      d1 += value[j*55+8] * src[col_idx[j]+3];
      d2 += value[j*55+13] * src[col_idx[j]+3];
      d3 += value[j*55+18] * src[col_idx[j]+3];
      d4 += value[j*55+23] * src[col_idx[j]+3];
      d5 += value[j*55+28] * src[col_idx[j]+3];
      d6 += value[j*55+33] * src[col_idx[j]+3];
      d7 += value[j*55+38] * src[col_idx[j]+3];
      d8 += value[j*55+43] * src[col_idx[j]+3];
      d9 += value[j*55+48] * src[col_idx[j]+3];
      d10 += value[j*55+53] * src[col_idx[j]+3];
      d0 += value[j*55+4] * src[col_idx[j]+4];
      d1 += value[j*55+9] * src[col_idx[j]+4];
      d2 += value[j*55+14] * src[col_idx[j]+4];
      d3 += value[j*55+19] * src[col_idx[j]+4];
      d4 += value[j*55+24] * src[col_idx[j]+4];
      d5 += value[j*55+29] * src[col_idx[j]+4];
      d6 += value[j*55+34] * src[col_idx[j]+4];
      d7 += value[j*55+39] * src[col_idx[j]+4];
      d8 += value[j*55+44] * src[col_idx[j]+4];
      d9 += value[j*55+49] * src[col_idx[j]+4];
      d10 += value[j*55+54] * src[col_idx[j]+4];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*66+0] * src[col_idx[j]+0];
      d1 += value[j*66+6] * src[col_idx[j]+0];
      d2 += value[j*66+12] * src[col_idx[j]+0];
      d3 += value[j*66+18] * src[col_idx[j]+0];
      d4 += value[j*66+24] * src[col_idx[j]+0];
      d5 += value[j*66+30] * src[col_idx[j]+0];
      d6 += value[j*66+36] * src[col_idx[j]+0];
      d7 += value[j*66+42] * src[col_idx[j]+0];
      d8 += value[j*66+48] * src[col_idx[j]+0];
      d9 += value[j*66+54] * src[col_idx[j]+0];
      d10 += value[j*66+60] * src[col_idx[j]+0];
      d0 += value[j*66+1] * src[col_idx[j]+1];
      d1 += value[j*66+7] * src[col_idx[j]+1];
      d2 += value[j*66+13] * src[col_idx[j]+1];
      d3 += value[j*66+19] * src[col_idx[j]+1];
      d4 += value[j*66+25] * src[col_idx[j]+1];
      d5 += value[j*66+31] * src[col_idx[j]+1];
      d6 += value[j*66+37] * src[col_idx[j]+1];
      d7 += value[j*66+43] * src[col_idx[j]+1];
      d8 += value[j*66+49] * src[col_idx[j]+1];
      d9 += value[j*66+55] * src[col_idx[j]+1];
      d10 += value[j*66+61] * src[col_idx[j]+1];
      d0 += value[j*66+2] * src[col_idx[j]+2];
      d1 += value[j*66+8] * src[col_idx[j]+2];
      d2 += value[j*66+14] * src[col_idx[j]+2];
      d3 += value[j*66+20] * src[col_idx[j]+2];
      d4 += value[j*66+26] * src[col_idx[j]+2];
      d5 += value[j*66+32] * src[col_idx[j]+2];
      d6 += value[j*66+38] * src[col_idx[j]+2];
      d7 += value[j*66+44] * src[col_idx[j]+2];
      d8 += value[j*66+50] * src[col_idx[j]+2];
      d9 += value[j*66+56] * src[col_idx[j]+2];
      d10 += value[j*66+62] * src[col_idx[j]+2];
      d0 += value[j*66+3] * src[col_idx[j]+3];
      d1 += value[j*66+9] * src[col_idx[j]+3];
      d2 += value[j*66+15] * src[col_idx[j]+3];
      d3 += value[j*66+21] * src[col_idx[j]+3];
      d4 += value[j*66+27] * src[col_idx[j]+3];
      d5 += value[j*66+33] * src[col_idx[j]+3];
      d6 += value[j*66+39] * src[col_idx[j]+3];
      d7 += value[j*66+45] * src[col_idx[j]+3];
      d8 += value[j*66+51] * src[col_idx[j]+3];
      d9 += value[j*66+57] * src[col_idx[j]+3];
      d10 += value[j*66+63] * src[col_idx[j]+3];
      d0 += value[j*66+4] * src[col_idx[j]+4];
      d1 += value[j*66+10] * src[col_idx[j]+4];
      d2 += value[j*66+16] * src[col_idx[j]+4];
      d3 += value[j*66+22] * src[col_idx[j]+4];
      d4 += value[j*66+28] * src[col_idx[j]+4];
      d5 += value[j*66+34] * src[col_idx[j]+4];
      d6 += value[j*66+40] * src[col_idx[j]+4];
      d7 += value[j*66+46] * src[col_idx[j]+4];
      d8 += value[j*66+52] * src[col_idx[j]+4];
      d9 += value[j*66+58] * src[col_idx[j]+4];
      d10 += value[j*66+64] * src[col_idx[j]+4];
      d0 += value[j*66+5] * src[col_idx[j]+5];
      d1 += value[j*66+11] * src[col_idx[j]+5];
      d2 += value[j*66+17] * src[col_idx[j]+5];
      d3 += value[j*66+23] * src[col_idx[j]+5];
      d4 += value[j*66+29] * src[col_idx[j]+5];
      d5 += value[j*66+35] * src[col_idx[j]+5];
      d6 += value[j*66+41] * src[col_idx[j]+5];
      d7 += value[j*66+47] * src[col_idx[j]+5];
      d8 += value[j*66+53] * src[col_idx[j]+5];
      d9 += value[j*66+59] * src[col_idx[j]+5];
      d10 += value[j*66+65] * src[col_idx[j]+5];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*77+0] * src[col_idx[j]+0];
      d1 += value[j*77+7] * src[col_idx[j]+0];
      d2 += value[j*77+14] * src[col_idx[j]+0];
      d3 += value[j*77+21] * src[col_idx[j]+0];
      d4 += value[j*77+28] * src[col_idx[j]+0];
      d5 += value[j*77+35] * src[col_idx[j]+0];
      d6 += value[j*77+42] * src[col_idx[j]+0];
      d7 += value[j*77+49] * src[col_idx[j]+0];
      d8 += value[j*77+56] * src[col_idx[j]+0];
      d9 += value[j*77+63] * src[col_idx[j]+0];
      d10 += value[j*77+70] * src[col_idx[j]+0];
      d0 += value[j*77+1] * src[col_idx[j]+1];
      d1 += value[j*77+8] * src[col_idx[j]+1];
      d2 += value[j*77+15] * src[col_idx[j]+1];
      d3 += value[j*77+22] * src[col_idx[j]+1];
      d4 += value[j*77+29] * src[col_idx[j]+1];
      d5 += value[j*77+36] * src[col_idx[j]+1];
      d6 += value[j*77+43] * src[col_idx[j]+1];
      d7 += value[j*77+50] * src[col_idx[j]+1];
      d8 += value[j*77+57] * src[col_idx[j]+1];
      d9 += value[j*77+64] * src[col_idx[j]+1];
      d10 += value[j*77+71] * src[col_idx[j]+1];
      d0 += value[j*77+2] * src[col_idx[j]+2];
      d1 += value[j*77+9] * src[col_idx[j]+2];
      d2 += value[j*77+16] * src[col_idx[j]+2];
      d3 += value[j*77+23] * src[col_idx[j]+2];
      d4 += value[j*77+30] * src[col_idx[j]+2];
      d5 += value[j*77+37] * src[col_idx[j]+2];
      d6 += value[j*77+44] * src[col_idx[j]+2];
      d7 += value[j*77+51] * src[col_idx[j]+2];
      d8 += value[j*77+58] * src[col_idx[j]+2];
      d9 += value[j*77+65] * src[col_idx[j]+2];
      d10 += value[j*77+72] * src[col_idx[j]+2];
      d0 += value[j*77+3] * src[col_idx[j]+3];
      d1 += value[j*77+10] * src[col_idx[j]+3];
      d2 += value[j*77+17] * src[col_idx[j]+3];
      d3 += value[j*77+24] * src[col_idx[j]+3];
      d4 += value[j*77+31] * src[col_idx[j]+3];
      d5 += value[j*77+38] * src[col_idx[j]+3];
      d6 += value[j*77+45] * src[col_idx[j]+3];
      d7 += value[j*77+52] * src[col_idx[j]+3];
      d8 += value[j*77+59] * src[col_idx[j]+3];
      d9 += value[j*77+66] * src[col_idx[j]+3];
      d10 += value[j*77+73] * src[col_idx[j]+3];
      d0 += value[j*77+4] * src[col_idx[j]+4];
      d1 += value[j*77+11] * src[col_idx[j]+4];
      d2 += value[j*77+18] * src[col_idx[j]+4];
      d3 += value[j*77+25] * src[col_idx[j]+4];
      d4 += value[j*77+32] * src[col_idx[j]+4];
      d5 += value[j*77+39] * src[col_idx[j]+4];
      d6 += value[j*77+46] * src[col_idx[j]+4];
      d7 += value[j*77+53] * src[col_idx[j]+4];
      d8 += value[j*77+60] * src[col_idx[j]+4];
      d9 += value[j*77+67] * src[col_idx[j]+4];
      d10 += value[j*77+74] * src[col_idx[j]+4];
      d0 += value[j*77+5] * src[col_idx[j]+5];
      d1 += value[j*77+12] * src[col_idx[j]+5];
      d2 += value[j*77+19] * src[col_idx[j]+5];
      d3 += value[j*77+26] * src[col_idx[j]+5];
      d4 += value[j*77+33] * src[col_idx[j]+5];
      d5 += value[j*77+40] * src[col_idx[j]+5];
      d6 += value[j*77+47] * src[col_idx[j]+5];
      d7 += value[j*77+54] * src[col_idx[j]+5];
      d8 += value[j*77+61] * src[col_idx[j]+5];
      d9 += value[j*77+68] * src[col_idx[j]+5];
      d10 += value[j*77+75] * src[col_idx[j]+5];
      d0 += value[j*77+6] * src[col_idx[j]+6];
      d1 += value[j*77+13] * src[col_idx[j]+6];
      d2 += value[j*77+20] * src[col_idx[j]+6];
      d3 += value[j*77+27] * src[col_idx[j]+6];
      d4 += value[j*77+34] * src[col_idx[j]+6];
      d5 += value[j*77+41] * src[col_idx[j]+6];
      d6 += value[j*77+48] * src[col_idx[j]+6];
      d7 += value[j*77+55] * src[col_idx[j]+6];
      d8 += value[j*77+62] * src[col_idx[j]+6];
      d9 += value[j*77+69] * src[col_idx[j]+6];
      d10 += value[j*77+76] * src[col_idx[j]+6];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*88+0] * src[col_idx[j]+0];
      d1 += value[j*88+8] * src[col_idx[j]+0];
      d2 += value[j*88+16] * src[col_idx[j]+0];
      d3 += value[j*88+24] * src[col_idx[j]+0];
      d4 += value[j*88+32] * src[col_idx[j]+0];
      d5 += value[j*88+40] * src[col_idx[j]+0];
      d6 += value[j*88+48] * src[col_idx[j]+0];
      d7 += value[j*88+56] * src[col_idx[j]+0];
      d8 += value[j*88+64] * src[col_idx[j]+0];
      d9 += value[j*88+72] * src[col_idx[j]+0];
      d10 += value[j*88+80] * src[col_idx[j]+0];
      d0 += value[j*88+1] * src[col_idx[j]+1];
      d1 += value[j*88+9] * src[col_idx[j]+1];
      d2 += value[j*88+17] * src[col_idx[j]+1];
      d3 += value[j*88+25] * src[col_idx[j]+1];
      d4 += value[j*88+33] * src[col_idx[j]+1];
      d5 += value[j*88+41] * src[col_idx[j]+1];
      d6 += value[j*88+49] * src[col_idx[j]+1];
      d7 += value[j*88+57] * src[col_idx[j]+1];
      d8 += value[j*88+65] * src[col_idx[j]+1];
      d9 += value[j*88+73] * src[col_idx[j]+1];
      d10 += value[j*88+81] * src[col_idx[j]+1];
      d0 += value[j*88+2] * src[col_idx[j]+2];
      d1 += value[j*88+10] * src[col_idx[j]+2];
      d2 += value[j*88+18] * src[col_idx[j]+2];
      d3 += value[j*88+26] * src[col_idx[j]+2];
      d4 += value[j*88+34] * src[col_idx[j]+2];
      d5 += value[j*88+42] * src[col_idx[j]+2];
      d6 += value[j*88+50] * src[col_idx[j]+2];
      d7 += value[j*88+58] * src[col_idx[j]+2];
      d8 += value[j*88+66] * src[col_idx[j]+2];
      d9 += value[j*88+74] * src[col_idx[j]+2];
      d10 += value[j*88+82] * src[col_idx[j]+2];
      d0 += value[j*88+3] * src[col_idx[j]+3];
      d1 += value[j*88+11] * src[col_idx[j]+3];
      d2 += value[j*88+19] * src[col_idx[j]+3];
      d3 += value[j*88+27] * src[col_idx[j]+3];
      d4 += value[j*88+35] * src[col_idx[j]+3];
      d5 += value[j*88+43] * src[col_idx[j]+3];
      d6 += value[j*88+51] * src[col_idx[j]+3];
      d7 += value[j*88+59] * src[col_idx[j]+3];
      d8 += value[j*88+67] * src[col_idx[j]+3];
      d9 += value[j*88+75] * src[col_idx[j]+3];
      d10 += value[j*88+83] * src[col_idx[j]+3];
      d0 += value[j*88+4] * src[col_idx[j]+4];
      d1 += value[j*88+12] * src[col_idx[j]+4];
      d2 += value[j*88+20] * src[col_idx[j]+4];
      d3 += value[j*88+28] * src[col_idx[j]+4];
      d4 += value[j*88+36] * src[col_idx[j]+4];
      d5 += value[j*88+44] * src[col_idx[j]+4];
      d6 += value[j*88+52] * src[col_idx[j]+4];
      d7 += value[j*88+60] * src[col_idx[j]+4];
      d8 += value[j*88+68] * src[col_idx[j]+4];
      d9 += value[j*88+76] * src[col_idx[j]+4];
      d10 += value[j*88+84] * src[col_idx[j]+4];
      d0 += value[j*88+5] * src[col_idx[j]+5];
      d1 += value[j*88+13] * src[col_idx[j]+5];
      d2 += value[j*88+21] * src[col_idx[j]+5];
      d3 += value[j*88+29] * src[col_idx[j]+5];
      d4 += value[j*88+37] * src[col_idx[j]+5];
      d5 += value[j*88+45] * src[col_idx[j]+5];
      d6 += value[j*88+53] * src[col_idx[j]+5];
      d7 += value[j*88+61] * src[col_idx[j]+5];
      d8 += value[j*88+69] * src[col_idx[j]+5];
      d9 += value[j*88+77] * src[col_idx[j]+5];
      d10 += value[j*88+85] * src[col_idx[j]+5];
      d0 += value[j*88+6] * src[col_idx[j]+6];
      d1 += value[j*88+14] * src[col_idx[j]+6];
      d2 += value[j*88+22] * src[col_idx[j]+6];
      d3 += value[j*88+30] * src[col_idx[j]+6];
      d4 += value[j*88+38] * src[col_idx[j]+6];
      d5 += value[j*88+46] * src[col_idx[j]+6];
      d6 += value[j*88+54] * src[col_idx[j]+6];
      d7 += value[j*88+62] * src[col_idx[j]+6];
      d8 += value[j*88+70] * src[col_idx[j]+6];
      d9 += value[j*88+78] * src[col_idx[j]+6];
      d10 += value[j*88+86] * src[col_idx[j]+6];
      d0 += value[j*88+7] * src[col_idx[j]+7];
      d1 += value[j*88+15] * src[col_idx[j]+7];
      d2 += value[j*88+23] * src[col_idx[j]+7];
      d3 += value[j*88+31] * src[col_idx[j]+7];
      d4 += value[j*88+39] * src[col_idx[j]+7];
      d5 += value[j*88+47] * src[col_idx[j]+7];
      d6 += value[j*88+55] * src[col_idx[j]+7];
      d7 += value[j*88+63] * src[col_idx[j]+7];
      d8 += value[j*88+71] * src[col_idx[j]+7];
      d9 += value[j*88+79] * src[col_idx[j]+7];
      d10 += value[j*88+87] * src[col_idx[j]+7];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*99+0] * src[col_idx[j]+0];
      d1 += value[j*99+9] * src[col_idx[j]+0];
      d2 += value[j*99+18] * src[col_idx[j]+0];
      d3 += value[j*99+27] * src[col_idx[j]+0];
      d4 += value[j*99+36] * src[col_idx[j]+0];
      d5 += value[j*99+45] * src[col_idx[j]+0];
      d6 += value[j*99+54] * src[col_idx[j]+0];
      d7 += value[j*99+63] * src[col_idx[j]+0];
      d8 += value[j*99+72] * src[col_idx[j]+0];
      d9 += value[j*99+81] * src[col_idx[j]+0];
      d10 += value[j*99+90] * src[col_idx[j]+0];
      d0 += value[j*99+1] * src[col_idx[j]+1];
      d1 += value[j*99+10] * src[col_idx[j]+1];
      d2 += value[j*99+19] * src[col_idx[j]+1];
      d3 += value[j*99+28] * src[col_idx[j]+1];
      d4 += value[j*99+37] * src[col_idx[j]+1];
      d5 += value[j*99+46] * src[col_idx[j]+1];
      d6 += value[j*99+55] * src[col_idx[j]+1];
      d7 += value[j*99+64] * src[col_idx[j]+1];
      d8 += value[j*99+73] * src[col_idx[j]+1];
      d9 += value[j*99+82] * src[col_idx[j]+1];
      d10 += value[j*99+91] * src[col_idx[j]+1];
      d0 += value[j*99+2] * src[col_idx[j]+2];
      d1 += value[j*99+11] * src[col_idx[j]+2];
      d2 += value[j*99+20] * src[col_idx[j]+2];
      d3 += value[j*99+29] * src[col_idx[j]+2];
      d4 += value[j*99+38] * src[col_idx[j]+2];
      d5 += value[j*99+47] * src[col_idx[j]+2];
      d6 += value[j*99+56] * src[col_idx[j]+2];
      d7 += value[j*99+65] * src[col_idx[j]+2];
      d8 += value[j*99+74] * src[col_idx[j]+2];
      d9 += value[j*99+83] * src[col_idx[j]+2];
      d10 += value[j*99+92] * src[col_idx[j]+2];
      d0 += value[j*99+3] * src[col_idx[j]+3];
      d1 += value[j*99+12] * src[col_idx[j]+3];
      d2 += value[j*99+21] * src[col_idx[j]+3];
      d3 += value[j*99+30] * src[col_idx[j]+3];
      d4 += value[j*99+39] * src[col_idx[j]+3];
      d5 += value[j*99+48] * src[col_idx[j]+3];
      d6 += value[j*99+57] * src[col_idx[j]+3];
      d7 += value[j*99+66] * src[col_idx[j]+3];
      d8 += value[j*99+75] * src[col_idx[j]+3];
      d9 += value[j*99+84] * src[col_idx[j]+3];
      d10 += value[j*99+93] * src[col_idx[j]+3];
      d0 += value[j*99+4] * src[col_idx[j]+4];
      d1 += value[j*99+13] * src[col_idx[j]+4];
      d2 += value[j*99+22] * src[col_idx[j]+4];
      d3 += value[j*99+31] * src[col_idx[j]+4];
      d4 += value[j*99+40] * src[col_idx[j]+4];
      d5 += value[j*99+49] * src[col_idx[j]+4];
      d6 += value[j*99+58] * src[col_idx[j]+4];
      d7 += value[j*99+67] * src[col_idx[j]+4];
      d8 += value[j*99+76] * src[col_idx[j]+4];
      d9 += value[j*99+85] * src[col_idx[j]+4];
      d10 += value[j*99+94] * src[col_idx[j]+4];
      d0 += value[j*99+5] * src[col_idx[j]+5];
      d1 += value[j*99+14] * src[col_idx[j]+5];
      d2 += value[j*99+23] * src[col_idx[j]+5];
      d3 += value[j*99+32] * src[col_idx[j]+5];
      d4 += value[j*99+41] * src[col_idx[j]+5];
      d5 += value[j*99+50] * src[col_idx[j]+5];
      d6 += value[j*99+59] * src[col_idx[j]+5];
      d7 += value[j*99+68] * src[col_idx[j]+5];
      d8 += value[j*99+77] * src[col_idx[j]+5];
      d9 += value[j*99+86] * src[col_idx[j]+5];
      d10 += value[j*99+95] * src[col_idx[j]+5];
      d0 += value[j*99+6] * src[col_idx[j]+6];
      d1 += value[j*99+15] * src[col_idx[j]+6];
      d2 += value[j*99+24] * src[col_idx[j]+6];
      d3 += value[j*99+33] * src[col_idx[j]+6];
      d4 += value[j*99+42] * src[col_idx[j]+6];
      d5 += value[j*99+51] * src[col_idx[j]+6];
      d6 += value[j*99+60] * src[col_idx[j]+6];
      d7 += value[j*99+69] * src[col_idx[j]+6];
      d8 += value[j*99+78] * src[col_idx[j]+6];
      d9 += value[j*99+87] * src[col_idx[j]+6];
      d10 += value[j*99+96] * src[col_idx[j]+6];
      d0 += value[j*99+7] * src[col_idx[j]+7];
      d1 += value[j*99+16] * src[col_idx[j]+7];
      d2 += value[j*99+25] * src[col_idx[j]+7];
      d3 += value[j*99+34] * src[col_idx[j]+7];
      d4 += value[j*99+43] * src[col_idx[j]+7];
      d5 += value[j*99+52] * src[col_idx[j]+7];
      d6 += value[j*99+61] * src[col_idx[j]+7];
      d7 += value[j*99+70] * src[col_idx[j]+7];
      d8 += value[j*99+79] * src[col_idx[j]+7];
      d9 += value[j*99+88] * src[col_idx[j]+7];
      d10 += value[j*99+97] * src[col_idx[j]+7];
      d0 += value[j*99+8] * src[col_idx[j]+8];
      d1 += value[j*99+17] * src[col_idx[j]+8];
      d2 += value[j*99+26] * src[col_idx[j]+8];
      d3 += value[j*99+35] * src[col_idx[j]+8];
      d4 += value[j*99+44] * src[col_idx[j]+8];
      d5 += value[j*99+53] * src[col_idx[j]+8];
      d6 += value[j*99+62] * src[col_idx[j]+8];
      d7 += value[j*99+71] * src[col_idx[j]+8];
      d8 += value[j*99+80] * src[col_idx[j]+8];
      d9 += value[j*99+89] * src[col_idx[j]+8];
      d10 += value[j*99+98] * src[col_idx[j]+8];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*110+0] * src[col_idx[j]+0];
      d1 += value[j*110+10] * src[col_idx[j]+0];
      d2 += value[j*110+20] * src[col_idx[j]+0];
      d3 += value[j*110+30] * src[col_idx[j]+0];
      d4 += value[j*110+40] * src[col_idx[j]+0];
      d5 += value[j*110+50] * src[col_idx[j]+0];
      d6 += value[j*110+60] * src[col_idx[j]+0];
      d7 += value[j*110+70] * src[col_idx[j]+0];
      d8 += value[j*110+80] * src[col_idx[j]+0];
      d9 += value[j*110+90] * src[col_idx[j]+0];
      d10 += value[j*110+100] * src[col_idx[j]+0];
      d0 += value[j*110+1] * src[col_idx[j]+1];
      d1 += value[j*110+11] * src[col_idx[j]+1];
      d2 += value[j*110+21] * src[col_idx[j]+1];
      d3 += value[j*110+31] * src[col_idx[j]+1];
      d4 += value[j*110+41] * src[col_idx[j]+1];
      d5 += value[j*110+51] * src[col_idx[j]+1];
      d6 += value[j*110+61] * src[col_idx[j]+1];
      d7 += value[j*110+71] * src[col_idx[j]+1];
      d8 += value[j*110+81] * src[col_idx[j]+1];
      d9 += value[j*110+91] * src[col_idx[j]+1];
      d10 += value[j*110+101] * src[col_idx[j]+1];
      d0 += value[j*110+2] * src[col_idx[j]+2];
      d1 += value[j*110+12] * src[col_idx[j]+2];
      d2 += value[j*110+22] * src[col_idx[j]+2];
      d3 += value[j*110+32] * src[col_idx[j]+2];
      d4 += value[j*110+42] * src[col_idx[j]+2];
      d5 += value[j*110+52] * src[col_idx[j]+2];
      d6 += value[j*110+62] * src[col_idx[j]+2];
      d7 += value[j*110+72] * src[col_idx[j]+2];
      d8 += value[j*110+82] * src[col_idx[j]+2];
      d9 += value[j*110+92] * src[col_idx[j]+2];
      d10 += value[j*110+102] * src[col_idx[j]+2];
      d0 += value[j*110+3] * src[col_idx[j]+3];
      d1 += value[j*110+13] * src[col_idx[j]+3];
      d2 += value[j*110+23] * src[col_idx[j]+3];
      d3 += value[j*110+33] * src[col_idx[j]+3];
      d4 += value[j*110+43] * src[col_idx[j]+3];
      d5 += value[j*110+53] * src[col_idx[j]+3];
      d6 += value[j*110+63] * src[col_idx[j]+3];
      d7 += value[j*110+73] * src[col_idx[j]+3];
      d8 += value[j*110+83] * src[col_idx[j]+3];
      d9 += value[j*110+93] * src[col_idx[j]+3];
      d10 += value[j*110+103] * src[col_idx[j]+3];
      d0 += value[j*110+4] * src[col_idx[j]+4];
      d1 += value[j*110+14] * src[col_idx[j]+4];
      d2 += value[j*110+24] * src[col_idx[j]+4];
      d3 += value[j*110+34] * src[col_idx[j]+4];
      d4 += value[j*110+44] * src[col_idx[j]+4];
      d5 += value[j*110+54] * src[col_idx[j]+4];
      d6 += value[j*110+64] * src[col_idx[j]+4];
      d7 += value[j*110+74] * src[col_idx[j]+4];
      d8 += value[j*110+84] * src[col_idx[j]+4];
      d9 += value[j*110+94] * src[col_idx[j]+4];
      d10 += value[j*110+104] * src[col_idx[j]+4];
      d0 += value[j*110+5] * src[col_idx[j]+5];
      d1 += value[j*110+15] * src[col_idx[j]+5];
      d2 += value[j*110+25] * src[col_idx[j]+5];
      d3 += value[j*110+35] * src[col_idx[j]+5];
      d4 += value[j*110+45] * src[col_idx[j]+5];
      d5 += value[j*110+55] * src[col_idx[j]+5];
      d6 += value[j*110+65] * src[col_idx[j]+5];
      d7 += value[j*110+75] * src[col_idx[j]+5];
      d8 += value[j*110+85] * src[col_idx[j]+5];
      d9 += value[j*110+95] * src[col_idx[j]+5];
      d10 += value[j*110+105] * src[col_idx[j]+5];
      d0 += value[j*110+6] * src[col_idx[j]+6];
      d1 += value[j*110+16] * src[col_idx[j]+6];
      d2 += value[j*110+26] * src[col_idx[j]+6];
      d3 += value[j*110+36] * src[col_idx[j]+6];
      d4 += value[j*110+46] * src[col_idx[j]+6];
      d5 += value[j*110+56] * src[col_idx[j]+6];
      d6 += value[j*110+66] * src[col_idx[j]+6];
      d7 += value[j*110+76] * src[col_idx[j]+6];
      d8 += value[j*110+86] * src[col_idx[j]+6];
      d9 += value[j*110+96] * src[col_idx[j]+6];
      d10 += value[j*110+106] * src[col_idx[j]+6];
      d0 += value[j*110+7] * src[col_idx[j]+7];
      d1 += value[j*110+17] * src[col_idx[j]+7];
      d2 += value[j*110+27] * src[col_idx[j]+7];
      d3 += value[j*110+37] * src[col_idx[j]+7];
      d4 += value[j*110+47] * src[col_idx[j]+7];
      d5 += value[j*110+57] * src[col_idx[j]+7];
      d6 += value[j*110+67] * src[col_idx[j]+7];
      d7 += value[j*110+77] * src[col_idx[j]+7];
      d8 += value[j*110+87] * src[col_idx[j]+7];
      d9 += value[j*110+97] * src[col_idx[j]+7];
      d10 += value[j*110+107] * src[col_idx[j]+7];
      d0 += value[j*110+8] * src[col_idx[j]+8];
      d1 += value[j*110+18] * src[col_idx[j]+8];
      d2 += value[j*110+28] * src[col_idx[j]+8];
      d3 += value[j*110+38] * src[col_idx[j]+8];
      d4 += value[j*110+48] * src[col_idx[j]+8];
      d5 += value[j*110+58] * src[col_idx[j]+8];
      d6 += value[j*110+68] * src[col_idx[j]+8];
      d7 += value[j*110+78] * src[col_idx[j]+8];
      d8 += value[j*110+88] * src[col_idx[j]+8];
      d9 += value[j*110+98] * src[col_idx[j]+8];
      d10 += value[j*110+108] * src[col_idx[j]+8];
      d0 += value[j*110+9] * src[col_idx[j]+9];
      d1 += value[j*110+19] * src[col_idx[j]+9];
      d2 += value[j*110+29] * src[col_idx[j]+9];
      d3 += value[j*110+39] * src[col_idx[j]+9];
      d4 += value[j*110+49] * src[col_idx[j]+9];
      d5 += value[j*110+59] * src[col_idx[j]+9];
      d6 += value[j*110+69] * src[col_idx[j]+9];
      d7 += value[j*110+79] * src[col_idx[j]+9];
      d8 += value[j*110+89] * src[col_idx[j]+9];
      d9 += value[j*110+99] * src[col_idx[j]+9];
      d10 += value[j*110+109] * src[col_idx[j]+9];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*121+0] * src[col_idx[j]+0];
      d1 += value[j*121+11] * src[col_idx[j]+0];
      d2 += value[j*121+22] * src[col_idx[j]+0];
      d3 += value[j*121+33] * src[col_idx[j]+0];
      d4 += value[j*121+44] * src[col_idx[j]+0];
      d5 += value[j*121+55] * src[col_idx[j]+0];
      d6 += value[j*121+66] * src[col_idx[j]+0];
      d7 += value[j*121+77] * src[col_idx[j]+0];
      d8 += value[j*121+88] * src[col_idx[j]+0];
      d9 += value[j*121+99] * src[col_idx[j]+0];
      d10 += value[j*121+110] * src[col_idx[j]+0];
      d0 += value[j*121+1] * src[col_idx[j]+1];
      d1 += value[j*121+12] * src[col_idx[j]+1];
      d2 += value[j*121+23] * src[col_idx[j]+1];
      d3 += value[j*121+34] * src[col_idx[j]+1];
      d4 += value[j*121+45] * src[col_idx[j]+1];
      d5 += value[j*121+56] * src[col_idx[j]+1];
      d6 += value[j*121+67] * src[col_idx[j]+1];
      d7 += value[j*121+78] * src[col_idx[j]+1];
      d8 += value[j*121+89] * src[col_idx[j]+1];
      d9 += value[j*121+100] * src[col_idx[j]+1];
      d10 += value[j*121+111] * src[col_idx[j]+1];
      d0 += value[j*121+2] * src[col_idx[j]+2];
      d1 += value[j*121+13] * src[col_idx[j]+2];
      d2 += value[j*121+24] * src[col_idx[j]+2];
      d3 += value[j*121+35] * src[col_idx[j]+2];
      d4 += value[j*121+46] * src[col_idx[j]+2];
      d5 += value[j*121+57] * src[col_idx[j]+2];
      d6 += value[j*121+68] * src[col_idx[j]+2];
      d7 += value[j*121+79] * src[col_idx[j]+2];
      d8 += value[j*121+90] * src[col_idx[j]+2];
      d9 += value[j*121+101] * src[col_idx[j]+2];
      d10 += value[j*121+112] * src[col_idx[j]+2];
      d0 += value[j*121+3] * src[col_idx[j]+3];
      d1 += value[j*121+14] * src[col_idx[j]+3];
      d2 += value[j*121+25] * src[col_idx[j]+3];
      d3 += value[j*121+36] * src[col_idx[j]+3];
      d4 += value[j*121+47] * src[col_idx[j]+3];
      d5 += value[j*121+58] * src[col_idx[j]+3];
      d6 += value[j*121+69] * src[col_idx[j]+3];
      d7 += value[j*121+80] * src[col_idx[j]+3];
      d8 += value[j*121+91] * src[col_idx[j]+3];
      d9 += value[j*121+102] * src[col_idx[j]+3];
      d10 += value[j*121+113] * src[col_idx[j]+3];
      d0 += value[j*121+4] * src[col_idx[j]+4];
      d1 += value[j*121+15] * src[col_idx[j]+4];
      d2 += value[j*121+26] * src[col_idx[j]+4];
      d3 += value[j*121+37] * src[col_idx[j]+4];
      d4 += value[j*121+48] * src[col_idx[j]+4];
      d5 += value[j*121+59] * src[col_idx[j]+4];
      d6 += value[j*121+70] * src[col_idx[j]+4];
      d7 += value[j*121+81] * src[col_idx[j]+4];
      d8 += value[j*121+92] * src[col_idx[j]+4];
      d9 += value[j*121+103] * src[col_idx[j]+4];
      d10 += value[j*121+114] * src[col_idx[j]+4];
      d0 += value[j*121+5] * src[col_idx[j]+5];
      d1 += value[j*121+16] * src[col_idx[j]+5];
      d2 += value[j*121+27] * src[col_idx[j]+5];
      d3 += value[j*121+38] * src[col_idx[j]+5];
      d4 += value[j*121+49] * src[col_idx[j]+5];
      d5 += value[j*121+60] * src[col_idx[j]+5];
      d6 += value[j*121+71] * src[col_idx[j]+5];
      d7 += value[j*121+82] * src[col_idx[j]+5];
      d8 += value[j*121+93] * src[col_idx[j]+5];
      d9 += value[j*121+104] * src[col_idx[j]+5];
      d10 += value[j*121+115] * src[col_idx[j]+5];
      d0 += value[j*121+6] * src[col_idx[j]+6];
      d1 += value[j*121+17] * src[col_idx[j]+6];
      d2 += value[j*121+28] * src[col_idx[j]+6];
      d3 += value[j*121+39] * src[col_idx[j]+6];
      d4 += value[j*121+50] * src[col_idx[j]+6];
      d5 += value[j*121+61] * src[col_idx[j]+6];
      d6 += value[j*121+72] * src[col_idx[j]+6];
      d7 += value[j*121+83] * src[col_idx[j]+6];
      d8 += value[j*121+94] * src[col_idx[j]+6];
      d9 += value[j*121+105] * src[col_idx[j]+6];
      d10 += value[j*121+116] * src[col_idx[j]+6];
      d0 += value[j*121+7] * src[col_idx[j]+7];
      d1 += value[j*121+18] * src[col_idx[j]+7];
      d2 += value[j*121+29] * src[col_idx[j]+7];
      d3 += value[j*121+40] * src[col_idx[j]+7];
      d4 += value[j*121+51] * src[col_idx[j]+7];
      d5 += value[j*121+62] * src[col_idx[j]+7];
      d6 += value[j*121+73] * src[col_idx[j]+7];
      d7 += value[j*121+84] * src[col_idx[j]+7];
      d8 += value[j*121+95] * src[col_idx[j]+7];
      d9 += value[j*121+106] * src[col_idx[j]+7];
      d10 += value[j*121+117] * src[col_idx[j]+7];
      d0 += value[j*121+8] * src[col_idx[j]+8];
      d1 += value[j*121+19] * src[col_idx[j]+8];
      d2 += value[j*121+30] * src[col_idx[j]+8];
      d3 += value[j*121+41] * src[col_idx[j]+8];
      d4 += value[j*121+52] * src[col_idx[j]+8];
      d5 += value[j*121+63] * src[col_idx[j]+8];
      d6 += value[j*121+74] * src[col_idx[j]+8];
      d7 += value[j*121+85] * src[col_idx[j]+8];
      d8 += value[j*121+96] * src[col_idx[j]+8];
      d9 += value[j*121+107] * src[col_idx[j]+8];
      d10 += value[j*121+118] * src[col_idx[j]+8];
      d0 += value[j*121+9] * src[col_idx[j]+9];
      d1 += value[j*121+20] * src[col_idx[j]+9];
      d2 += value[j*121+31] * src[col_idx[j]+9];
      d3 += value[j*121+42] * src[col_idx[j]+9];
      d4 += value[j*121+53] * src[col_idx[j]+9];
      d5 += value[j*121+64] * src[col_idx[j]+9];
      d6 += value[j*121+75] * src[col_idx[j]+9];
      d7 += value[j*121+86] * src[col_idx[j]+9];
      d8 += value[j*121+97] * src[col_idx[j]+9];
      d9 += value[j*121+108] * src[col_idx[j]+9];
      d10 += value[j*121+119] * src[col_idx[j]+9];
      d0 += value[j*121+10] * src[col_idx[j]+10];
      d1 += value[j*121+21] * src[col_idx[j]+10];
      d2 += value[j*121+32] * src[col_idx[j]+10];
      d3 += value[j*121+43] * src[col_idx[j]+10];
      d4 += value[j*121+54] * src[col_idx[j]+10];
      d5 += value[j*121+65] * src[col_idx[j]+10];
      d6 += value[j*121+76] * src[col_idx[j]+10];
      d7 += value[j*121+87] * src[col_idx[j]+10];
      d8 += value[j*121+98] * src[col_idx[j]+10];
      d9 += value[j*121+109] * src[col_idx[j]+10];
      d10 += value[j*121+120] * src[col_idx[j]+10];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_11x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10;
    d0 = dest[11*i+0];
    d1 = dest[11*i+1];
    d2 = dest[11*i+2];
    d3 = dest[11*i+3];
    d4 = dest[11*i+4];
    d5 = dest[11*i+5];
    d6 = dest[11*i+6];
    d7 = dest[11*i+7];
    d8 = dest[11*i+8];
    d9 = dest[11*i+9];
    d10 = dest[11*i+10];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*132+0] * src[col_idx[j]+0];
      d1 += value[j*132+12] * src[col_idx[j]+0];
      d2 += value[j*132+24] * src[col_idx[j]+0];
      d3 += value[j*132+36] * src[col_idx[j]+0];
      d4 += value[j*132+48] * src[col_idx[j]+0];
      d5 += value[j*132+60] * src[col_idx[j]+0];
      d6 += value[j*132+72] * src[col_idx[j]+0];
      d7 += value[j*132+84] * src[col_idx[j]+0];
      d8 += value[j*132+96] * src[col_idx[j]+0];
      d9 += value[j*132+108] * src[col_idx[j]+0];
      d10 += value[j*132+120] * src[col_idx[j]+0];
      d0 += value[j*132+1] * src[col_idx[j]+1];
      d1 += value[j*132+13] * src[col_idx[j]+1];
      d2 += value[j*132+25] * src[col_idx[j]+1];
      d3 += value[j*132+37] * src[col_idx[j]+1];
      d4 += value[j*132+49] * src[col_idx[j]+1];
      d5 += value[j*132+61] * src[col_idx[j]+1];
      d6 += value[j*132+73] * src[col_idx[j]+1];
      d7 += value[j*132+85] * src[col_idx[j]+1];
      d8 += value[j*132+97] * src[col_idx[j]+1];
      d9 += value[j*132+109] * src[col_idx[j]+1];
      d10 += value[j*132+121] * src[col_idx[j]+1];
      d0 += value[j*132+2] * src[col_idx[j]+2];
      d1 += value[j*132+14] * src[col_idx[j]+2];
      d2 += value[j*132+26] * src[col_idx[j]+2];
      d3 += value[j*132+38] * src[col_idx[j]+2];
      d4 += value[j*132+50] * src[col_idx[j]+2];
      d5 += value[j*132+62] * src[col_idx[j]+2];
      d6 += value[j*132+74] * src[col_idx[j]+2];
      d7 += value[j*132+86] * src[col_idx[j]+2];
      d8 += value[j*132+98] * src[col_idx[j]+2];
      d9 += value[j*132+110] * src[col_idx[j]+2];
      d10 += value[j*132+122] * src[col_idx[j]+2];
      d0 += value[j*132+3] * src[col_idx[j]+3];
      d1 += value[j*132+15] * src[col_idx[j]+3];
      d2 += value[j*132+27] * src[col_idx[j]+3];
      d3 += value[j*132+39] * src[col_idx[j]+3];
      d4 += value[j*132+51] * src[col_idx[j]+3];
      d5 += value[j*132+63] * src[col_idx[j]+3];
      d6 += value[j*132+75] * src[col_idx[j]+3];
      d7 += value[j*132+87] * src[col_idx[j]+3];
      d8 += value[j*132+99] * src[col_idx[j]+3];
      d9 += value[j*132+111] * src[col_idx[j]+3];
      d10 += value[j*132+123] * src[col_idx[j]+3];
      d0 += value[j*132+4] * src[col_idx[j]+4];
      d1 += value[j*132+16] * src[col_idx[j]+4];
      d2 += value[j*132+28] * src[col_idx[j]+4];
      d3 += value[j*132+40] * src[col_idx[j]+4];
      d4 += value[j*132+52] * src[col_idx[j]+4];
      d5 += value[j*132+64] * src[col_idx[j]+4];
      d6 += value[j*132+76] * src[col_idx[j]+4];
      d7 += value[j*132+88] * src[col_idx[j]+4];
      d8 += value[j*132+100] * src[col_idx[j]+4];
      d9 += value[j*132+112] * src[col_idx[j]+4];
      d10 += value[j*132+124] * src[col_idx[j]+4];
      d0 += value[j*132+5] * src[col_idx[j]+5];
      d1 += value[j*132+17] * src[col_idx[j]+5];
      d2 += value[j*132+29] * src[col_idx[j]+5];
      d3 += value[j*132+41] * src[col_idx[j]+5];
      d4 += value[j*132+53] * src[col_idx[j]+5];
      d5 += value[j*132+65] * src[col_idx[j]+5];
      d6 += value[j*132+77] * src[col_idx[j]+5];
      d7 += value[j*132+89] * src[col_idx[j]+5];
      d8 += value[j*132+101] * src[col_idx[j]+5];
      d9 += value[j*132+113] * src[col_idx[j]+5];
      d10 += value[j*132+125] * src[col_idx[j]+5];
      d0 += value[j*132+6] * src[col_idx[j]+6];
      d1 += value[j*132+18] * src[col_idx[j]+6];
      d2 += value[j*132+30] * src[col_idx[j]+6];
      d3 += value[j*132+42] * src[col_idx[j]+6];
      d4 += value[j*132+54] * src[col_idx[j]+6];
      d5 += value[j*132+66] * src[col_idx[j]+6];
      d6 += value[j*132+78] * src[col_idx[j]+6];
      d7 += value[j*132+90] * src[col_idx[j]+6];
      d8 += value[j*132+102] * src[col_idx[j]+6];
      d9 += value[j*132+114] * src[col_idx[j]+6];
      d10 += value[j*132+126] * src[col_idx[j]+6];
      d0 += value[j*132+7] * src[col_idx[j]+7];
      d1 += value[j*132+19] * src[col_idx[j]+7];
      d2 += value[j*132+31] * src[col_idx[j]+7];
      d3 += value[j*132+43] * src[col_idx[j]+7];
      d4 += value[j*132+55] * src[col_idx[j]+7];
      d5 += value[j*132+67] * src[col_idx[j]+7];
      d6 += value[j*132+79] * src[col_idx[j]+7];
      d7 += value[j*132+91] * src[col_idx[j]+7];
      d8 += value[j*132+103] * src[col_idx[j]+7];
      d9 += value[j*132+115] * src[col_idx[j]+7];
      d10 += value[j*132+127] * src[col_idx[j]+7];
      d0 += value[j*132+8] * src[col_idx[j]+8];
      d1 += value[j*132+20] * src[col_idx[j]+8];
      d2 += value[j*132+32] * src[col_idx[j]+8];
      d3 += value[j*132+44] * src[col_idx[j]+8];
      d4 += value[j*132+56] * src[col_idx[j]+8];
      d5 += value[j*132+68] * src[col_idx[j]+8];
      d6 += value[j*132+80] * src[col_idx[j]+8];
      d7 += value[j*132+92] * src[col_idx[j]+8];
      d8 += value[j*132+104] * src[col_idx[j]+8];
      d9 += value[j*132+116] * src[col_idx[j]+8];
      d10 += value[j*132+128] * src[col_idx[j]+8];
      d0 += value[j*132+9] * src[col_idx[j]+9];
      d1 += value[j*132+21] * src[col_idx[j]+9];
      d2 += value[j*132+33] * src[col_idx[j]+9];
      d3 += value[j*132+45] * src[col_idx[j]+9];
      d4 += value[j*132+57] * src[col_idx[j]+9];
      d5 += value[j*132+69] * src[col_idx[j]+9];
      d6 += value[j*132+81] * src[col_idx[j]+9];
      d7 += value[j*132+93] * src[col_idx[j]+9];
      d8 += value[j*132+105] * src[col_idx[j]+9];
      d9 += value[j*132+117] * src[col_idx[j]+9];
      d10 += value[j*132+129] * src[col_idx[j]+9];
      d0 += value[j*132+10] * src[col_idx[j]+10];
      d1 += value[j*132+22] * src[col_idx[j]+10];
      d2 += value[j*132+34] * src[col_idx[j]+10];
      d3 += value[j*132+46] * src[col_idx[j]+10];
      d4 += value[j*132+58] * src[col_idx[j]+10];
      d5 += value[j*132+70] * src[col_idx[j]+10];
      d6 += value[j*132+82] * src[col_idx[j]+10];
      d7 += value[j*132+94] * src[col_idx[j]+10];
      d8 += value[j*132+106] * src[col_idx[j]+10];
      d9 += value[j*132+118] * src[col_idx[j]+10];
      d10 += value[j*132+130] * src[col_idx[j]+10];
      d0 += value[j*132+11] * src[col_idx[j]+11];
      d1 += value[j*132+23] * src[col_idx[j]+11];
      d2 += value[j*132+35] * src[col_idx[j]+11];
      d3 += value[j*132+47] * src[col_idx[j]+11];
      d4 += value[j*132+59] * src[col_idx[j]+11];
      d5 += value[j*132+71] * src[col_idx[j]+11];
      d6 += value[j*132+83] * src[col_idx[j]+11];
      d7 += value[j*132+95] * src[col_idx[j]+11];
      d8 += value[j*132+107] * src[col_idx[j]+11];
      d9 += value[j*132+119] * src[col_idx[j]+11];
      d10 += value[j*132+131] * src[col_idx[j]+11];
    }
    dest[11*i+0] = d0;
    dest[11*i+1] = d1;
    dest[11*i+2] = d2;
    dest[11*i+3] = d3;
    dest[11*i+4] = d4;
    dest[11*i+5] = d5;
    dest[11*i+6] = d6;
    dest[11*i+7] = d7;
    dest[11*i+8] = d8;
    dest[11*i+9] = d9;
    dest[11*i+10] = d10;
  }
}
void bsmvm_12x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*12+0] * src[col_idx[j]+0];
      d1 += value[j*12+1] * src[col_idx[j]+0];
      d2 += value[j*12+2] * src[col_idx[j]+0];
      d3 += value[j*12+3] * src[col_idx[j]+0];
      d4 += value[j*12+4] * src[col_idx[j]+0];
      d5 += value[j*12+5] * src[col_idx[j]+0];
      d6 += value[j*12+6] * src[col_idx[j]+0];
      d7 += value[j*12+7] * src[col_idx[j]+0];
      d8 += value[j*12+8] * src[col_idx[j]+0];
      d9 += value[j*12+9] * src[col_idx[j]+0];
      d10 += value[j*12+10] * src[col_idx[j]+0];
      d11 += value[j*12+11] * src[col_idx[j]+0];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*24+0] * src[col_idx[j]+0];
      d1 += value[j*24+2] * src[col_idx[j]+0];
      d2 += value[j*24+4] * src[col_idx[j]+0];
      d3 += value[j*24+6] * src[col_idx[j]+0];
      d4 += value[j*24+8] * src[col_idx[j]+0];
      d5 += value[j*24+10] * src[col_idx[j]+0];
      d6 += value[j*24+12] * src[col_idx[j]+0];
      d7 += value[j*24+14] * src[col_idx[j]+0];
      d8 += value[j*24+16] * src[col_idx[j]+0];
      d9 += value[j*24+18] * src[col_idx[j]+0];
      d10 += value[j*24+20] * src[col_idx[j]+0];
      d11 += value[j*24+22] * src[col_idx[j]+0];
      d0 += value[j*24+1] * src[col_idx[j]+1];
      d1 += value[j*24+3] * src[col_idx[j]+1];
      d2 += value[j*24+5] * src[col_idx[j]+1];
      d3 += value[j*24+7] * src[col_idx[j]+1];
      d4 += value[j*24+9] * src[col_idx[j]+1];
      d5 += value[j*24+11] * src[col_idx[j]+1];
      d6 += value[j*24+13] * src[col_idx[j]+1];
      d7 += value[j*24+15] * src[col_idx[j]+1];
      d8 += value[j*24+17] * src[col_idx[j]+1];
      d9 += value[j*24+19] * src[col_idx[j]+1];
      d10 += value[j*24+21] * src[col_idx[j]+1];
      d11 += value[j*24+23] * src[col_idx[j]+1];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*36+0] * src[col_idx[j]+0];
      d1 += value[j*36+3] * src[col_idx[j]+0];
      d2 += value[j*36+6] * src[col_idx[j]+0];
      d3 += value[j*36+9] * src[col_idx[j]+0];
      d4 += value[j*36+12] * src[col_idx[j]+0];
      d5 += value[j*36+15] * src[col_idx[j]+0];
      d6 += value[j*36+18] * src[col_idx[j]+0];
      d7 += value[j*36+21] * src[col_idx[j]+0];
      d8 += value[j*36+24] * src[col_idx[j]+0];
      d9 += value[j*36+27] * src[col_idx[j]+0];
      d10 += value[j*36+30] * src[col_idx[j]+0];
      d11 += value[j*36+33] * src[col_idx[j]+0];
      d0 += value[j*36+1] * src[col_idx[j]+1];
      d1 += value[j*36+4] * src[col_idx[j]+1];
      d2 += value[j*36+7] * src[col_idx[j]+1];
      d3 += value[j*36+10] * src[col_idx[j]+1];
      d4 += value[j*36+13] * src[col_idx[j]+1];
      d5 += value[j*36+16] * src[col_idx[j]+1];
      d6 += value[j*36+19] * src[col_idx[j]+1];
      d7 += value[j*36+22] * src[col_idx[j]+1];
      d8 += value[j*36+25] * src[col_idx[j]+1];
      d9 += value[j*36+28] * src[col_idx[j]+1];
      d10 += value[j*36+31] * src[col_idx[j]+1];
      d11 += value[j*36+34] * src[col_idx[j]+1];
      d0 += value[j*36+2] * src[col_idx[j]+2];
      d1 += value[j*36+5] * src[col_idx[j]+2];
      d2 += value[j*36+8] * src[col_idx[j]+2];
      d3 += value[j*36+11] * src[col_idx[j]+2];
      d4 += value[j*36+14] * src[col_idx[j]+2];
      d5 += value[j*36+17] * src[col_idx[j]+2];
      d6 += value[j*36+20] * src[col_idx[j]+2];
      d7 += value[j*36+23] * src[col_idx[j]+2];
      d8 += value[j*36+26] * src[col_idx[j]+2];
      d9 += value[j*36+29] * src[col_idx[j]+2];
      d10 += value[j*36+32] * src[col_idx[j]+2];
      d11 += value[j*36+35] * src[col_idx[j]+2];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*48+0] * src[col_idx[j]+0];
      d1 += value[j*48+4] * src[col_idx[j]+0];
      d2 += value[j*48+8] * src[col_idx[j]+0];
      d3 += value[j*48+12] * src[col_idx[j]+0];
      d4 += value[j*48+16] * src[col_idx[j]+0];
      d5 += value[j*48+20] * src[col_idx[j]+0];
      d6 += value[j*48+24] * src[col_idx[j]+0];
      d7 += value[j*48+28] * src[col_idx[j]+0];
      d8 += value[j*48+32] * src[col_idx[j]+0];
      d9 += value[j*48+36] * src[col_idx[j]+0];
      d10 += value[j*48+40] * src[col_idx[j]+0];
      d11 += value[j*48+44] * src[col_idx[j]+0];
      d0 += value[j*48+1] * src[col_idx[j]+1];
      d1 += value[j*48+5] * src[col_idx[j]+1];
      d2 += value[j*48+9] * src[col_idx[j]+1];
      d3 += value[j*48+13] * src[col_idx[j]+1];
      d4 += value[j*48+17] * src[col_idx[j]+1];
      d5 += value[j*48+21] * src[col_idx[j]+1];
      d6 += value[j*48+25] * src[col_idx[j]+1];
      d7 += value[j*48+29] * src[col_idx[j]+1];
      d8 += value[j*48+33] * src[col_idx[j]+1];
      d9 += value[j*48+37] * src[col_idx[j]+1];
      d10 += value[j*48+41] * src[col_idx[j]+1];
      d11 += value[j*48+45] * src[col_idx[j]+1];
      d0 += value[j*48+2] * src[col_idx[j]+2];
      d1 += value[j*48+6] * src[col_idx[j]+2];
      d2 += value[j*48+10] * src[col_idx[j]+2];
      d3 += value[j*48+14] * src[col_idx[j]+2];
      d4 += value[j*48+18] * src[col_idx[j]+2];
      d5 += value[j*48+22] * src[col_idx[j]+2];
      d6 += value[j*48+26] * src[col_idx[j]+2];
      d7 += value[j*48+30] * src[col_idx[j]+2];
      d8 += value[j*48+34] * src[col_idx[j]+2];
      d9 += value[j*48+38] * src[col_idx[j]+2];
      d10 += value[j*48+42] * src[col_idx[j]+2];
      d11 += value[j*48+46] * src[col_idx[j]+2];
      d0 += value[j*48+3] * src[col_idx[j]+3];
      d1 += value[j*48+7] * src[col_idx[j]+3];
      d2 += value[j*48+11] * src[col_idx[j]+3];
      d3 += value[j*48+15] * src[col_idx[j]+3];
      d4 += value[j*48+19] * src[col_idx[j]+3];
      d5 += value[j*48+23] * src[col_idx[j]+3];
      d6 += value[j*48+27] * src[col_idx[j]+3];
      d7 += value[j*48+31] * src[col_idx[j]+3];
      d8 += value[j*48+35] * src[col_idx[j]+3];
      d9 += value[j*48+39] * src[col_idx[j]+3];
      d10 += value[j*48+43] * src[col_idx[j]+3];
      d11 += value[j*48+47] * src[col_idx[j]+3];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*60+0] * src[col_idx[j]+0];
      d1 += value[j*60+5] * src[col_idx[j]+0];
      d2 += value[j*60+10] * src[col_idx[j]+0];
      d3 += value[j*60+15] * src[col_idx[j]+0];
      d4 += value[j*60+20] * src[col_idx[j]+0];
      d5 += value[j*60+25] * src[col_idx[j]+0];
      d6 += value[j*60+30] * src[col_idx[j]+0];
      d7 += value[j*60+35] * src[col_idx[j]+0];
      d8 += value[j*60+40] * src[col_idx[j]+0];
      d9 += value[j*60+45] * src[col_idx[j]+0];
      d10 += value[j*60+50] * src[col_idx[j]+0];
      d11 += value[j*60+55] * src[col_idx[j]+0];
      d0 += value[j*60+1] * src[col_idx[j]+1];
      d1 += value[j*60+6] * src[col_idx[j]+1];
      d2 += value[j*60+11] * src[col_idx[j]+1];
      d3 += value[j*60+16] * src[col_idx[j]+1];
      d4 += value[j*60+21] * src[col_idx[j]+1];
      d5 += value[j*60+26] * src[col_idx[j]+1];
      d6 += value[j*60+31] * src[col_idx[j]+1];
      d7 += value[j*60+36] * src[col_idx[j]+1];
      d8 += value[j*60+41] * src[col_idx[j]+1];
      d9 += value[j*60+46] * src[col_idx[j]+1];
      d10 += value[j*60+51] * src[col_idx[j]+1];
      d11 += value[j*60+56] * src[col_idx[j]+1];
      d0 += value[j*60+2] * src[col_idx[j]+2];
      d1 += value[j*60+7] * src[col_idx[j]+2];
      d2 += value[j*60+12] * src[col_idx[j]+2];
      d3 += value[j*60+17] * src[col_idx[j]+2];
      d4 += value[j*60+22] * src[col_idx[j]+2];
      d5 += value[j*60+27] * src[col_idx[j]+2];
      d6 += value[j*60+32] * src[col_idx[j]+2];
      d7 += value[j*60+37] * src[col_idx[j]+2];
      d8 += value[j*60+42] * src[col_idx[j]+2];
      d9 += value[j*60+47] * src[col_idx[j]+2];
      d10 += value[j*60+52] * src[col_idx[j]+2];
      d11 += value[j*60+57] * src[col_idx[j]+2];
      d0 += value[j*60+3] * src[col_idx[j]+3];
      d1 += value[j*60+8] * src[col_idx[j]+3];
      d2 += value[j*60+13] * src[col_idx[j]+3];
      d3 += value[j*60+18] * src[col_idx[j]+3];
      d4 += value[j*60+23] * src[col_idx[j]+3];
      d5 += value[j*60+28] * src[col_idx[j]+3];
      d6 += value[j*60+33] * src[col_idx[j]+3];
      d7 += value[j*60+38] * src[col_idx[j]+3];
      d8 += value[j*60+43] * src[col_idx[j]+3];
      d9 += value[j*60+48] * src[col_idx[j]+3];
      d10 += value[j*60+53] * src[col_idx[j]+3];
      d11 += value[j*60+58] * src[col_idx[j]+3];
      d0 += value[j*60+4] * src[col_idx[j]+4];
      d1 += value[j*60+9] * src[col_idx[j]+4];
      d2 += value[j*60+14] * src[col_idx[j]+4];
      d3 += value[j*60+19] * src[col_idx[j]+4];
      d4 += value[j*60+24] * src[col_idx[j]+4];
      d5 += value[j*60+29] * src[col_idx[j]+4];
      d6 += value[j*60+34] * src[col_idx[j]+4];
      d7 += value[j*60+39] * src[col_idx[j]+4];
      d8 += value[j*60+44] * src[col_idx[j]+4];
      d9 += value[j*60+49] * src[col_idx[j]+4];
      d10 += value[j*60+54] * src[col_idx[j]+4];
      d11 += value[j*60+59] * src[col_idx[j]+4];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*72+0] * src[col_idx[j]+0];
      d1 += value[j*72+6] * src[col_idx[j]+0];
      d2 += value[j*72+12] * src[col_idx[j]+0];
      d3 += value[j*72+18] * src[col_idx[j]+0];
      d4 += value[j*72+24] * src[col_idx[j]+0];
      d5 += value[j*72+30] * src[col_idx[j]+0];
      d6 += value[j*72+36] * src[col_idx[j]+0];
      d7 += value[j*72+42] * src[col_idx[j]+0];
      d8 += value[j*72+48] * src[col_idx[j]+0];
      d9 += value[j*72+54] * src[col_idx[j]+0];
      d10 += value[j*72+60] * src[col_idx[j]+0];
      d11 += value[j*72+66] * src[col_idx[j]+0];
      d0 += value[j*72+1] * src[col_idx[j]+1];
      d1 += value[j*72+7] * src[col_idx[j]+1];
      d2 += value[j*72+13] * src[col_idx[j]+1];
      d3 += value[j*72+19] * src[col_idx[j]+1];
      d4 += value[j*72+25] * src[col_idx[j]+1];
      d5 += value[j*72+31] * src[col_idx[j]+1];
      d6 += value[j*72+37] * src[col_idx[j]+1];
      d7 += value[j*72+43] * src[col_idx[j]+1];
      d8 += value[j*72+49] * src[col_idx[j]+1];
      d9 += value[j*72+55] * src[col_idx[j]+1];
      d10 += value[j*72+61] * src[col_idx[j]+1];
      d11 += value[j*72+67] * src[col_idx[j]+1];
      d0 += value[j*72+2] * src[col_idx[j]+2];
      d1 += value[j*72+8] * src[col_idx[j]+2];
      d2 += value[j*72+14] * src[col_idx[j]+2];
      d3 += value[j*72+20] * src[col_idx[j]+2];
      d4 += value[j*72+26] * src[col_idx[j]+2];
      d5 += value[j*72+32] * src[col_idx[j]+2];
      d6 += value[j*72+38] * src[col_idx[j]+2];
      d7 += value[j*72+44] * src[col_idx[j]+2];
      d8 += value[j*72+50] * src[col_idx[j]+2];
      d9 += value[j*72+56] * src[col_idx[j]+2];
      d10 += value[j*72+62] * src[col_idx[j]+2];
      d11 += value[j*72+68] * src[col_idx[j]+2];
      d0 += value[j*72+3] * src[col_idx[j]+3];
      d1 += value[j*72+9] * src[col_idx[j]+3];
      d2 += value[j*72+15] * src[col_idx[j]+3];
      d3 += value[j*72+21] * src[col_idx[j]+3];
      d4 += value[j*72+27] * src[col_idx[j]+3];
      d5 += value[j*72+33] * src[col_idx[j]+3];
      d6 += value[j*72+39] * src[col_idx[j]+3];
      d7 += value[j*72+45] * src[col_idx[j]+3];
      d8 += value[j*72+51] * src[col_idx[j]+3];
      d9 += value[j*72+57] * src[col_idx[j]+3];
      d10 += value[j*72+63] * src[col_idx[j]+3];
      d11 += value[j*72+69] * src[col_idx[j]+3];
      d0 += value[j*72+4] * src[col_idx[j]+4];
      d1 += value[j*72+10] * src[col_idx[j]+4];
      d2 += value[j*72+16] * src[col_idx[j]+4];
      d3 += value[j*72+22] * src[col_idx[j]+4];
      d4 += value[j*72+28] * src[col_idx[j]+4];
      d5 += value[j*72+34] * src[col_idx[j]+4];
      d6 += value[j*72+40] * src[col_idx[j]+4];
      d7 += value[j*72+46] * src[col_idx[j]+4];
      d8 += value[j*72+52] * src[col_idx[j]+4];
      d9 += value[j*72+58] * src[col_idx[j]+4];
      d10 += value[j*72+64] * src[col_idx[j]+4];
      d11 += value[j*72+70] * src[col_idx[j]+4];
      d0 += value[j*72+5] * src[col_idx[j]+5];
      d1 += value[j*72+11] * src[col_idx[j]+5];
      d2 += value[j*72+17] * src[col_idx[j]+5];
      d3 += value[j*72+23] * src[col_idx[j]+5];
      d4 += value[j*72+29] * src[col_idx[j]+5];
      d5 += value[j*72+35] * src[col_idx[j]+5];
      d6 += value[j*72+41] * src[col_idx[j]+5];
      d7 += value[j*72+47] * src[col_idx[j]+5];
      d8 += value[j*72+53] * src[col_idx[j]+5];
      d9 += value[j*72+59] * src[col_idx[j]+5];
      d10 += value[j*72+65] * src[col_idx[j]+5];
      d11 += value[j*72+71] * src[col_idx[j]+5];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*84+0] * src[col_idx[j]+0];
      d1 += value[j*84+7] * src[col_idx[j]+0];
      d2 += value[j*84+14] * src[col_idx[j]+0];
      d3 += value[j*84+21] * src[col_idx[j]+0];
      d4 += value[j*84+28] * src[col_idx[j]+0];
      d5 += value[j*84+35] * src[col_idx[j]+0];
      d6 += value[j*84+42] * src[col_idx[j]+0];
      d7 += value[j*84+49] * src[col_idx[j]+0];
      d8 += value[j*84+56] * src[col_idx[j]+0];
      d9 += value[j*84+63] * src[col_idx[j]+0];
      d10 += value[j*84+70] * src[col_idx[j]+0];
      d11 += value[j*84+77] * src[col_idx[j]+0];
      d0 += value[j*84+1] * src[col_idx[j]+1];
      d1 += value[j*84+8] * src[col_idx[j]+1];
      d2 += value[j*84+15] * src[col_idx[j]+1];
      d3 += value[j*84+22] * src[col_idx[j]+1];
      d4 += value[j*84+29] * src[col_idx[j]+1];
      d5 += value[j*84+36] * src[col_idx[j]+1];
      d6 += value[j*84+43] * src[col_idx[j]+1];
      d7 += value[j*84+50] * src[col_idx[j]+1];
      d8 += value[j*84+57] * src[col_idx[j]+1];
      d9 += value[j*84+64] * src[col_idx[j]+1];
      d10 += value[j*84+71] * src[col_idx[j]+1];
      d11 += value[j*84+78] * src[col_idx[j]+1];
      d0 += value[j*84+2] * src[col_idx[j]+2];
      d1 += value[j*84+9] * src[col_idx[j]+2];
      d2 += value[j*84+16] * src[col_idx[j]+2];
      d3 += value[j*84+23] * src[col_idx[j]+2];
      d4 += value[j*84+30] * src[col_idx[j]+2];
      d5 += value[j*84+37] * src[col_idx[j]+2];
      d6 += value[j*84+44] * src[col_idx[j]+2];
      d7 += value[j*84+51] * src[col_idx[j]+2];
      d8 += value[j*84+58] * src[col_idx[j]+2];
      d9 += value[j*84+65] * src[col_idx[j]+2];
      d10 += value[j*84+72] * src[col_idx[j]+2];
      d11 += value[j*84+79] * src[col_idx[j]+2];
      d0 += value[j*84+3] * src[col_idx[j]+3];
      d1 += value[j*84+10] * src[col_idx[j]+3];
      d2 += value[j*84+17] * src[col_idx[j]+3];
      d3 += value[j*84+24] * src[col_idx[j]+3];
      d4 += value[j*84+31] * src[col_idx[j]+3];
      d5 += value[j*84+38] * src[col_idx[j]+3];
      d6 += value[j*84+45] * src[col_idx[j]+3];
      d7 += value[j*84+52] * src[col_idx[j]+3];
      d8 += value[j*84+59] * src[col_idx[j]+3];
      d9 += value[j*84+66] * src[col_idx[j]+3];
      d10 += value[j*84+73] * src[col_idx[j]+3];
      d11 += value[j*84+80] * src[col_idx[j]+3];
      d0 += value[j*84+4] * src[col_idx[j]+4];
      d1 += value[j*84+11] * src[col_idx[j]+4];
      d2 += value[j*84+18] * src[col_idx[j]+4];
      d3 += value[j*84+25] * src[col_idx[j]+4];
      d4 += value[j*84+32] * src[col_idx[j]+4];
      d5 += value[j*84+39] * src[col_idx[j]+4];
      d6 += value[j*84+46] * src[col_idx[j]+4];
      d7 += value[j*84+53] * src[col_idx[j]+4];
      d8 += value[j*84+60] * src[col_idx[j]+4];
      d9 += value[j*84+67] * src[col_idx[j]+4];
      d10 += value[j*84+74] * src[col_idx[j]+4];
      d11 += value[j*84+81] * src[col_idx[j]+4];
      d0 += value[j*84+5] * src[col_idx[j]+5];
      d1 += value[j*84+12] * src[col_idx[j]+5];
      d2 += value[j*84+19] * src[col_idx[j]+5];
      d3 += value[j*84+26] * src[col_idx[j]+5];
      d4 += value[j*84+33] * src[col_idx[j]+5];
      d5 += value[j*84+40] * src[col_idx[j]+5];
      d6 += value[j*84+47] * src[col_idx[j]+5];
      d7 += value[j*84+54] * src[col_idx[j]+5];
      d8 += value[j*84+61] * src[col_idx[j]+5];
      d9 += value[j*84+68] * src[col_idx[j]+5];
      d10 += value[j*84+75] * src[col_idx[j]+5];
      d11 += value[j*84+82] * src[col_idx[j]+5];
      d0 += value[j*84+6] * src[col_idx[j]+6];
      d1 += value[j*84+13] * src[col_idx[j]+6];
      d2 += value[j*84+20] * src[col_idx[j]+6];
      d3 += value[j*84+27] * src[col_idx[j]+6];
      d4 += value[j*84+34] * src[col_idx[j]+6];
      d5 += value[j*84+41] * src[col_idx[j]+6];
      d6 += value[j*84+48] * src[col_idx[j]+6];
      d7 += value[j*84+55] * src[col_idx[j]+6];
      d8 += value[j*84+62] * src[col_idx[j]+6];
      d9 += value[j*84+69] * src[col_idx[j]+6];
      d10 += value[j*84+76] * src[col_idx[j]+6];
      d11 += value[j*84+83] * src[col_idx[j]+6];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*96+0] * src[col_idx[j]+0];
      d1 += value[j*96+8] * src[col_idx[j]+0];
      d2 += value[j*96+16] * src[col_idx[j]+0];
      d3 += value[j*96+24] * src[col_idx[j]+0];
      d4 += value[j*96+32] * src[col_idx[j]+0];
      d5 += value[j*96+40] * src[col_idx[j]+0];
      d6 += value[j*96+48] * src[col_idx[j]+0];
      d7 += value[j*96+56] * src[col_idx[j]+0];
      d8 += value[j*96+64] * src[col_idx[j]+0];
      d9 += value[j*96+72] * src[col_idx[j]+0];
      d10 += value[j*96+80] * src[col_idx[j]+0];
      d11 += value[j*96+88] * src[col_idx[j]+0];
      d0 += value[j*96+1] * src[col_idx[j]+1];
      d1 += value[j*96+9] * src[col_idx[j]+1];
      d2 += value[j*96+17] * src[col_idx[j]+1];
      d3 += value[j*96+25] * src[col_idx[j]+1];
      d4 += value[j*96+33] * src[col_idx[j]+1];
      d5 += value[j*96+41] * src[col_idx[j]+1];
      d6 += value[j*96+49] * src[col_idx[j]+1];
      d7 += value[j*96+57] * src[col_idx[j]+1];
      d8 += value[j*96+65] * src[col_idx[j]+1];
      d9 += value[j*96+73] * src[col_idx[j]+1];
      d10 += value[j*96+81] * src[col_idx[j]+1];
      d11 += value[j*96+89] * src[col_idx[j]+1];
      d0 += value[j*96+2] * src[col_idx[j]+2];
      d1 += value[j*96+10] * src[col_idx[j]+2];
      d2 += value[j*96+18] * src[col_idx[j]+2];
      d3 += value[j*96+26] * src[col_idx[j]+2];
      d4 += value[j*96+34] * src[col_idx[j]+2];
      d5 += value[j*96+42] * src[col_idx[j]+2];
      d6 += value[j*96+50] * src[col_idx[j]+2];
      d7 += value[j*96+58] * src[col_idx[j]+2];
      d8 += value[j*96+66] * src[col_idx[j]+2];
      d9 += value[j*96+74] * src[col_idx[j]+2];
      d10 += value[j*96+82] * src[col_idx[j]+2];
      d11 += value[j*96+90] * src[col_idx[j]+2];
      d0 += value[j*96+3] * src[col_idx[j]+3];
      d1 += value[j*96+11] * src[col_idx[j]+3];
      d2 += value[j*96+19] * src[col_idx[j]+3];
      d3 += value[j*96+27] * src[col_idx[j]+3];
      d4 += value[j*96+35] * src[col_idx[j]+3];
      d5 += value[j*96+43] * src[col_idx[j]+3];
      d6 += value[j*96+51] * src[col_idx[j]+3];
      d7 += value[j*96+59] * src[col_idx[j]+3];
      d8 += value[j*96+67] * src[col_idx[j]+3];
      d9 += value[j*96+75] * src[col_idx[j]+3];
      d10 += value[j*96+83] * src[col_idx[j]+3];
      d11 += value[j*96+91] * src[col_idx[j]+3];
      d0 += value[j*96+4] * src[col_idx[j]+4];
      d1 += value[j*96+12] * src[col_idx[j]+4];
      d2 += value[j*96+20] * src[col_idx[j]+4];
      d3 += value[j*96+28] * src[col_idx[j]+4];
      d4 += value[j*96+36] * src[col_idx[j]+4];
      d5 += value[j*96+44] * src[col_idx[j]+4];
      d6 += value[j*96+52] * src[col_idx[j]+4];
      d7 += value[j*96+60] * src[col_idx[j]+4];
      d8 += value[j*96+68] * src[col_idx[j]+4];
      d9 += value[j*96+76] * src[col_idx[j]+4];
      d10 += value[j*96+84] * src[col_idx[j]+4];
      d11 += value[j*96+92] * src[col_idx[j]+4];
      d0 += value[j*96+5] * src[col_idx[j]+5];
      d1 += value[j*96+13] * src[col_idx[j]+5];
      d2 += value[j*96+21] * src[col_idx[j]+5];
      d3 += value[j*96+29] * src[col_idx[j]+5];
      d4 += value[j*96+37] * src[col_idx[j]+5];
      d5 += value[j*96+45] * src[col_idx[j]+5];
      d6 += value[j*96+53] * src[col_idx[j]+5];
      d7 += value[j*96+61] * src[col_idx[j]+5];
      d8 += value[j*96+69] * src[col_idx[j]+5];
      d9 += value[j*96+77] * src[col_idx[j]+5];
      d10 += value[j*96+85] * src[col_idx[j]+5];
      d11 += value[j*96+93] * src[col_idx[j]+5];
      d0 += value[j*96+6] * src[col_idx[j]+6];
      d1 += value[j*96+14] * src[col_idx[j]+6];
      d2 += value[j*96+22] * src[col_idx[j]+6];
      d3 += value[j*96+30] * src[col_idx[j]+6];
      d4 += value[j*96+38] * src[col_idx[j]+6];
      d5 += value[j*96+46] * src[col_idx[j]+6];
      d6 += value[j*96+54] * src[col_idx[j]+6];
      d7 += value[j*96+62] * src[col_idx[j]+6];
      d8 += value[j*96+70] * src[col_idx[j]+6];
      d9 += value[j*96+78] * src[col_idx[j]+6];
      d10 += value[j*96+86] * src[col_idx[j]+6];
      d11 += value[j*96+94] * src[col_idx[j]+6];
      d0 += value[j*96+7] * src[col_idx[j]+7];
      d1 += value[j*96+15] * src[col_idx[j]+7];
      d2 += value[j*96+23] * src[col_idx[j]+7];
      d3 += value[j*96+31] * src[col_idx[j]+7];
      d4 += value[j*96+39] * src[col_idx[j]+7];
      d5 += value[j*96+47] * src[col_idx[j]+7];
      d6 += value[j*96+55] * src[col_idx[j]+7];
      d7 += value[j*96+63] * src[col_idx[j]+7];
      d8 += value[j*96+71] * src[col_idx[j]+7];
      d9 += value[j*96+79] * src[col_idx[j]+7];
      d10 += value[j*96+87] * src[col_idx[j]+7];
      d11 += value[j*96+95] * src[col_idx[j]+7];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*108+0] * src[col_idx[j]+0];
      d1 += value[j*108+9] * src[col_idx[j]+0];
      d2 += value[j*108+18] * src[col_idx[j]+0];
      d3 += value[j*108+27] * src[col_idx[j]+0];
      d4 += value[j*108+36] * src[col_idx[j]+0];
      d5 += value[j*108+45] * src[col_idx[j]+0];
      d6 += value[j*108+54] * src[col_idx[j]+0];
      d7 += value[j*108+63] * src[col_idx[j]+0];
      d8 += value[j*108+72] * src[col_idx[j]+0];
      d9 += value[j*108+81] * src[col_idx[j]+0];
      d10 += value[j*108+90] * src[col_idx[j]+0];
      d11 += value[j*108+99] * src[col_idx[j]+0];
      d0 += value[j*108+1] * src[col_idx[j]+1];
      d1 += value[j*108+10] * src[col_idx[j]+1];
      d2 += value[j*108+19] * src[col_idx[j]+1];
      d3 += value[j*108+28] * src[col_idx[j]+1];
      d4 += value[j*108+37] * src[col_idx[j]+1];
      d5 += value[j*108+46] * src[col_idx[j]+1];
      d6 += value[j*108+55] * src[col_idx[j]+1];
      d7 += value[j*108+64] * src[col_idx[j]+1];
      d8 += value[j*108+73] * src[col_idx[j]+1];
      d9 += value[j*108+82] * src[col_idx[j]+1];
      d10 += value[j*108+91] * src[col_idx[j]+1];
      d11 += value[j*108+100] * src[col_idx[j]+1];
      d0 += value[j*108+2] * src[col_idx[j]+2];
      d1 += value[j*108+11] * src[col_idx[j]+2];
      d2 += value[j*108+20] * src[col_idx[j]+2];
      d3 += value[j*108+29] * src[col_idx[j]+2];
      d4 += value[j*108+38] * src[col_idx[j]+2];
      d5 += value[j*108+47] * src[col_idx[j]+2];
      d6 += value[j*108+56] * src[col_idx[j]+2];
      d7 += value[j*108+65] * src[col_idx[j]+2];
      d8 += value[j*108+74] * src[col_idx[j]+2];
      d9 += value[j*108+83] * src[col_idx[j]+2];
      d10 += value[j*108+92] * src[col_idx[j]+2];
      d11 += value[j*108+101] * src[col_idx[j]+2];
      d0 += value[j*108+3] * src[col_idx[j]+3];
      d1 += value[j*108+12] * src[col_idx[j]+3];
      d2 += value[j*108+21] * src[col_idx[j]+3];
      d3 += value[j*108+30] * src[col_idx[j]+3];
      d4 += value[j*108+39] * src[col_idx[j]+3];
      d5 += value[j*108+48] * src[col_idx[j]+3];
      d6 += value[j*108+57] * src[col_idx[j]+3];
      d7 += value[j*108+66] * src[col_idx[j]+3];
      d8 += value[j*108+75] * src[col_idx[j]+3];
      d9 += value[j*108+84] * src[col_idx[j]+3];
      d10 += value[j*108+93] * src[col_idx[j]+3];
      d11 += value[j*108+102] * src[col_idx[j]+3];
      d0 += value[j*108+4] * src[col_idx[j]+4];
      d1 += value[j*108+13] * src[col_idx[j]+4];
      d2 += value[j*108+22] * src[col_idx[j]+4];
      d3 += value[j*108+31] * src[col_idx[j]+4];
      d4 += value[j*108+40] * src[col_idx[j]+4];
      d5 += value[j*108+49] * src[col_idx[j]+4];
      d6 += value[j*108+58] * src[col_idx[j]+4];
      d7 += value[j*108+67] * src[col_idx[j]+4];
      d8 += value[j*108+76] * src[col_idx[j]+4];
      d9 += value[j*108+85] * src[col_idx[j]+4];
      d10 += value[j*108+94] * src[col_idx[j]+4];
      d11 += value[j*108+103] * src[col_idx[j]+4];
      d0 += value[j*108+5] * src[col_idx[j]+5];
      d1 += value[j*108+14] * src[col_idx[j]+5];
      d2 += value[j*108+23] * src[col_idx[j]+5];
      d3 += value[j*108+32] * src[col_idx[j]+5];
      d4 += value[j*108+41] * src[col_idx[j]+5];
      d5 += value[j*108+50] * src[col_idx[j]+5];
      d6 += value[j*108+59] * src[col_idx[j]+5];
      d7 += value[j*108+68] * src[col_idx[j]+5];
      d8 += value[j*108+77] * src[col_idx[j]+5];
      d9 += value[j*108+86] * src[col_idx[j]+5];
      d10 += value[j*108+95] * src[col_idx[j]+5];
      d11 += value[j*108+104] * src[col_idx[j]+5];
      d0 += value[j*108+6] * src[col_idx[j]+6];
      d1 += value[j*108+15] * src[col_idx[j]+6];
      d2 += value[j*108+24] * src[col_idx[j]+6];
      d3 += value[j*108+33] * src[col_idx[j]+6];
      d4 += value[j*108+42] * src[col_idx[j]+6];
      d5 += value[j*108+51] * src[col_idx[j]+6];
      d6 += value[j*108+60] * src[col_idx[j]+6];
      d7 += value[j*108+69] * src[col_idx[j]+6];
      d8 += value[j*108+78] * src[col_idx[j]+6];
      d9 += value[j*108+87] * src[col_idx[j]+6];
      d10 += value[j*108+96] * src[col_idx[j]+6];
      d11 += value[j*108+105] * src[col_idx[j]+6];
      d0 += value[j*108+7] * src[col_idx[j]+7];
      d1 += value[j*108+16] * src[col_idx[j]+7];
      d2 += value[j*108+25] * src[col_idx[j]+7];
      d3 += value[j*108+34] * src[col_idx[j]+7];
      d4 += value[j*108+43] * src[col_idx[j]+7];
      d5 += value[j*108+52] * src[col_idx[j]+7];
      d6 += value[j*108+61] * src[col_idx[j]+7];
      d7 += value[j*108+70] * src[col_idx[j]+7];
      d8 += value[j*108+79] * src[col_idx[j]+7];
      d9 += value[j*108+88] * src[col_idx[j]+7];
      d10 += value[j*108+97] * src[col_idx[j]+7];
      d11 += value[j*108+106] * src[col_idx[j]+7];
      d0 += value[j*108+8] * src[col_idx[j]+8];
      d1 += value[j*108+17] * src[col_idx[j]+8];
      d2 += value[j*108+26] * src[col_idx[j]+8];
      d3 += value[j*108+35] * src[col_idx[j]+8];
      d4 += value[j*108+44] * src[col_idx[j]+8];
      d5 += value[j*108+53] * src[col_idx[j]+8];
      d6 += value[j*108+62] * src[col_idx[j]+8];
      d7 += value[j*108+71] * src[col_idx[j]+8];
      d8 += value[j*108+80] * src[col_idx[j]+8];
      d9 += value[j*108+89] * src[col_idx[j]+8];
      d10 += value[j*108+98] * src[col_idx[j]+8];
      d11 += value[j*108+107] * src[col_idx[j]+8];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*120+0] * src[col_idx[j]+0];
      d1 += value[j*120+10] * src[col_idx[j]+0];
      d2 += value[j*120+20] * src[col_idx[j]+0];
      d3 += value[j*120+30] * src[col_idx[j]+0];
      d4 += value[j*120+40] * src[col_idx[j]+0];
      d5 += value[j*120+50] * src[col_idx[j]+0];
      d6 += value[j*120+60] * src[col_idx[j]+0];
      d7 += value[j*120+70] * src[col_idx[j]+0];
      d8 += value[j*120+80] * src[col_idx[j]+0];
      d9 += value[j*120+90] * src[col_idx[j]+0];
      d10 += value[j*120+100] * src[col_idx[j]+0];
      d11 += value[j*120+110] * src[col_idx[j]+0];
      d0 += value[j*120+1] * src[col_idx[j]+1];
      d1 += value[j*120+11] * src[col_idx[j]+1];
      d2 += value[j*120+21] * src[col_idx[j]+1];
      d3 += value[j*120+31] * src[col_idx[j]+1];
      d4 += value[j*120+41] * src[col_idx[j]+1];
      d5 += value[j*120+51] * src[col_idx[j]+1];
      d6 += value[j*120+61] * src[col_idx[j]+1];
      d7 += value[j*120+71] * src[col_idx[j]+1];
      d8 += value[j*120+81] * src[col_idx[j]+1];
      d9 += value[j*120+91] * src[col_idx[j]+1];
      d10 += value[j*120+101] * src[col_idx[j]+1];
      d11 += value[j*120+111] * src[col_idx[j]+1];
      d0 += value[j*120+2] * src[col_idx[j]+2];
      d1 += value[j*120+12] * src[col_idx[j]+2];
      d2 += value[j*120+22] * src[col_idx[j]+2];
      d3 += value[j*120+32] * src[col_idx[j]+2];
      d4 += value[j*120+42] * src[col_idx[j]+2];
      d5 += value[j*120+52] * src[col_idx[j]+2];
      d6 += value[j*120+62] * src[col_idx[j]+2];
      d7 += value[j*120+72] * src[col_idx[j]+2];
      d8 += value[j*120+82] * src[col_idx[j]+2];
      d9 += value[j*120+92] * src[col_idx[j]+2];
      d10 += value[j*120+102] * src[col_idx[j]+2];
      d11 += value[j*120+112] * src[col_idx[j]+2];
      d0 += value[j*120+3] * src[col_idx[j]+3];
      d1 += value[j*120+13] * src[col_idx[j]+3];
      d2 += value[j*120+23] * src[col_idx[j]+3];
      d3 += value[j*120+33] * src[col_idx[j]+3];
      d4 += value[j*120+43] * src[col_idx[j]+3];
      d5 += value[j*120+53] * src[col_idx[j]+3];
      d6 += value[j*120+63] * src[col_idx[j]+3];
      d7 += value[j*120+73] * src[col_idx[j]+3];
      d8 += value[j*120+83] * src[col_idx[j]+3];
      d9 += value[j*120+93] * src[col_idx[j]+3];
      d10 += value[j*120+103] * src[col_idx[j]+3];
      d11 += value[j*120+113] * src[col_idx[j]+3];
      d0 += value[j*120+4] * src[col_idx[j]+4];
      d1 += value[j*120+14] * src[col_idx[j]+4];
      d2 += value[j*120+24] * src[col_idx[j]+4];
      d3 += value[j*120+34] * src[col_idx[j]+4];
      d4 += value[j*120+44] * src[col_idx[j]+4];
      d5 += value[j*120+54] * src[col_idx[j]+4];
      d6 += value[j*120+64] * src[col_idx[j]+4];
      d7 += value[j*120+74] * src[col_idx[j]+4];
      d8 += value[j*120+84] * src[col_idx[j]+4];
      d9 += value[j*120+94] * src[col_idx[j]+4];
      d10 += value[j*120+104] * src[col_idx[j]+4];
      d11 += value[j*120+114] * src[col_idx[j]+4];
      d0 += value[j*120+5] * src[col_idx[j]+5];
      d1 += value[j*120+15] * src[col_idx[j]+5];
      d2 += value[j*120+25] * src[col_idx[j]+5];
      d3 += value[j*120+35] * src[col_idx[j]+5];
      d4 += value[j*120+45] * src[col_idx[j]+5];
      d5 += value[j*120+55] * src[col_idx[j]+5];
      d6 += value[j*120+65] * src[col_idx[j]+5];
      d7 += value[j*120+75] * src[col_idx[j]+5];
      d8 += value[j*120+85] * src[col_idx[j]+5];
      d9 += value[j*120+95] * src[col_idx[j]+5];
      d10 += value[j*120+105] * src[col_idx[j]+5];
      d11 += value[j*120+115] * src[col_idx[j]+5];
      d0 += value[j*120+6] * src[col_idx[j]+6];
      d1 += value[j*120+16] * src[col_idx[j]+6];
      d2 += value[j*120+26] * src[col_idx[j]+6];
      d3 += value[j*120+36] * src[col_idx[j]+6];
      d4 += value[j*120+46] * src[col_idx[j]+6];
      d5 += value[j*120+56] * src[col_idx[j]+6];
      d6 += value[j*120+66] * src[col_idx[j]+6];
      d7 += value[j*120+76] * src[col_idx[j]+6];
      d8 += value[j*120+86] * src[col_idx[j]+6];
      d9 += value[j*120+96] * src[col_idx[j]+6];
      d10 += value[j*120+106] * src[col_idx[j]+6];
      d11 += value[j*120+116] * src[col_idx[j]+6];
      d0 += value[j*120+7] * src[col_idx[j]+7];
      d1 += value[j*120+17] * src[col_idx[j]+7];
      d2 += value[j*120+27] * src[col_idx[j]+7];
      d3 += value[j*120+37] * src[col_idx[j]+7];
      d4 += value[j*120+47] * src[col_idx[j]+7];
      d5 += value[j*120+57] * src[col_idx[j]+7];
      d6 += value[j*120+67] * src[col_idx[j]+7];
      d7 += value[j*120+77] * src[col_idx[j]+7];
      d8 += value[j*120+87] * src[col_idx[j]+7];
      d9 += value[j*120+97] * src[col_idx[j]+7];
      d10 += value[j*120+107] * src[col_idx[j]+7];
      d11 += value[j*120+117] * src[col_idx[j]+7];
      d0 += value[j*120+8] * src[col_idx[j]+8];
      d1 += value[j*120+18] * src[col_idx[j]+8];
      d2 += value[j*120+28] * src[col_idx[j]+8];
      d3 += value[j*120+38] * src[col_idx[j]+8];
      d4 += value[j*120+48] * src[col_idx[j]+8];
      d5 += value[j*120+58] * src[col_idx[j]+8];
      d6 += value[j*120+68] * src[col_idx[j]+8];
      d7 += value[j*120+78] * src[col_idx[j]+8];
      d8 += value[j*120+88] * src[col_idx[j]+8];
      d9 += value[j*120+98] * src[col_idx[j]+8];
      d10 += value[j*120+108] * src[col_idx[j]+8];
      d11 += value[j*120+118] * src[col_idx[j]+8];
      d0 += value[j*120+9] * src[col_idx[j]+9];
      d1 += value[j*120+19] * src[col_idx[j]+9];
      d2 += value[j*120+29] * src[col_idx[j]+9];
      d3 += value[j*120+39] * src[col_idx[j]+9];
      d4 += value[j*120+49] * src[col_idx[j]+9];
      d5 += value[j*120+59] * src[col_idx[j]+9];
      d6 += value[j*120+69] * src[col_idx[j]+9];
      d7 += value[j*120+79] * src[col_idx[j]+9];
      d8 += value[j*120+89] * src[col_idx[j]+9];
      d9 += value[j*120+99] * src[col_idx[j]+9];
      d10 += value[j*120+109] * src[col_idx[j]+9];
      d11 += value[j*120+119] * src[col_idx[j]+9];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*132+0] * src[col_idx[j]+0];
      d1 += value[j*132+11] * src[col_idx[j]+0];
      d2 += value[j*132+22] * src[col_idx[j]+0];
      d3 += value[j*132+33] * src[col_idx[j]+0];
      d4 += value[j*132+44] * src[col_idx[j]+0];
      d5 += value[j*132+55] * src[col_idx[j]+0];
      d6 += value[j*132+66] * src[col_idx[j]+0];
      d7 += value[j*132+77] * src[col_idx[j]+0];
      d8 += value[j*132+88] * src[col_idx[j]+0];
      d9 += value[j*132+99] * src[col_idx[j]+0];
      d10 += value[j*132+110] * src[col_idx[j]+0];
      d11 += value[j*132+121] * src[col_idx[j]+0];
      d0 += value[j*132+1] * src[col_idx[j]+1];
      d1 += value[j*132+12] * src[col_idx[j]+1];
      d2 += value[j*132+23] * src[col_idx[j]+1];
      d3 += value[j*132+34] * src[col_idx[j]+1];
      d4 += value[j*132+45] * src[col_idx[j]+1];
      d5 += value[j*132+56] * src[col_idx[j]+1];
      d6 += value[j*132+67] * src[col_idx[j]+1];
      d7 += value[j*132+78] * src[col_idx[j]+1];
      d8 += value[j*132+89] * src[col_idx[j]+1];
      d9 += value[j*132+100] * src[col_idx[j]+1];
      d10 += value[j*132+111] * src[col_idx[j]+1];
      d11 += value[j*132+122] * src[col_idx[j]+1];
      d0 += value[j*132+2] * src[col_idx[j]+2];
      d1 += value[j*132+13] * src[col_idx[j]+2];
      d2 += value[j*132+24] * src[col_idx[j]+2];
      d3 += value[j*132+35] * src[col_idx[j]+2];
      d4 += value[j*132+46] * src[col_idx[j]+2];
      d5 += value[j*132+57] * src[col_idx[j]+2];
      d6 += value[j*132+68] * src[col_idx[j]+2];
      d7 += value[j*132+79] * src[col_idx[j]+2];
      d8 += value[j*132+90] * src[col_idx[j]+2];
      d9 += value[j*132+101] * src[col_idx[j]+2];
      d10 += value[j*132+112] * src[col_idx[j]+2];
      d11 += value[j*132+123] * src[col_idx[j]+2];
      d0 += value[j*132+3] * src[col_idx[j]+3];
      d1 += value[j*132+14] * src[col_idx[j]+3];
      d2 += value[j*132+25] * src[col_idx[j]+3];
      d3 += value[j*132+36] * src[col_idx[j]+3];
      d4 += value[j*132+47] * src[col_idx[j]+3];
      d5 += value[j*132+58] * src[col_idx[j]+3];
      d6 += value[j*132+69] * src[col_idx[j]+3];
      d7 += value[j*132+80] * src[col_idx[j]+3];
      d8 += value[j*132+91] * src[col_idx[j]+3];
      d9 += value[j*132+102] * src[col_idx[j]+3];
      d10 += value[j*132+113] * src[col_idx[j]+3];
      d11 += value[j*132+124] * src[col_idx[j]+3];
      d0 += value[j*132+4] * src[col_idx[j]+4];
      d1 += value[j*132+15] * src[col_idx[j]+4];
      d2 += value[j*132+26] * src[col_idx[j]+4];
      d3 += value[j*132+37] * src[col_idx[j]+4];
      d4 += value[j*132+48] * src[col_idx[j]+4];
      d5 += value[j*132+59] * src[col_idx[j]+4];
      d6 += value[j*132+70] * src[col_idx[j]+4];
      d7 += value[j*132+81] * src[col_idx[j]+4];
      d8 += value[j*132+92] * src[col_idx[j]+4];
      d9 += value[j*132+103] * src[col_idx[j]+4];
      d10 += value[j*132+114] * src[col_idx[j]+4];
      d11 += value[j*132+125] * src[col_idx[j]+4];
      d0 += value[j*132+5] * src[col_idx[j]+5];
      d1 += value[j*132+16] * src[col_idx[j]+5];
      d2 += value[j*132+27] * src[col_idx[j]+5];
      d3 += value[j*132+38] * src[col_idx[j]+5];
      d4 += value[j*132+49] * src[col_idx[j]+5];
      d5 += value[j*132+60] * src[col_idx[j]+5];
      d6 += value[j*132+71] * src[col_idx[j]+5];
      d7 += value[j*132+82] * src[col_idx[j]+5];
      d8 += value[j*132+93] * src[col_idx[j]+5];
      d9 += value[j*132+104] * src[col_idx[j]+5];
      d10 += value[j*132+115] * src[col_idx[j]+5];
      d11 += value[j*132+126] * src[col_idx[j]+5];
      d0 += value[j*132+6] * src[col_idx[j]+6];
      d1 += value[j*132+17] * src[col_idx[j]+6];
      d2 += value[j*132+28] * src[col_idx[j]+6];
      d3 += value[j*132+39] * src[col_idx[j]+6];
      d4 += value[j*132+50] * src[col_idx[j]+6];
      d5 += value[j*132+61] * src[col_idx[j]+6];
      d6 += value[j*132+72] * src[col_idx[j]+6];
      d7 += value[j*132+83] * src[col_idx[j]+6];
      d8 += value[j*132+94] * src[col_idx[j]+6];
      d9 += value[j*132+105] * src[col_idx[j]+6];
      d10 += value[j*132+116] * src[col_idx[j]+6];
      d11 += value[j*132+127] * src[col_idx[j]+6];
      d0 += value[j*132+7] * src[col_idx[j]+7];
      d1 += value[j*132+18] * src[col_idx[j]+7];
      d2 += value[j*132+29] * src[col_idx[j]+7];
      d3 += value[j*132+40] * src[col_idx[j]+7];
      d4 += value[j*132+51] * src[col_idx[j]+7];
      d5 += value[j*132+62] * src[col_idx[j]+7];
      d6 += value[j*132+73] * src[col_idx[j]+7];
      d7 += value[j*132+84] * src[col_idx[j]+7];
      d8 += value[j*132+95] * src[col_idx[j]+7];
      d9 += value[j*132+106] * src[col_idx[j]+7];
      d10 += value[j*132+117] * src[col_idx[j]+7];
      d11 += value[j*132+128] * src[col_idx[j]+7];
      d0 += value[j*132+8] * src[col_idx[j]+8];
      d1 += value[j*132+19] * src[col_idx[j]+8];
      d2 += value[j*132+30] * src[col_idx[j]+8];
      d3 += value[j*132+41] * src[col_idx[j]+8];
      d4 += value[j*132+52] * src[col_idx[j]+8];
      d5 += value[j*132+63] * src[col_idx[j]+8];
      d6 += value[j*132+74] * src[col_idx[j]+8];
      d7 += value[j*132+85] * src[col_idx[j]+8];
      d8 += value[j*132+96] * src[col_idx[j]+8];
      d9 += value[j*132+107] * src[col_idx[j]+8];
      d10 += value[j*132+118] * src[col_idx[j]+8];
      d11 += value[j*132+129] * src[col_idx[j]+8];
      d0 += value[j*132+9] * src[col_idx[j]+9];
      d1 += value[j*132+20] * src[col_idx[j]+9];
      d2 += value[j*132+31] * src[col_idx[j]+9];
      d3 += value[j*132+42] * src[col_idx[j]+9];
      d4 += value[j*132+53] * src[col_idx[j]+9];
      d5 += value[j*132+64] * src[col_idx[j]+9];
      d6 += value[j*132+75] * src[col_idx[j]+9];
      d7 += value[j*132+86] * src[col_idx[j]+9];
      d8 += value[j*132+97] * src[col_idx[j]+9];
      d9 += value[j*132+108] * src[col_idx[j]+9];
      d10 += value[j*132+119] * src[col_idx[j]+9];
      d11 += value[j*132+130] * src[col_idx[j]+9];
      d0 += value[j*132+10] * src[col_idx[j]+10];
      d1 += value[j*132+21] * src[col_idx[j]+10];
      d2 += value[j*132+32] * src[col_idx[j]+10];
      d3 += value[j*132+43] * src[col_idx[j]+10];
      d4 += value[j*132+54] * src[col_idx[j]+10];
      d5 += value[j*132+65] * src[col_idx[j]+10];
      d6 += value[j*132+76] * src[col_idx[j]+10];
      d7 += value[j*132+87] * src[col_idx[j]+10];
      d8 += value[j*132+98] * src[col_idx[j]+10];
      d9 += value[j*132+109] * src[col_idx[j]+10];
      d10 += value[j*132+120] * src[col_idx[j]+10];
      d11 += value[j*132+131] * src[col_idx[j]+10];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
void bsmvm_12x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[])
{
  int i, j;
  
  for (i=start_row; i<=end_row; i++)
  {
    register double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
    d0 = dest[12*i+0];
    d1 = dest[12*i+1];
    d2 = dest[12*i+2];
    d3 = dest[12*i+3];
    d4 = dest[12*i+4];
    d5 = dest[12*i+5];
    d6 = dest[12*i+6];
    d7 = dest[12*i+7];
    d8 = dest[12*i+8];
    d9 = dest[12*i+9];
    d10 = dest[12*i+10];
    d11 = dest[12*i+11];
    for (j=row_start[i]; j<row_start[i+1]; j++)
    {
      d0 += value[j*144+0] * src[col_idx[j]+0];
      d1 += value[j*144+12] * src[col_idx[j]+0];
      d2 += value[j*144+24] * src[col_idx[j]+0];
      d3 += value[j*144+36] * src[col_idx[j]+0];
      d4 += value[j*144+48] * src[col_idx[j]+0];
      d5 += value[j*144+60] * src[col_idx[j]+0];
      d6 += value[j*144+72] * src[col_idx[j]+0];
      d7 += value[j*144+84] * src[col_idx[j]+0];
      d8 += value[j*144+96] * src[col_idx[j]+0];
      d9 += value[j*144+108] * src[col_idx[j]+0];
      d10 += value[j*144+120] * src[col_idx[j]+0];
      d11 += value[j*144+132] * src[col_idx[j]+0];
      d0 += value[j*144+1] * src[col_idx[j]+1];
      d1 += value[j*144+13] * src[col_idx[j]+1];
      d2 += value[j*144+25] * src[col_idx[j]+1];
      d3 += value[j*144+37] * src[col_idx[j]+1];
      d4 += value[j*144+49] * src[col_idx[j]+1];
      d5 += value[j*144+61] * src[col_idx[j]+1];
      d6 += value[j*144+73] * src[col_idx[j]+1];
      d7 += value[j*144+85] * src[col_idx[j]+1];
      d8 += value[j*144+97] * src[col_idx[j]+1];
      d9 += value[j*144+109] * src[col_idx[j]+1];
      d10 += value[j*144+121] * src[col_idx[j]+1];
      d11 += value[j*144+133] * src[col_idx[j]+1];
      d0 += value[j*144+2] * src[col_idx[j]+2];
      d1 += value[j*144+14] * src[col_idx[j]+2];
      d2 += value[j*144+26] * src[col_idx[j]+2];
      d3 += value[j*144+38] * src[col_idx[j]+2];
      d4 += value[j*144+50] * src[col_idx[j]+2];
      d5 += value[j*144+62] * src[col_idx[j]+2];
      d6 += value[j*144+74] * src[col_idx[j]+2];
      d7 += value[j*144+86] * src[col_idx[j]+2];
      d8 += value[j*144+98] * src[col_idx[j]+2];
      d9 += value[j*144+110] * src[col_idx[j]+2];
      d10 += value[j*144+122] * src[col_idx[j]+2];
      d11 += value[j*144+134] * src[col_idx[j]+2];
      d0 += value[j*144+3] * src[col_idx[j]+3];
      d1 += value[j*144+15] * src[col_idx[j]+3];
      d2 += value[j*144+27] * src[col_idx[j]+3];
      d3 += value[j*144+39] * src[col_idx[j]+3];
      d4 += value[j*144+51] * src[col_idx[j]+3];
      d5 += value[j*144+63] * src[col_idx[j]+3];
      d6 += value[j*144+75] * src[col_idx[j]+3];
      d7 += value[j*144+87] * src[col_idx[j]+3];
      d8 += value[j*144+99] * src[col_idx[j]+3];
      d9 += value[j*144+111] * src[col_idx[j]+3];
      d10 += value[j*144+123] * src[col_idx[j]+3];
      d11 += value[j*144+135] * src[col_idx[j]+3];
      d0 += value[j*144+4] * src[col_idx[j]+4];
      d1 += value[j*144+16] * src[col_idx[j]+4];
      d2 += value[j*144+28] * src[col_idx[j]+4];
      d3 += value[j*144+40] * src[col_idx[j]+4];
      d4 += value[j*144+52] * src[col_idx[j]+4];
      d5 += value[j*144+64] * src[col_idx[j]+4];
      d6 += value[j*144+76] * src[col_idx[j]+4];
      d7 += value[j*144+88] * src[col_idx[j]+4];
      d8 += value[j*144+100] * src[col_idx[j]+4];
      d9 += value[j*144+112] * src[col_idx[j]+4];
      d10 += value[j*144+124] * src[col_idx[j]+4];
      d11 += value[j*144+136] * src[col_idx[j]+4];
      d0 += value[j*144+5] * src[col_idx[j]+5];
      d1 += value[j*144+17] * src[col_idx[j]+5];
      d2 += value[j*144+29] * src[col_idx[j]+5];
      d3 += value[j*144+41] * src[col_idx[j]+5];
      d4 += value[j*144+53] * src[col_idx[j]+5];
      d5 += value[j*144+65] * src[col_idx[j]+5];
      d6 += value[j*144+77] * src[col_idx[j]+5];
      d7 += value[j*144+89] * src[col_idx[j]+5];
      d8 += value[j*144+101] * src[col_idx[j]+5];
      d9 += value[j*144+113] * src[col_idx[j]+5];
      d10 += value[j*144+125] * src[col_idx[j]+5];
      d11 += value[j*144+137] * src[col_idx[j]+5];
      d0 += value[j*144+6] * src[col_idx[j]+6];
      d1 += value[j*144+18] * src[col_idx[j]+6];
      d2 += value[j*144+30] * src[col_idx[j]+6];
      d3 += value[j*144+42] * src[col_idx[j]+6];
      d4 += value[j*144+54] * src[col_idx[j]+6];
      d5 += value[j*144+66] * src[col_idx[j]+6];
      d6 += value[j*144+78] * src[col_idx[j]+6];
      d7 += value[j*144+90] * src[col_idx[j]+6];
      d8 += value[j*144+102] * src[col_idx[j]+6];
      d9 += value[j*144+114] * src[col_idx[j]+6];
      d10 += value[j*144+126] * src[col_idx[j]+6];
      d11 += value[j*144+138] * src[col_idx[j]+6];
      d0 += value[j*144+7] * src[col_idx[j]+7];
      d1 += value[j*144+19] * src[col_idx[j]+7];
      d2 += value[j*144+31] * src[col_idx[j]+7];
      d3 += value[j*144+43] * src[col_idx[j]+7];
      d4 += value[j*144+55] * src[col_idx[j]+7];
      d5 += value[j*144+67] * src[col_idx[j]+7];
      d6 += value[j*144+79] * src[col_idx[j]+7];
      d7 += value[j*144+91] * src[col_idx[j]+7];
      d8 += value[j*144+103] * src[col_idx[j]+7];
      d9 += value[j*144+115] * src[col_idx[j]+7];
      d10 += value[j*144+127] * src[col_idx[j]+7];
      d11 += value[j*144+139] * src[col_idx[j]+7];
      d0 += value[j*144+8] * src[col_idx[j]+8];
      d1 += value[j*144+20] * src[col_idx[j]+8];
      d2 += value[j*144+32] * src[col_idx[j]+8];
      d3 += value[j*144+44] * src[col_idx[j]+8];
      d4 += value[j*144+56] * src[col_idx[j]+8];
      d5 += value[j*144+68] * src[col_idx[j]+8];
      d6 += value[j*144+80] * src[col_idx[j]+8];
      d7 += value[j*144+92] * src[col_idx[j]+8];
      d8 += value[j*144+104] * src[col_idx[j]+8];
      d9 += value[j*144+116] * src[col_idx[j]+8];
      d10 += value[j*144+128] * src[col_idx[j]+8];
      d11 += value[j*144+140] * src[col_idx[j]+8];
      d0 += value[j*144+9] * src[col_idx[j]+9];
      d1 += value[j*144+21] * src[col_idx[j]+9];
      d2 += value[j*144+33] * src[col_idx[j]+9];
      d3 += value[j*144+45] * src[col_idx[j]+9];
      d4 += value[j*144+57] * src[col_idx[j]+9];
      d5 += value[j*144+69] * src[col_idx[j]+9];
      d6 += value[j*144+81] * src[col_idx[j]+9];
      d7 += value[j*144+93] * src[col_idx[j]+9];
      d8 += value[j*144+105] * src[col_idx[j]+9];
      d9 += value[j*144+117] * src[col_idx[j]+9];
      d10 += value[j*144+129] * src[col_idx[j]+9];
      d11 += value[j*144+141] * src[col_idx[j]+9];
      d0 += value[j*144+10] * src[col_idx[j]+10];
      d1 += value[j*144+22] * src[col_idx[j]+10];
      d2 += value[j*144+34] * src[col_idx[j]+10];
      d3 += value[j*144+46] * src[col_idx[j]+10];
      d4 += value[j*144+58] * src[col_idx[j]+10];
      d5 += value[j*144+70] * src[col_idx[j]+10];
      d6 += value[j*144+82] * src[col_idx[j]+10];
      d7 += value[j*144+94] * src[col_idx[j]+10];
      d8 += value[j*144+106] * src[col_idx[j]+10];
      d9 += value[j*144+118] * src[col_idx[j]+10];
      d10 += value[j*144+130] * src[col_idx[j]+10];
      d11 += value[j*144+142] * src[col_idx[j]+10];
      d0 += value[j*144+11] * src[col_idx[j]+11];
      d1 += value[j*144+23] * src[col_idx[j]+11];
      d2 += value[j*144+35] * src[col_idx[j]+11];
      d3 += value[j*144+47] * src[col_idx[j]+11];
      d4 += value[j*144+59] * src[col_idx[j]+11];
      d5 += value[j*144+71] * src[col_idx[j]+11];
      d6 += value[j*144+83] * src[col_idx[j]+11];
      d7 += value[j*144+95] * src[col_idx[j]+11];
      d8 += value[j*144+107] * src[col_idx[j]+11];
      d9 += value[j*144+119] * src[col_idx[j]+11];
      d10 += value[j*144+131] * src[col_idx[j]+11];
      d11 += value[j*144+143] * src[col_idx[j]+11];
    }
    dest[12*i+0] = d0;
    dest[12*i+1] = d1;
    dest[12*i+2] = d2;
    dest[12*i+3] = d3;
    dest[12*i+4] = d4;
    dest[12*i+5] = d5;
    dest[12*i+6] = d6;
    dest[12*i+7] = d7;
    dest[12*i+8] = d8;
    dest[12*i+9] = d9;
    dest[12*i+10] = d10;
    dest[12*i+11] = d11;
  }
}
