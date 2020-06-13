/*
$Id: $

Description: key-value workload generator

$Log: $
*/

#define ALIGN_SZ 256
#define _FILE_OFFSET_BITS 64
//#define NDEBUG
#include <stdio.h>
#include <stdlib.h> /* malloc, free, exit, rand, atoi, atof, strtoul */
#include <ctype.h> /* toupper, tolower, isalpha, isspace, isdigit, isgraph */
#include <string.h> /* strcmp, strlen, memset, memcmp */
#include <math.h> /* pow, ceil */
#include <assert.h> /* assert */
#if defined(_OPENMP)
#include <omp.h>
#endif

// Example main arguments
// #define MARGS "-e32Mi -l.60 -rsrr550.fa -w1Mi -h.90 -z.99"
// #define MARGS "-e32Mi -l.60 -c -w1Mi -h.90 -z.99" *
// #define MARGS "-p -e32Mi -l.50 -rsrr550.fa -w8Mi -h.90 -z.99"
// #define MARGS "-p -e32Mi -l.20 -rsrr_nr.fa -qsrr_sh.fa"
// #define MARGS "-p -e32Mi -l.20 -cd"
// #define MARGS "-p -e32Mi -l.20 -c -qsrr_sh.fa"
// #define MARGS "-e8Mi -ramr_cur.fa -qamr_cur.fa"
// #define MARGS "-e1K -k16 -rtestdb.fa -qtestqr.fa"

#include "lime.h"

#include "batch.h"
#include "batch_map.h"

#define DEFAULT_ENT 100000
#define DEFAULT_LOAD 0.95
#define DEFAULT_BLOCK_LSZ 10 /* log 2 size */
#define DEFAULT_KLEN 8
#define DEFAULT_HITR 0.98
#define DEFAULT_SKEW 0.0
#define DEFAULT_WORK 0

#define CFLAG 0x01
#define PFLAG 0x02

#define PROGRESS_COUNT 1000000

typedef unsigned long long ikey_t;
typedef unsigned int ival_t;
typedef unsigned long ulong_t;

int flags; /* argument flags */
unsigned earg = DEFAULT_ENT; /* maximum entries (keys) */
float larg = DEFAULT_LOAD; /* load factor */
unsigned barg = 1U<<DEFAULT_BLOCK_LSZ; /* buffer block length */
int karg = DEFAULT_KLEN; /* key length */ // TODO: KSZ
float harg = DEFAULT_HITR; /* hit ratio */
float zarg = DEFAULT_SKEW; /* zipf skew */
unsigned warg = DEFAULT_WORK; /* workload length */

// TODO: find a better place for these globals
/* these are global to avoid overhead at runtime on the stack */
tick_t t0, t1, t2, t3;
unsigned long long tbatch, toper, trun;

// typedef kvop<ikey_t,ival_t> entry_type;

#if 1
#include "vcon.h"
template <typename elem_t> using sp_vec = vcon<elem_t, true>;  // scratchpad
template <typename elem_t> using hp_vec = vcon<elem_t, false>; // heap
#else
#include <vector>
template <typename elem_t> using sp_vec = std::vector<elem_t>; // heap
template <typename elem_t> using hp_vec = std::vector<elem_t>; // heap
#endif

#if defined(MULTIMAP)
typedef batch_multimap<ikey_t,ival_t> kdb_t;
#else // MULTIMAP
typedef batch_map<ikey_t,ival_t> kdb_t;
#endif // MULTIMAP

typedef typename kdb_t::key_type key_type;
typedef typename kdb_t::mapped_type mapped_type;
typedef typename kdb_t::value_type value_type;
typedef typename kdb_t::size_type size_type;
typedef typename kdb_t::const_local_iterator const_local_iterator;
typedef typename kdb_t::entry_type entry_type;

kdb_t kvdb;

#define LOAD_COUNT kvdb.size()
#define LOAD_MAX (kvdb.bucket_count() * larg + 0.5)
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if 0

