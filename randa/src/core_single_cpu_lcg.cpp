/* -*- mode: C; tab-width: 2; indent-tabs-mode: nil; -*- */

/*
 * This code has been contributed by the DARPA HPCS program.  Contact
 * David Koester <dkoester@mitre.org> or Bob Lucas <rflucas@isi.edu>
 * if you have questions.
 *
 * GUPS (Giga UPdates per Second) is a measurement that profiles the memory
 * architecture of a system and is a measure of performance similar to MFLOPS.
 * The HPCS HPCchallenge RandomAccess benchmark is intended to exercise the
 * GUPS capability of a system, much like the LINPACK benchmark is intended to
 * exercise the MFLOPS capability of a computer.  In each case, we would
 * expect these benchmarks to achieve close to the "peak" capability of the
 * memory system. The extent of the similarities between RandomAccess and
 * LINPACK are limited to both benchmarks attempting to calculate a peak system
 * capability.
 *
 * GUPS is calculated by identifying the number of memory locations that can be
 * randomly updated in one second, divided by 1 billion (1e9). The term "randomly"
 * means that there is little relationship between one address to be updated and
 * the next, except that they occur in the space of one half the total system
 * memory.  An update is a read-modify-write operation on a table of 64-bit words.
 * An address is generated, the value at that address read from memory, modified
 * by an integer operation (add, and, or, xor) with a literal value, and that
 * new value is written back to memory.
 *
 * We are interested in knowing the GUPS performance of both entire systems and
 * system subcomponents --- e.g., the GUPS rating of a distributed memory
 * multiprocessor the GUPS rating of an SMP node, and the GUPS rating of a
 * single processor.  While there is typically a scaling of FLOPS with processor
 * count, a similar phenomenon may not always occur for GUPS.
 *
 * For additional information on the GUPS metric, the HPCchallenge RandomAccess
 * Benchmark,and the rules to run RandomAccess or modify it to optimize
 * performance -- see http://icl.cs.utk.edu/hpcc/
 *
 */

/*
 * This file contains the computational core of the single cpu version
 * of GUPS.  The inner loop should easily be vectorized by compilers
 * with such support.
 *
 * This core is used by both the single_cpu and star_single_cpu tests.
 */

#include "hpcc.h"
#include "RandomAccess.h"

#include "lime.h"

typedef index_t* index_p;
typedef table_t* table_p;
typedef uran_t*  uran_p;

tick_t start, finish;
tick_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
unsigned long long tsetup, treorg, toper, tcache;


#if defined(USE_ACC)

#include "IndexArray.hpp"

#define restrict __restrict__

extern IndexArray<index_t> dre; // Data Reorganization Engine

/*
   For LCG:
   table entries: 2^26 = 67,108,864
   updates: 268,435,456
   block length:  128 duplicates:   262 errors:   262 (passed)
   block length: 1024 duplicates: 1,976 errors: 1,976 (passed)
*/

#ifdef COUNT_DUP
static size_t duplicates = 0;

static int
cmpp(const void *p1, const void *p2)
{
	index_t i1 = *(index_t *)p1;
	index_t i2 = *(index_t *)p2;

	if (i1 == i2) return 0;
	else if (i1 < i2) return -1;
	else return 1;
}
#endif

