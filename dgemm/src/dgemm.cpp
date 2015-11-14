/*
 * dgemm.c
 *
 *  Created on: Apr 15, 2015
 *      Author: lloyd23
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "hpcc.h"
#ifdef __cplusplus
}
#endif

#include "config.h"
#include "alloc.h"
#include "cache.h"
#include "monitor.h"
#include "ticks.h"
#include "clocks.h"

// TODO: find a better place for these globals

#if defined(STATS) || defined(TRACE) 
XAxiPmon apm;
#endif // STATS || TRACE


int main()
{
	HPCC_Params params;

	MONITOR_INIT

	//for (unsigned i = 18; i <= 24; i++) {printf("\nMax Mem: 2^%u\n", i);
	params.outFname[0] = '\0'; /* use stdout */
	params.HPLMaxProcMem = (size_t)1 << 20; /* one meg */
	params.RunSingleDGEMM = 1;

	/* -------------------------------------------------- */
	/*                    SingleDGEMM                     */
	/* -------------------------------------------------- */

	if (params.RunSingleDGEMM) {
		printf("Begin of SingleDGEMM section.\n");
		HPCC_TestDGEMM(&params, 1, &params.SingleDGEMMGflops, &params.DGEMM_N, &params.Failure);
		printf("End of SingleDGEMM section.\n");
	}
	//}

	TRACE_CAP
	return 0;
}
