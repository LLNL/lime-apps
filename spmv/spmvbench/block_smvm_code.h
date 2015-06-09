#ifndef _block_smvm_code_h
#define _block_smvm_code_h
/**
 * @file block_smvm_code.h
 * @date Time-stamp: <2004-03-24 08:24:35 mhoemmen>
 *
 * @brief Auto-generated (BSR sparse matrix)*(dense vector) mult routines.
 *
 * Automatically generated matrix - vector multiplication routines for BSR 
 * sparse matrices and dense vectors.  All (r x c) register block sizes for
 * 1 <= r,c <= 12 are supported.  This file comes from the original Sparsity
 * code, with modifications; in particular, `const' access specifiers were
 * added (by mfh). 
 ****************************************************************************/

typedef void (*SMVM_FP)(const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
extern SMVM_FP bsmvm_routines[12][12][1];


void bsmvm_1x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_1x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_2x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_3x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_4x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_5x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_6x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_7x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_8x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_9x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_10x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_11x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x1_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x2_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x3_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x4_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x5_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x6_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x7_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x8_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x9_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x10_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x11_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);
void bsmvm_12x12_1 (const int start_row, const int end_row, const int bm,
               const int row_start[], const int col_idx[], const double value[], 
               const double src[], double dest[]);

#endif /* NOT _block_smvm_code_h */
