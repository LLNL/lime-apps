
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <smvm_benchmark.h>
#include <smvm_timing_results.h>
#include <smvm_util.h>

#ifdef __cplusplus
}
#endif

#include "config.h"
#include "alloc.h"
#include "cache.h"
#include "monitor.h"
#include "IndexArray.hpp"
#include "ticks.h"
#include "clocks.h"

typedef int index_t; // used in bcsr_matrix_t

IndexArray<index_t> dre; // Data Reorganization Engine

// TODO: find a better place for these globals

#if defined(STATS) || defined(TRACE) 
XAxiPmon apm;
#endif // STATS || TRACE


int main(int argc, char **argv) {

	struct SMVM_timing_results timing_results;
	struct SMVM_parameters spmv_params;
	double interval_fracs[] = {0.659,0.114,0.0584,0.0684,0.0285,0.0186,0.0144,0.0271,0.00774,0.00387};
	FILE *fout = stdout;

	MONITOR_INIT
	dre.wait(); // wait for DRE initialization
	//smvm_set_debug_level_from_environment();
	//smvm_set_debug_level(1);

#if 1
	spmv_params.m = 1 << 21;
	spmv_params.n = 1 << 21;
	spmv_params.percent_fill = -34;
	spmv_params.interval_fracs = interval_fracs;
#endif
#if 0
	spmv_params.m = 1 << 4;
	spmv_params.n = 1 << 4;
	spmv_params.percent_fill = -4;
	spmv_params.interval_fracs = NULL;
#endif
	spmv_params.r = 1;
	spmv_params.c = 1;
	spmv_params.num_trials = 1;
	spmv_params.dataoutfile = fout; /* not used */

	smvm_benchmark_with_results(&spmv_params, &timing_results);

//	fprintf(fout, "Mean time:          %.6f sec\n", timing_results.t_median); /* actually mean time */
//	fprintf(fout, "Min time:           %.6f sec\n", timing_results.t_min);
//	fprintf(fout, "Max time:           %.6f sec\n", timing_results.t_max);
	fprintf(fout, "Matrix Dim (r x c): (%d x %d)\n", timing_results.m, timing_results.n);
	fprintf(fout, "Block Dim (r x c):  (%d x %d)\n", timing_results.r, timing_results.c);
	fprintf(fout, "Non-zero blocks:    %d\n", timing_results.nnzb);
	fprintf(fout, "Repetitions:        %d\n", timing_results.num_trials);
	fprintf(fout, "Mflop/s:            %.6f\n", timing_results.mflops/timing_results.t_median);
	fprintf(fout, "num_loads:          %d\n", timing_results.num_loads);
	fprintf(fout, "num_stores:         %d\n", timing_results.num_stores);

	TRACE_CAP
	return 0;
}
