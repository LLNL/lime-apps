/*
 * randa.c
 *
 *  Created on: Sep 12, 2014
 *      Author: lloyd23
 */

#include "hpcc.h"
#include "RandomAccess.h"

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

#if defined(USE_ACC)
#include "IndexArray.hpp"
IndexArray<index_t> dre; // Data Reorganization Engine
#endif // USE_ACC


int main()
{
	HPCC_Params params;

	MONITOR_INIT
#if defined(USE_ACC)
	dre.wait(); // wait for DRE initialization
#if 0
	/* test communication performance between Host and DRE (uncomment wait) */
	{
		register int i;
		tick_t t0, t1;

		CLOCKS_EMULATE
		tget(t0);
		for (i = 0; i < 1000; i++) {dre.wait();}
		tget(t1);
		CLOCKS_NORMAL
		printf("DRE wait time: %llu ticks\n", tdiff(t1,t0)/1000);
		printf("DRE wait time: %.3f usec\n", tesec(t1,t0)*1000);
		return 0;
	}
#endif
#endif // USE_ACC

	params.outFname[0] = '\0'; /* use stdout */
	params.HPLMaxProcMem = (size_t)1 << 29; /* half-gig */
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
	return 0;
}
