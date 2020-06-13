
#include <stdio.h>
#include <unistd.h> // getopt, optarg
#include <string.h> // strncmp

#ifdef __cplusplus
extern "C" {
#endif

#include <smvm_benchmark.h>
#include <smvm_timing_results.h>
#include <smvm_util.h>

#ifdef __cplusplus
}
#endif

// Example main arguments
// #define MARGS "-c -s18 -n34 -v15" *
// #define MARGS "-c -s21 -n34 -v15"

#include "lime.h"

#define DEFAULT_BLOCK_LSZ 15 // log 2 size
#define DEFAULT_MATRIX_LSZ 21 // log 2 size
#define DEFAULT_NNZ 34 // number of non-zeros per row

unsigned block_sz = 1U<<DEFAULT_BLOCK_LSZ;

// TODO: find a better place for these globals

#if defined(USE_ACC)
#include "IndexArray.hpp"
typedef int index_t; // int used in bcsr_matrix_t
IndexArray<index_t> dre; // Data Reorganization Engine
#endif


int MAIN(int argc, char *argv[])
{
	/* * * * * * * * * * get arguments beg * * * * * * * * * */
	int opt;
	bool nok = false;
	bool cflag = false;
	double ifrac[] = {0.659,0.114,0.0584,0.0684,0.0285,0.0186,0.0144,0.0271,0.00774,0.00386}; // += 1
	struct SMVM_timing_results timing_results;
	struct SMVM_parameters spmv_params;
	FILE *fout = stdout;

	//smvm_set_debug_level_from_environment();
	spmv_params.m = 1 << DEFAULT_MATRIX_LSZ;
	spmv_params.n = 1 << DEFAULT_MATRIX_LSZ;
	spmv_params.percent_fill = -DEFAULT_NNZ;
	spmv_params.interval_fracs = ifrac;
	while ((opt = getopt(argc, argv, "cd:f:n:i:s:v:")) != -1) {
		switch (opt) {
		case 'c':
			cflag = true;
			break;
		case 'd':
			smvm_set_debug_level(atoi(optarg));
			break;
		case 'f':
			spmv_params.percent_fill = atof(optarg);
			break;
		case 'n':
			spmv_params.percent_fill = -atoi(optarg);
			break;
		case 'i':
			if (strncmp(optarg, "null", 4) == 0) {
				spmv_params.interval_fracs = NULL;
			} else if (sscanf(optarg, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
				ifrac+0,ifrac+1,ifrac+2,ifrac+3,ifrac+4,
				ifrac+5,ifrac+6,ifrac+7,ifrac+8,ifrac+9) != 10) {
				nok = true;
			}
			break;
		case 's':
			spmv_params.m = (1 << atoi(optarg)); // rows
			spmv_params.n = (1 << atoi(optarg)); // cols
			break;
		case 'v':
			block_sz = (1U << atoi(optarg));
			if (block_sz < 8) {
				fprintf(stderr, "view buffer block size must be 8 or greater.\n");
				nok = true;
			}
			break;
		default: /* '?' */
			nok = true;
		}
	}
	if (nok) {
		fprintf(stderr, "Usage: spmv -c -d<int> -f<float> -n<int> -i[null|<float>,...] -s<int> -v<int>\n");
		fprintf(stderr, "  -c  check result\n");
		fprintf(stderr, "  -d  debug level\n");
		fprintf(stderr, "  -f  percent fill\n");
		fprintf(stderr, "  -n  non-zeros per row, default: n=%d\n", DEFAULT_NNZ);
		fprintf(stderr, "  -i  interval fractions, null or 10 <float>,...\n");
		fprintf(stderr, "  -s  matrix size 2^n, default: n=%d\n", DEFAULT_MATRIX_LSZ);
		fprintf(stderr, "  -v  view buffer block size 2^n, default: n=%d\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
#ifdef USE_SP
	if (block_sz > SP_SIZE) block_sz = SP_SIZE;
#endif

	/* * * * * * * * * * get arguments end * * * * * * * * * */

	spmv_params.r = 1;
	spmv_params.c = 1;
	spmv_params.num_trials = 1;
	spmv_params.dataoutfile = fout; /* not used */

	smvm_benchmark_with_results(&spmv_params, &timing_results, cflag);

//	fprintf(fout, "Mean time:          %.6f sec\n", timing_results.t_median); /* actually mean time */
//	fprintf(fout, "Min time:           %.6f sec\n", timing_results.t_min);
//	fprintf(fout, "Max time:           %.6f sec\n", timing_results.t_max);
	fprintf(fout, "Matrix Dim (r x c): (%d x %d)\n", timing_results.m, timing_results.n);
	fprintf(fout, "Block Dim (r x c):  (%d x %d)\n", timing_results.r, timing_results.c);
	fprintf(fout, "Non-zero blocks:    %d\n", timing_results.nnzb);
	fprintf(fout, "Repetitions:        %d\n", timing_results.num_trials);
	fprintf(fout, "Mflop/s:            %.6f\n", timing_results.mflops/timing_results.t_median);
	fprintf(fout, "num_loads:          %ld\n", timing_results.num_loads);
	fprintf(fout, "num_stores:         %ld\n", timing_results.num_stores);

	TRACE_CAP
	return 0;
}
