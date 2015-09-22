/**
 * @file smvm_timing_results.c
 * @author mfh
 * @date Time-stamp: <2004-03-24 19:43:34 mhoemmen>
 *****************************************************************************/
#include "smvm_timing_results.h"
#include <smvm_util.h>


/************************************************************************/
int
compare_timing_results_by_t_min (const void* r1, const void* r2)
{
  const double r1_t_min = ((struct SMVM_timing_results*) r1)->t_min;
  const double r2_t_min = ((struct SMVM_timing_results*) r2)->t_min;

  if (r1_t_min < r2_t_min) return -1;
  if (r1_t_min > r2_t_min) return 1;
  return 0;
}


/************************************************************************/
int
compare_timing_results_by_t_median (const void* r1, const void* r2)
{
  const double r1_t_median = ((struct SMVM_timing_results*) r1)->t_median;
  const double r2_t_median = ((struct SMVM_timing_results*) r2)->t_median;

  if (r1_t_median < r2_t_median) return -1;
  if (r1_t_median > r2_t_median) return 1;
  return 0;
}


/*****************************************************************************/
void
smvm_save_timing_results (struct SMVM_timing_results* p_results, 
			  const int m, const int n, const int r, const int c, 
			  const int nnzb, const int num_trials, 
			  const double t_median, const double t_min, 
			  const double t_max, const double mflops, 
			  const int num_loads, const int num_stores)
{
  if (p_results == NULL)
    {
      die_with_message ("*** ERROR ***\nsmvm_save_timing_results: "
			"p_results is NULL!\n", -1);
    }
  p_results->m = m;
  p_results->n = n;
  p_results->r = r;
  p_results->c = c;
  p_results->nnzb = nnzb;
  p_results->num_trials = num_trials;
  p_results->t_median = t_median;
  p_results->t_min = t_min;
  p_results->t_max = t_max;
  p_results->mflops = mflops;
  p_results->num_loads = num_loads;
  p_results->num_stores = num_stores;
}