#define RAND_VAR \
	key_type mask = (~(key_type)0 >> ((sizeof(key_type)-karg)*8))

#define RAND_SEED(n) \
	srand(n)

#define RAND_GEN \
	((((key_type)rand() << sizeof(int)*8) ^ rand()) & mask)

#else

#include <random>

#define RAND_VAR \
	std::mt19937_64 generator; \
	std::uniform_int_distribution<key_type> \
	distribution(0,~(key_type)0 >> ((sizeof(key_type)-karg)*8))

#define RAND_SEED(n) \
	generator.seed(n)

#define RAND_GEN \
	distribution(generator)

#endif

static unsigned long atoulk(const char *s)
{
	char *kptr;
	unsigned long num = strtoul(s, &kptr, 0);
	unsigned int k = (isalpha(kptr[0]) && toupper(kptr[1]) == 'I') ? 1024 : 1000;
	switch (toupper(*kptr)) {
	case 'K': num *= k; break;
	case 'M': num *= k*k; break;
	case 'G': num *= k*k*k; break;
	}
	return num;
}


int MAIN(int argc, char *argv[])
{
	int nok = 0;
	char *s;

#if defined(USE_ACC)
	kvdb.acc.wait(); /* wait for accelerator initialization */
#endif // USE_ACC
	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s; s++)
			switch (*s) {
			/* * * * * parameters * * * * */
			case 'e':
				if (isdigit(s[1])) earg = atoulk(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'l':
				if (s[1] == '.' || isdigit(s[1])) larg = atof(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'b':
				if (isdigit(s[1])) barg = (1U << atoi(s+1));
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'k':
				if (isdigit(s[1])) karg = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'h':
				if (s[1] == '.' || isdigit(s[1])) harg = atof(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'z':
				if (s[1] == '.' || isdigit(s[1])) zarg = atof(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'p':
				flags |= PFLAG;
				break;
			/* * * * * operations * * * * */
			case 'c':
				flags |= CFLAG;
				break;
			case 'w':
				if (isdigit(s[1])) warg = atoulk(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			default:
				nok = 1;
				fprintf(stderr, " -- not an option: %c\n", *s);
				break;
			}

	if (barg < 2) {
		fprintf(stderr, " -- block length must be 2 or greater.\n");
		nok = 1;
	}
	if ((unsigned)karg > sizeof(ikey_t)) {
		fprintf(stderr, " -- key length must be %lu or less.\n", (ulong_t)sizeof(ikey_t));
		nok = 1;
	}
	// TODO: bounds check larg and harg
	if (nok || argc > 0) { // (argc > 0 && *argv[0] == '?')
		fprintf(stderr, "Usage: kvgen {-flag} {-option<arg>} (example: kvgen -cp -e16Mi -l.20)\n");
		fprintf(stderr, "      ---------- parameters ----------\n");
		fprintf(stderr, "  -e  max entries (keys) <int>, default %u\n", DEFAULT_ENT);
		fprintf(stderr, "  -l  load factor <float>, default %.4f\n", DEFAULT_LOAD);
		fprintf(stderr, "  -b  block length 2^n <int>, default: n=%u\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -k  key length <int>, default %d\n", DEFAULT_KLEN);
		fprintf(stderr, "  -h  hit ratio <float>, default %.4f\n", DEFAULT_HITR);
		fprintf(stderr, "  -z  zipf skew <float>, default %.4f\n", DEFAULT_SKEW);
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "      ---------- operations ----------\n");
		fprintf(stderr, "  -c  insert random keys\n");
		// read write insert delete %
		fprintf(stderr, "  -w  workload length <int>, default %u\n", DEFAULT_WORK);
		fprintf(stderr, "      --------------------------------\n");
		exit(EXIT_FAILURE);
	}
	printf("########## KVgen ##########\n");
#if defined(_OPENMP)
	// to control the number of threads use: export OMP_NUM_THREADS=N
	printf("threads: %d\n", omp_get_max_threads());
#endif
#if defined(USE_SP)
	if (barg > SP_SIZE/sizeof(entry_type)) barg = SP_SIZE/sizeof(entry_type);
#endif
	printf("block length: %u\n", barg);
	printf("key length: %d\n", karg);
	printf("max entries: %u\n", earg);
	printf("max key size: %lu max value size: %lu\n", (ulong_t)sizeof(ikey_t), (ulong_t)sizeof(ival_t));
	printf("batch entry size: %lu\n", (ulong_t)sizeof(entry_type));
#if defined(USE_ACC)
	printf("slot size: %lu\n", (ulong_t)sizeof(kdb_t::slot_s));
#endif // USE_ACC

	/* * * * * * * * * * Start Up * * * * * * * * * */
	tget(t0);
	kvdb.reserve(earg);
	tget(t1);
	printf("Startup time: %f sec\n", tsec(t1, t0));

	if (flags & CFLAG) {
		int klim = ceil(log(LOAD_MAX)/log(256));
		/* test for key length too small for specified load factor */
		/* if key length is too small, there are not enough permutations */
		if (karg < klim) { /* 256^karg < LOAD_MAX */
			fprintf(stderr, " -- key length must be %u or greater.\n", klim);
			exit(EXIT_FAILURE);
		}
	}

	/* * * * * * * * * * Insert Random Keys * * * * * * * * * */
	if (flags & CFLAG) {
		batch<ikey_t, ival_t, sp_vec> kvbuf; // use scratchpad
		size_type pcount = PROGRESS_COUNT;
		size_type maxl = LOAD_MAX; /* max load count */
		RAND_VAR;

		if (flags & PFLAG) fprintf(stderr, "insert");
		RAND_SEED(42);
		tbatch = 0;
		kvbuf.clear_counts();
		kvbuf.reserve(barg);

		tget(t0);
		/* Insert keys until the specified load factor is reached */
		/* The first test is faster, the second is more accurate. */
		while (LOAD_COUNT < maxl || kvdb.load_factor() < larg) {
			ikey_t key = RAND_GEN;
			ival_t val = kvbuf.insert_tot+1;
			kvbuf.insert({key, val});
			if (kvbuf.size() == kvbuf.capacity()) {
				tget(t1);
				kvdb.exec(kvbuf.data(), kvbuf.size());
				tget(t2);
				tinc(tbatch, tdiff(t2,t1));
				kvbuf.update_counts();
				kvbuf.clear();
			}
			if ((flags & PFLAG) && kvbuf.insert_tot >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		tget(t1);
		kvdb.exec(kvbuf.data(), kvbuf.size());
		tget(t2);
		tinc(tbatch, tdiff(t2,t1));
		kvbuf.update_counts();
		CACHE_SEND_ALL(kvdb.acc)
		tget(t3);

		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(t3, t0);
		toper = trun-tbatch;
		printf("Insert count: %lu\n", (ulong_t)kvbuf.insert_tot);
		printf("Run     time: %f sec\n", tvsec(trun));
		printf("Oper.   time: %f sec\n", tvsec(toper));
		printf("Insert  time: %f sec\n", tvsec(tbatch));
		printf("Insert  rate: %f ops/sec\n", kvbuf.insert_tot/tvsec(tbatch));
	}

	/* * * * * * * * * * Display Database Stats * * * * * * * * * */
	if (flags & CFLAG) {
		if (flags & PFLAG) fprintf(stderr, "gather stats...\n");

		tget(t0);
		kvdb.print_stats();
		tget(t1);
		SHOW_HEAP
		printf("Stats   time: %f sec\n", tsec(t1, t0));
	}

	/* * * * * * * * * * Workload * * * * * * * * * */
	if (warg && kvdb.size()) {
		// optionally allocate from scratchpad, use sp_vec
		batch<ikey_t, ival_t, hp_vec> kvbuf;
		double zeta = 0.0;
		size_type i, rank, samples;
		key_type temp;
		RAND_VAR;

		RAND_SEED(43);
		kvbuf.clear_counts();
		kvbuf.reserve(warg);

		// When Zipf is active, the miss distribution is independent
		// of the hit distribution. If a single distribution is wanted,
		// random ranks could be selected until the required number of
		// misses are generated. These ranks could be skipped during
		// hit generation. Also a fully random approach could be used.

		/* calculate zeta */
		if (zarg != 0.0) {
			// FIXME: for multimap, use key count
			size_type zN = kvdb.size(); /* Zipf N, dictionary size  */
			if (zN > 10000000U) zN = 10000000U;
			if (flags & PFLAG) fprintf(stderr, "calc zeta... ");
			for (i = 1; i <= zN; i++) zeta += 1.0/pow((double)i, zarg);
			if (flags & PFLAG) fprintf(stderr, "%f\n", zeta);
		}

		/* generate misses by checking if random keys are in kvdb */
		if (flags & PFLAG) fprintf(stderr, "generate misses...\n");
		samples = (1.0-harg)*warg+0.5;
		if (samples > warg) samples = warg;
		rank = 1;
		for (i = 0; i < samples; i++) {
			/* generate a random item */
			do {
				temp = RAND_GEN;
			} while (kvdb.count(temp));
			kvbuf.find(temp);
			/* repeat item */
			if (zarg != 0.0) {
				size_type count = ceil((1.0/(pow((double)rank, zarg)*zeta)) * samples);
				if (samples-i < count) count = samples-i;
				while (count > 1) {
					kvbuf.find(temp);
					i++;
					count--;
				}
				rank++;
			}
		}

		/* generate hits by randomly selecting items from kvdb */
		if (flags & PFLAG) fprintf(stderr, "generate hits...\n");
		samples = warg - i;
		rank = 1;
		for (i = 0; i < samples; i++) {
			size_type bucket, steps;
			const_local_iterator clit;
			/* pick a random bucket until a non-empty one is found */
			do {
				bucket = rand() % kvdb.bucket_count();
				clit = kvdb.cbegin(bucket);
			} while (clit == kvdb.cend(bucket));
			/* pick a random item from the bucket */
			for (steps = rand() % kvdb.bucket_size(bucket); steps; steps--) clit++;
			kvbuf.find(clit->first);
			/* repeat item */
			if (zarg != 0.0) {
				size_type count = ceil((1.0/(pow((double)rank, zarg)*zeta)) * samples);
				if (samples-i < count) count = samples-i;
				while (count > 1) {
					kvbuf.find(clit->first);
					i++;
					count--;
				}
				rank++;
			}
		}

		/* shuffle keys */
		if (flags & PFLAG) fprintf(stderr, "shuffle keys...\n");
		kvbuf.shuffle();

		/* execute workload */
		if (flags & PFLAG) fprintf(stderr, "workload");
		tbatch = 0;
		kvdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kvdb.acc)
		TRACE_START
		STATS_START

		if (flags & PFLAG) fprintf(stderr, "...");
		tget(t0);
		kvdb.exec(kvbuf.data(), kvbuf.size());
		tget(t1);
		kvbuf.update_counts();
		tget(t2);

		CACHE_BARRIER(kvdb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(t2, t0);
		tbatch = tdiff(t1, t0);
		toper = trun-tbatch;
		if (kvbuf.find_tot > 0) {
			printf("Lookup count: %lu\n", (ulong_t)kvbuf.find_tot);
			printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)kvbuf.find_yes, (double)kvbuf.find_yes/kvbuf.find_tot*100.0);
			printf("Lookup  zipf: %.2f\n", zarg);
		}
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Command time: %f sec\n", tvesec(tbatch));
		printf("Command rate: %f ops/sec\n", (kvbuf.insert_tot+kvbuf.find_tot+kvbuf.erase_tot)/tvesec(tbatch));
		kvdb.print_time();
		STATS_PRINT
	}

	/* * * * * * * * * * Wrap Up * * * * * * * * * */

	TRACE_CAP
	return EXIT_SUCCESS;
}

/* TODO:
*/

/* DONE:
*/
