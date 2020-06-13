/*
 * randa.c
 *
 *  Created on: Sep 12, 2014
 *      Author: lloyd23
 */

#include <unistd.h> // getopt, optarg
#if defined(_OPENMP)
#include <omp.h>
#endif
#include "hpcc.h"
#include "RandomAccess.h"

// Example main arguments
// #define MARGS "-s29" *

#include "lime.h"

#define DEFAULT_SCALE 29U // log 2 size
#define DEFAULT_VECTOR_LSZ 10U // log 2 size

unsigned scale = DEFAULT_SCALE;
unsigned vect_len = 1U<<DEFAULT_VECTOR_LSZ;

// TODO: find a better place for these globals

#if defined(USE_ACC)
#include "IndexArray.hpp"
IndexArray<index_t> dre; // Data Reorganization Engine
#endif // USE_ACC


int MAIN(int argc, char *argv[])
{
	HPCC_Params params;
	/* * * * * * * * * * get arguments beg * * * * * * * * * */
	int opt;
	bool nok = false;

#if defined(USE_ACC)
	dre.wait(); // wait for DRE initialization
#endif // USE_ACC
	while ((opt = getopt(argc, argv, "s:v:")) != -1) {
		switch (opt) {
		case 's':
			scale = atoi(optarg);
			break;
		case 'v':
			vect_len = (1U << atoi(optarg));
			if (vect_len < 8) {
				fprintf(stderr, "vector length must be 8 or greater.\n");
				nok = true;
			}
			break;
		default: /* '?' */
			nok = true;
		}
	}
	if (nok) {
		fprintf(stderr, "Usage: randa -s<int> -v<int>\n");
		fprintf(stderr, "  -s  table scale 2^n, default: n=%d\n", DEFAULT_SCALE);
		// not used, currently a constant specified by VECT_LEN in RandomAccess.h
		// fprintf(stderr, "  -v  vector length 2^n, default: n=%d\n", DEFAULT_VECTOR_LSZ);
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
#if defined(_OPENMP)
	// to control the number of threads use: export OMP_NUM_THREADS=N
	printf("threads: %d\n", omp_get_max_threads());
#endif
	printf("table scale: %u\n", scale);
	printf("vector length: %u\n", VECT_LEN);

	/* * * * * * * * * * get arguments end * * * * * * * * * */

	params.outFname[0] = '\0'; /* use stdout */
	params.HPLMaxProcMem = (size_t)1 << scale;
	params.RunSingleRandomAccess_LCG = 1;

	/* -------------------------------------------------- */
	/*                 SingleRandomAccess LCG             */
	/* -------------------------------------------------- */

	if (params.RunSingleRandomAccess_LCG) {
		printf("Begin of SingleRandomAccess_LCG section.\n");
		HPCC_RandomAccess_LCG(&params, 1, &params.Single_LCG_GUPs, &params.Failure);
		printf("End of SingleRandomAccess_LCG section.\n");
	}

	TRACE_CAP
	return EXIT_SUCCESS;
}
