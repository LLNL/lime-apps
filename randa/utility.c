/* -*- mode: C; tab-width: 2; indent-tabs-mode: nil; -*-
 *
 * This file provides utility functions for the RandomAccess benchmark suite.
 */

#include "hpcc.h"
#include "RandomAccess.h"


/* Utility routine to start LCG random number generator at Nth step */
u64Int
HPCC_starts_LCG(s64Int n)
{
  u64Int mul_k, add_k, ran, un;

  mul_k = LCG_MUL64;
  add_k = LCG_ADD64;

  ran = 1;
  for (un = (u64Int)n; un; un >>= 1) {
    if (un & 1)
      ran = mul_k * ran + add_k;
    add_k *= (mul_k + 1);
    mul_k *= mul_k;
  }

  return ran;
}
