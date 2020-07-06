/*
 * dgemm.c
 *
 *  Created on: Apr 15, 2015
 *      Author: lloyd23
 */

#include <unistd.h> // getopt, optarg
#ifdef __cplusplus
extern "C" {
#endif
#include "hpcc.h"
#ifdef __cplusplus
}
#endif

// Example main arguments
// #define MARGS "-s20"

#include "lime.h"

#define DEFAULT_SCALE 20U // log 2 size

unsigned scale = DEFAULT_SCALE;


int MAIN(int argc, char *argv[])
{
	HPCC_Params params;
	/* * * * * * * * * * get arguments beg * * * * * * * * * */
	int opt;
	bool nok = false;

	while ((opt = getopt(argc, argv, "s:")) != -1) {
		switch (opt) {
		case 's':
			scale = atoi(optarg);
			break;
		default: /* '?' */
			nok = true;
		}
	}
	if (nok) {
		fprintf(stderr, "Usage: dgemm -s<int>\n");
		fprintf(stderr, "  -s  scale 2^n, default: n=%d\n", DEFAULT_SCALE);
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
	printf("scale: %u\n", scale);

	/* * * * * * * * * * get arguments end * * * * * * * * * */

	params.outFname[0] = '\0'; /* use stdout */
	params.HPLMaxProcMem = (size_t)1 << scale;
	params.RunSingleDGEMM = 1;

	/* -------------------------------------------------- */
	/*                    SingleDGEMM                     */
	/* -------------------------------------------------- */

	if (params.RunSingleDGEMM) {
		printf("Begin of SingleDGEMM section.\n");
		HPCC_TestDGEMM(&params, 1, &params.SingleDGEMMGflops, &params.DGEMM_N, &params.Failure);
		if (params.Failure) printf(" -- DGEMM Failure\n");
		printf("End of SingleDGEMM section.\n");
	}

	TRACE_CAP
	return EXIT_SUCCESS;
}