static void
RandomAccessUpdate_LCG(unsigned int logTableSize, size_t TableSize, table_p Table, nupdate_t nupdate)
{
	nupdate_t i;
	size_t j;
#ifdef RAND_SERIAL
	uran_t temp;
#endif
	unsigned block_sz = VECT_LEN*sizeof(table_t);
	/* TODO: check for SP_SIZE overflow */
	table_p restrict block = SP_NALLOC(table_t, VECT_LEN); /* view block */
	index_p restrict tidx = SP_NALLOC(index_t, VECT_LEN); /* current table index */
	uran_t ran[VECT_LEN]; /* current random numbers */

	if (tidx == NULL) {printf(" -- error: allocating tidx\n"); return;}
	//printf("Table:%p tidx:%p block:%p\n", Table, tidx, block); fflush(stdout);
	tget(t0);
	// receive block, not needed when memory has been invalidated before entry.
	// Xil_L1DCacheInvalidateRange((INTPTR)block, block_sz);
	tget(t1);
	/* Set up view of Table */
	dre.setup(Table, sizeof(table_t), tidx, VECT_LEN);
	dre.wait();
	tget(t2);
#ifdef RAND_SERIAL
	temp = 1;
#else
	for (j=0; j<VECT_LEN; j++)
		ran[j] = HPCC_starts_LCG((nupdate/VECT_LEN) * j);
#endif

	for (i=0; i<nupdate/VECT_LEN; i++) {
		// if (i % 1000 == 0) outbyte('.');
		/* The good, the bad, and the ugly:
		 * Good: inserting cache management within the loop is faster
		 * Bad:  cache management instructions are often privileged
		 * Ugly: loop unrolling is dependent on the size of a cache line and an array element
		 */
		// TODO: test if needed? mtcp(XREG_CP15_CACHE_SIZE_SEL, 0); /* Select cache L0 D-cache in CSSR */
		for (j=0; j<VECT_LEN;
#if defined(__aarch64__)
			j+=16
#else
			j+=8
#endif
		) {
			tidx[j  ] = (ran[j  ] = LCG_MUL64 * ran[j  ] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+1] = (ran[j+1] = LCG_MUL64 * ran[j+1] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+2] = (ran[j+2] = LCG_MUL64 * ran[j+2] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+3] = (ran[j+3] = LCG_MUL64 * ran[j+3] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+4] = (ran[j+4] = LCG_MUL64 * ran[j+4] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+5] = (ran[j+5] = LCG_MUL64 * ran[j+5] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+6] = (ran[j+6] = LCG_MUL64 * ran[j+6] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+7] = (ran[j+7] = LCG_MUL64 * ran[j+7] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
#if defined(__aarch64__)
			tidx[j+ 8] = (ran[j+ 8] = LCG_MUL64 * ran[j+ 8] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+ 9] = (ran[j+ 9] = LCG_MUL64 * ran[j+ 9] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+10] = (ran[j+10] = LCG_MUL64 * ran[j+10] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+11] = (ran[j+11] = LCG_MUL64 * ran[j+11] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+12] = (ran[j+12] = LCG_MUL64 * ran[j+12] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+13] = (ran[j+13] = LCG_MUL64 * ran[j+13] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+14] = (ran[j+14] = LCG_MUL64 * ran[j+14] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
			tidx[j+15] = (ran[j+15] = LCG_MUL64 * ran[j+15] + LCG_ADD64) >> (sizeof(uran_t)*8 - logTableSize);
#endif
			dc_CVAC(&tidx[j]);
		}
		dsb(); /* Wait for cache operation to complete */
#if 0
		for (j=0; j<VECT_LEN; j++) {
#ifdef RAND_SERIAL
			ran[j] = temp = LCG_MUL64 * temp + LCG_ADD64;
#else
			ran[j] = LCG_MUL64 * ran[j] + LCG_ADD64;
#endif
			tidx[j] = ran[j] >> (sizeof(uran_t)*8 - logTableSize);
		}
#endif
#ifdef COUNT_DUP
		qsort(tidx, VECT_LEN, sizeof(index_t), cmpp);
		for (j=0; j<VECT_LEN-1; j++) {
			if (tidx[j] == tidx[j+1]) duplicates++;
		}
#endif
		// tget(t3);
		// send tidx
		// Xil_L1DCacheFlushRange((INTPTR)tidx, VECT_LEN*sizeof(index_t)); // done in tidx loop
		// dre.cache_invalidate(tidx, VECT_LEN*sizeof(index_t)); // done on MB in drain loop
		tget(t4);
		dre.fill(block, block_sz, 0); /* fill block -- gather */
		dre.wait();
		tget(t5);
		// TODO: test if needed? mtcp(XREG_CP15_CACHE_SIZE_SEL, 0); /* Select cache L0 D-cache in CSSR */
		for (j=0; j<VECT_LEN;
#if defined(__aarch64__)
			j+=8
#else
			j+=4
#endif
		) {
			block[j  ] ^= ran[j  ];
			block[j+1] ^= ran[j+1];
			block[j+2] ^= ran[j+2];
			block[j+3] ^= ran[j+3];
#if defined(__aarch64__)
			block[j+4] ^= ran[j+4];
			block[j+5] ^= ran[j+5];
			block[j+6] ^= ran[j+6];
			block[j+7] ^= ran[j+7];
#endif
			dc_CIVAC(&block[j]);
		}
		dsb(); /* Wait for cache operation to complete */
		// tget(t6);
		// send & receive block
		// Xil_L1DCacheFlushRange((INTPTR)block, block_sz); /* this also invalidates */ // done in block loop
		tget(t7);
		dre.drain(block, block_sz, 0); /* drain block -- scatter */
		dre.wait();
		tget(t8);
		// tinc(tcache, tdiff(t4,t3) + tdiff(t7,t6));
		tinc(treorg, tdiff(t5,t4) + tdiff(t8,t7));
	}
	tget(t9);
	// receive Table
	host::cache_flush_invalidate(); /* invalidate all since Table size is half of memory */
	// make sure to invalidate cache before delete
	// CACHE_DISPOSE(dre, tidx, VECT_LEN*sizeof(index_t)) // done in prior line
	// CACHE_DISPOSE(dre, block, block_sz)
	tget(t10);
	SP_NFREE((index_t*)tidx);
	SP_NFREE((table_t*)block);
#ifdef COUNT_DUP
	printf("duplicates: %u\n", duplicates);
#endif
	tinc(tcache, tdiff(t1,t0) + tdiff(t10,t9));
	tinc(tsetup, tdiff(t2,t1));
}

#else /* USE_ACC */

#ifdef VECTOR

static void
RandomAccessUpdate_LCG(unsigned int logTableSize, size_t TableSize, table_p Table, nupdate_t nupdate)
{
	nupdate_t i;
	size_t j;
	uran_t ran[VECT_LEN]; /* current random numbers */

	tget(t2);
#if defined(_OPENMP)
	#pragma omp parallel for
#endif
	for (j=0; j<VECT_LEN; j++)
		ran[j] = HPCC_starts_LCG((nupdate/VECT_LEN) * j);

	for (i=0; i<nupdate/VECT_LEN; i++) {
#if defined(_OPENMP)
		#pragma omp parallel for
#endif
		for (j=0; j<VECT_LEN; j++) {
			ran[j] = LCG_MUL64 * ran[j] + LCG_ADD64;
			Table[ran[j] >> (sizeof(uran_t)*8 - logTableSize)] ^= ran[j];
		}
	}
	tget(t3);
	host::cache_flush(); /* flush all since Table size is half of memory */
	tget(t4);
	toper += tdiff(t3,t2);
	tcache += tdiff(t4,t3);
}

#else /* VECTOR */

/* Perform updates to main table -- scalar version */
static void
RandomAccessUpdate_LCG(unsigned int logTableSize, size_t TableSize, table_p Table, nupdate_t nupdate)
{
	nupdate_t i;
	uran_t ran;
	ran = 1;
	for (i=0; i<nupdate; i++) {
		ran = LCG_MUL64 * ran + LCG_ADD64;
		Table[ran >> (sizeof(uran_t)*8 - logTableSize)] ^= ran;
	}
}

#endif /* VECTOR */
#endif /* USE_ACC */

int
HPCC_RandomAccess_LCG(HPCC_Params *params, int doIO, double *GUPs, int *failure)
{
	nupdate_t nupdate;
	size_t t;
//	double cputime;               /* CPU time to update table */
	double realtime;              /* Real time to update table */
	double totalMem;
	table_p Table;
	size_t TableSize;
	unsigned int logTableSize;
	FILE *outFile = NULL;

	if (doIO) {
		if (params->outFname[0] == '\0') outFile = stdout;
		else outFile = fopen(params->outFname, "a");
		if (!outFile) {
			outFile = stderr;
			fprintf(outFile, "Cannot open output file.\n");
			return 1;
		}
	}

	/* calculate local memory per node for the update table */
	totalMem = params->HPLMaxProcMem;
	totalMem /= sizeof(table_t);

	/* calculate the size of update array (must be a power of 2) */
	for (totalMem *= 0.5, logTableSize = 0, TableSize = 1;
		totalMem >= 1.0;
		totalMem *= 0.5, logTableSize++, TableSize <<= 1)
		; /* EMPTY */

	Table = HPCC_XMALLOC(table_t, TableSize);
	if (!Table) {
		if (doIO) {
			fprintf(outFile, "Failed to allocate memory for the update table (" FSTRU64 ").\n", (u64Int)TableSize);
			if (params->outFname[0] != '\0') fclose(outFile);
		}
		return 1;
	}
	params->RandomAccess_LCG_N = (s64Int)TableSize;
	nupdate = (nupdate_t)TableSize * UPDATE_FACTOR;

	/* Print parameters for run */
	if (doIO) {
		fprintf(outFile, "Main table addr   = %p\n", Table);
		fprintf(outFile, "Main table size   = 2^%u = " FSTRU64 " words (%u bytes)\n", logTableSize, (u64Int)TableSize, (unsigned)sizeof(table_t));
		fprintf(outFile, "Number of updates = " FSTRU64 "\n", (u64Int)nupdate);
	}

	/* Initialize main table */
	for (t=0; t<TableSize; t++) Table[t] = t;

	/* Begin timing here */
	tsetup = treorg = tcache = 0;
	CLOCKS_EMULATE
	CACHE_BARRIER(dre)
	TRACE_START
	STATS_START
	/* assume input data is in memory and not cached (flushed and invalidated) */
	tget(start);
//	cputime = -CPUSEC();
//	realtime = -RTSEC();

	RandomAccessUpdate_LCG(logTableSize, TableSize, Table, nupdate);

	/* End timed section */
	/* assume output data is in memory (flushed) */
	tget(finish);
	CACHE_BARRIER(dre)
	STATS_STOP
	TRACE_STOP
	CLOCKS_NORMAL
	toper = tdiff(finish,start)-tsetup-treorg-tcache;
	realtime = tesec(finish,start);
//	cputime += CPUSEC();
//	realtime += RTSEC();

	/* make sure no division by zero */
	*GUPs = (realtime > 0.0 ? 1.0 / realtime : -1.0);
	*GUPs *= 1e-9*nupdate;
	/* Print timing results */
	if (doIO) {
//		fprintf(outFile, "CPU time used  = %.6f seconds\n", cputime);
		fprintf(outFile, "Real time used = %.6f seconds\n", realtime);
		fprintf(outFile, "%.9f Billion(10^9) Updates per second [GUP/s]\n", *GUPs);
#if defined(USE_ACC)
		fprintf(outFile, "Setup time: %f sec\n", tsetup/(double)TICKS_ESEC);
		fprintf(outFile, "Reorg time: %f sec\n", treorg/(double)TICKS_ESEC);
#endif
		fprintf(outFile, "Oper. time: %f sec\n", toper/(double)TICKS_ESEC);
		fprintf(outFile, "Cache time: %f sec\n", tcache/(double)TICKS_ESEC);
		STATS_PRINT
	}

#if defined(VERIFY) && !defined(COUNT_DUP)
	/* Verification of results (in serial or "safe" mode; optional) */
	{
		nupdate_t i;
		size_t errors = 0;
		uran_t temp = 1;

		for (i=0; i<nupdate; i++) {
			temp = LCG_MUL64 * temp + LCG_ADD64;
			Table[temp >> (sizeof(uran_t)*8 - logTableSize)] ^= temp;
		}

		for (t=0; t<TableSize; t++)
			if (Table[t] != t)
				errors++;

		if (doIO) {
			fprintf(outFile, "Found " FSTRU64 " errors in " FSTRU64 " locations (%s).\n",
					(u64Int)errors, (u64Int)TableSize, (errors <= 0.01*TableSize) ? "passed" : "failed");
		}
		if (errors <= 0.01*TableSize) *failure = 0;
		else *failure = 1;
	}
#endif

	HPCC_free(Table);

	if (doIO) {
		fflush(outFile);
		if (params->outFname[0] != '\0') fclose(outFile);
	}

	return 0;
}
