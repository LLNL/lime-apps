/*
$Id: $

Description: read test bench

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
#if defined(SYSTEMC)
#include <sys/time.h>
#endif

#include "fasta.h"
#include "path.h"

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

#include "block_map.h"

#define DEFAULT_ENT 100000
#define DEFAULT_LOAD 0.95
#define DEFAULT_BLOCK_LSZ 10 /* log 2 size */
#define DEFAULT_KLEN 18
#define DEFAULT_HITR 0.98
#define DEFAULT_SKEW 0.0
#define DEFAULT_WORK 0

#define CFLAG 0x01
#define DFLAG 0x02
#define PFLAG 0x04

#define PROGRESS_COUNT 1000000

#if defined(ENABLE_PLOT)
#define PIPE 1 /* enable pipe of plot commands */
#define DIRC '/' /* directory separator character */
#define EXT_PLOT ".gpi"
#define EXT_DATA ".dat"
#define EXT_TERM ".png"
#endif // ENABLE_PLOT

typedef unsigned long long kmer_t;
#define KEY_SZ sizeof(kmer_t)
typedef unsigned int sid_t;
#define DAT_SZ sizeof(sid_t)
typedef unsigned long ulong_t;

int flags; /* argument flags */
unsigned earg = DEFAULT_ENT; /* maximum entries (keys) */
float larg = DEFAULT_LOAD; /* load factor */
unsigned barg = 1U<<DEFAULT_BLOCK_LSZ; /* buffer block length */
int karg = DEFAULT_KLEN; /* k-mer length */
float harg = DEFAULT_HITR; /* hit ratio */
float zarg = DEFAULT_SKEW; /* zipf skew */
char *qarg = NULL; /* query file name (in) */
char *rarg = NULL; /* reference file name (in) */
char *sarg = NULL; /* saved workload file name (out) */
unsigned warg = DEFAULT_WORK; /* workload length */
#if defined(ENABLE_PLOT)
int xarg = 0; /* plot x range */
char *garg = NULL; /* plot command file name (out) */
char *targ = NULL; /* plot terminal file name (out) */
#endif // ENABLE_PLOT

// TODO: find a better place for these globals
/* these are global to avoid overhead at runtime on the stack */
tick_t t0, t1;
tick_t start, finish;
unsigned long long tinsert, tlookup, toper, trun;

// TODO: consider using std::conditional<>::type for result type
#if defined(MULTIMAP)
typedef block_multimap<kmer_t,sid_t> kdb_t;
#if defined(USE_ACC)
typedef typename kdb_t::mapped_type* result_type;
inline bool is_valid(const result_type &v) {return v != nullptr;}
#else // USE_ACC
typedef std::pair<typename kdb_t::iterator, typename kdb_t::iterator> result_type;
inline bool is_valid(const result_type &v) {return v.first != typename kdb_t::iterator(nullptr);}
#endif // USE_ACC

#else // MULTIMAP
typedef block_map<kmer_t,sid_t> kdb_t;
#if 1 || defined(USE_ACC)
typedef typename kdb_t::mapped_type result_type;
inline bool is_valid(const result_type &v) {return v != 0;}
#else // USE_ACC
typedef typename kdb_t::iterator result_type;
inline bool is_valid(const result_type &v) {return v != typename kdb_t::iterator(nullptr);}
#endif // USE_ACC

#endif // MULTIMAP

typedef typename kdb_t::key_type key_type;
typedef typename kdb_t::mapped_type mapped_type;
typedef typename kdb_t::value_type value_type;
typedef typename kdb_t::size_type size_type;
typedef typename kdb_t::const_local_iterator const_local_iterator;
typedef std::pair<key_type, mapped_type> kvpair_type;

kdb_t kdb;

#define LOAD_COUNT kdb.size()
#define LOAD_MAX (kdb.bucket_count() * larg + 0.5)
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#if 0

#define RAND_VAR \
	key_type mask = (~(key_type)0 >> (sizeof(key_type)*8-karg*2))

#define RAND_SEED(n) \
	srand(n)

#define RAND_GEN \
	((((key_type)rand() << sizeof(int)*8) ^ rand()) & mask)

#else

#include <random>

#define RAND_VAR \
	std::mt19937_64 generator; \
	std::uniform_int_distribution<key_type> \
	distribution(0,~(key_type)0 >> (sizeof(key_type)*8-karg*2))

#define RAND_SEED(n) \
	generator.seed(n)

#define RAND_GEN \
	distribution(generator)

#endif

struct {
	size_type length; /* length of buffers */
	size_type lcount; /* count of items in lookup buffers */
	size_type acount; /* count of items in add buffer */

	ulong_t hits; /* number of search hits */

	key_type *keys;
	result_type *result;
	kvpair_type *kvpair;

	void clear_counts(void)
	{
		hits = 0;
	}

	void init(size_type buf_len)
	{
		length = buf_len;
		acount = 0;
		lcount = 0;

		keys = SP_NALLOC(key_type, length); /* used on lookup */
		chk_alloc(keys, sizeof(key_type)*length, "SP_NALLOC keys in init()");

		result = SP_NALLOC(result_type, length); /* used on lookup */
		chk_alloc(result, sizeof(result_type)*length, "SP_NALLOC result in init()");

		kvpair = SP_NALLOC(kvpair_type, length); /* used on add */
		chk_alloc(kvpair, sizeof(kvpair_type)*length, "SP_NALLOC kvpair in init()");

	}

#if defined(NO_BUFFER)

	inline void lookup(key_type key)
	{
		tget(t0);
#if defined(MULTIMAP)
		result_type res = kdb.equal_range(key);
#else
		result_type res = kdb.find(key);
#endif
		tget(t1);
		tinc(tlookup, tdiff(t1,t0));
#if 1 // TODO: time without
		if (is_valid(res)) hits++;
#endif
	}

	inline void add(key_type key, mapped_type mval)
	{
		tget(t0);
		kdb.insert({key, mval});
		tget(t1);
		tinc(tinsert, tdiff(t1,t0));
	}

#else

	// TODO: use std::container to emplace
	inline void lookup(key_type key)
	{
		if (lcount == length) flush_lookup();
		keys[lcount++] = key;
	}

	// TODO: use std::container to emplace
	inline void add(key_type key, mapped_type mval)
	{
		if (acount == length) flush_add();
		kvpair[acount++] = {key, mval};
	}

#endif // NO_BUFFER

	void flush_lookup(void)
	{
		if (lcount == 0) return;
		tget(t0);
#if defined(MULTIMAP)
		kdb.equal_range(result, keys, lcount);
#else
		kdb.find(result, keys, lcount);
#endif
		tget(t1);
		tinc(tlookup, tdiff(t1,t0));
#if 1 // TODO: time without
		for (size_type i = 0; i < lcount; i++) {
			if (is_valid(result[i])) hits++;
		}
#endif
		lcount = 0;
	}

	void flush_add(void)
	{
		if (acount == 0) return;
		tget(t0);
		kdb.insert(reinterpret_cast<typename kdb_t::const_pointer>(kvpair), acount);
		tget(t1);
		tinc(tinsert, tdiff(t1,t0));
		acount = 0;
	}

	void block_lookup(key_type *keys, size_type blen)
	{
		for (size_type i = 0; i < blen; i += length) {
			size_type count = MIN(length, blen-i);
			tget(t0);
#if defined(MULTIMAP)
			kdb.equal_range(result, keys+i, count);
#else
			kdb.find(result, keys+i, count);
#endif
			tget(t1);
			tinc(tlookup, tdiff(t1,t0));
#if 1 // TODO: time without
			for (size_type j = 0; j < count; j++) {
				if (is_valid(result[j])) hits++;
			}
#endif
		}
	}

} kbuf;

#if 0
static
void ktob(kmer_t kmer, int klen, char *buf)
{
	int i, e = klen*2;

	for (i = 0; i < e; i++) {
		buf[e-i-1] = (kmer&1) ? '1' : '0';
		kmer >>= 1;
	}
	buf[e] = '\0';
	assert(kmer == 0);
}
#endif

static
void kton(kmer_t kmer, int klen, char *buf)
{
	int i;
	char ktab[4] = {'A', 'C', 'G', 'T'};

	for (i = 0; i < klen; i++) {
		buf[klen-i-1] = ktab[kmer&3];
		kmer >>= 2;
	}
	buf[klen] = '\0';
	assert(kmer == 0);
}

#if 0
static
void kputs(kmer_t kmer, int klen)
{
	char buf[64];
	kton(kmer, klen, buf);
	puts(buf);
}
#endif

#define ENCODE(t, c, k) \
switch (c) { \
case 'a': case 'A': t = 0; break; \
case 'c': case 'C': t = 1; break; \
case 'g': case 'G': t = 2; break; \
case 't': case 'T': t = 3; break; \
default: k = 0; continue; \
}

/*
   Given a sequence of nucleotide characters,
   break it into canonical k-mers in one pass.
   Nucleotides are encoded with two bits in
   the k-mer. Any k-mers with ambiguous characters
   are skipped.
 
   str:  DNA sequence (read)
   slen: DNA sequence length in nucleotides
   klen: k-mer length in nucleotides
*/

static
int seq_lookup(char *str, int slen, int klen)
{
	int kcnt = 0;
	int j; /* position of last nucleotide in sequence */
	int k = 0; /* count of contiguous valid characters */
	int highbits = (klen-1)*2; /* shift needed to reach highest k-mer bits */
	kmer_t mask = (~(kmer_t)0 >> (sizeof(kmer_t)*8-klen*2)); /* bits covering encoded k-mer */
	kmer_t forward = 0; /* forward k-mer */
	kmer_t reverse = 0; /* reverse k-mer */
	kmer_t kmer; /* canonical k-mer */

	// fprintf(stderr, "str: %s\n", str);
	for (j = 0; j < slen; j++) {
		register int t;
		ENCODE(t, str[j], k);
		forward = ((forward << 2) | t) & mask;
		reverse = ((kmer_t)(t^3) << highbits) | (reverse >> 2);
		if (++k >= klen) {
			kcnt++;
#if 0
			{
				char buf[64];
				kton(forward, klen, buf);
				fprintf(stderr, "forward: %s pos: %d\n", buf, j-klen+1);
				kton(reverse, klen, buf);
				fprintf(stderr, "reverse: %s pos: %d\n", buf, slen-j-1);
			}
#endif
			kmer = (forward < reverse) ? forward : reverse;
			/* do k-mer lookup here... */
			/* zero based position of forward k-mer is (j-klen+1) */
			/* zero based position of reverse k-mer is (slen-j-1) */
			kbuf.lookup(kmer);
		}
	}
	return kcnt;
}

static
int seq_add(char *str, int slen, int klen, int sid)
{
	int kcnt = 0;
	int j; /* position of last nucleotide in sequence */
	int k = 0; /* count of contiguous valid characters */
	int highbits = (klen-1)*2; /* shift needed to reach highest k-mer bits */
	kmer_t mask = (~(kmer_t)0 >> (sizeof(kmer_t)*8-klen*2)); /* bits covering encoded k-mer */
	kmer_t forward = 0; /* forward k-mer */
	kmer_t reverse = 0; /* reverse k-mer */
	kmer_t kmer; /* canonical k-mer */

	// fprintf(stderr, "str: %s\n", str);
	for (j = 0; j < slen; j++) {
		register int t;
		ENCODE(t, str[j], k);
		forward = ((forward << 2) | t) & mask;
		reverse = ((kmer_t)(t^3) << highbits) | (reverse >> 2);
		if (++k >= klen) {
			kcnt++;
#if 0
			{
				char buf[64];
				kton(forward, klen, buf);
				fprintf(stderr, "forward: %s pos: %d\n", buf, j-klen+1);
				kton(reverse, klen, buf);
				fprintf(stderr, "reverse: %s pos: %d\n", buf, slen-j-1);
			}
#endif
			kmer = (forward < reverse) ? forward : reverse;
			/* do k-mer add here... */
			/* zero based position of forward k-mer is (j-klen+1) */
			/* zero based position of reverse k-mer is (slen-j-1) */
			kbuf.add(kmer, sid);
		}
	}
	return kcnt;
}

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
	kdb.acc.wait(); /* wait for accelerator initialization */
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
			case 'd':
				flags |= DFLAG;
				break;
			case 'q':
				qarg = s+1;
				s += strlen(s+1);
				break;
			case 'r':
				rarg = s+1;
				s += strlen(s+1);
				break;
			case 's':
				sarg = s+1;
				s += strlen(s+1);
				break;
			case 'w':
				if (isdigit(s[1])) warg = atoulk(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			/* * * * * output options * * * * */
#if defined(ENABLE_PLOT)
			case 'g':
				garg = s+1;
				s += strlen(s+1);
				break;
			case 't':
				targ = s+1;
				s += strlen(s+1);
				if (garg == NULL) garg = targ;
				break;
			case 'x':
				if (isdigit(s[1])) xarg = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
#endif // ENABLE_PLOT
			default:
				nok = 1;
				fprintf(stderr, " -- not an option: %c\n", *s);
				break;
			}

	if (barg < 2) {
		fprintf(stderr, " -- block length must be 2 or greater.\n");
		nok = 1;
	}
	if ((unsigned)karg*2 > sizeof(kmer_t)*8) {
		fprintf(stderr, " -- k-mer length must be %lu or less.\n", (ulong_t)sizeof(kmer_t)*4);
		nok = 1;
	}
	// TODO: bounds check larg and harg
	if (nok || argc > 0) { // (argc > 0 && *argv[0] == '?')
		fprintf(stderr, "Usage: rtb {-flag} {-option<arg>} (example: rtb -cp -e16Mi -l.20)\n");
		fprintf(stderr, "      ---------- parameters ----------\n");
		fprintf(stderr, "  -e  max entries (keys) <int>, default %u\n", DEFAULT_ENT);
		fprintf(stderr, "  -l  load factor <float>, default %.4f\n", DEFAULT_LOAD);
		fprintf(stderr, "  -b  block length 2^n <int>, default: n=%u\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -k  k-mer length <int>, default %d\n", DEFAULT_KLEN);
		fprintf(stderr, "  -h  hit ratio <float>, default %.4f\n", DEFAULT_HITR);
		fprintf(stderr, "  -z  zipf skew <float>, default %.4f\n", DEFAULT_SKEW);
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "      ---------- operations ----------\n");
		fprintf(stderr, "  -c  add random keys\n");
		fprintf(stderr, "  -d  lookup random keys\n");
		fprintf(stderr, "  -q  query with FASTA file <in_file>\n");
		fprintf(stderr, "  -r  read FASTA reference <in_file>\n");
		fprintf(stderr, "  -s  save workload to FASTA file <out_file>\n");
		fprintf(stderr, "  -w  workload length <int>, default %u\n", DEFAULT_WORK);
		fprintf(stderr, "      --------------------------------\n");
#if defined(ENABLE_PLOT)
		fprintf(stderr, "  -g  plot command <out_file>\n");
		fprintf(stderr, "  -t  plot terminal <out_file>\n");
		fprintf(stderr, "  -x  plot x range <int>, default auto\n");
#endif // ENABLE_PLOT
		exit(EXIT_FAILURE);
	}
	printf("########## RTB ##########\n");
#if defined(_OPENMP)
	// to control the number of threads use: export OMP_NUM_THREADS=N
	printf("threads: %d\n", omp_get_max_threads());
#endif
#if defined(USE_SP)
	if (barg > SP_SIZE/sizeof(unsigned)/8) barg = SP_SIZE/sizeof(unsigned)/8; /* up to 1/8th of scratchpad */
#endif
	printf("block length: %u\n", barg);
	printf("k-mer length: %d\n", karg);
	printf("max entries: %u\n", earg);
	printf("key size: %lu data size: %lu\n", (ulong_t)KEY_SZ, (ulong_t)DAT_SZ);
#if defined(USE_ACC)
	printf("slot size: %lu\n", (ulong_t)sizeof(kdb_t::slot_s));
#endif // USE_ACC

	/* * * * * * * * * * Start Up * * * * * * * * * */
	tget(start);
	kdb.reserve(earg);
	kbuf.init(barg);
	tget(finish);
	printf("Startup time: %f sec\n", tsec(finish, start));

	if (flags & CFLAG || rarg != NULL) {
		int klim = ceil(log(LOAD_MAX)/log(4));
		/* test for k-mer length too small for specified load factor */
		/* if k-mer length is too small, there are not enough permutations */
		if (karg < klim) { /* 4^karg < LOAD_MAX */
			fprintf(stderr, " -- k-mer length must be %u or greater.\n", klim);
			exit(EXIT_FAILURE);
		}
	}

	/* * * * * * * * * * Add Random Keys * * * * * * * * * */
	if (flags & CFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; /* add count */
		size_type maxl = LOAD_MAX; /* max load count */
		RAND_VAR;

		if (flags & PFLAG) fprintf(stderr, "add");
		RAND_SEED(42);
		tinsert = tlookup = 0;
		kbuf.clear_counts();
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		// TRACE_START
		// STATS_START

		tget(start);
		/* Add k-mers until the specified load factor is reached */
		/* The first test is faster, the second is more accurate. */
		while (LOAD_COUNT < maxl || kdb.load_factor() < larg) {
			kmer_t kmer = RAND_GEN;
			sid_t sid = ++acount;
			kbuf.add(kmer, sid);
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kbuf.flush_add();
		CACHE_SEND_ALL(kdb.acc)
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		// STATS_STOP
		// TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Insert count: %lu\n", (ulong_t)acount);
		printf("Insert  rate: %f ops/sec\n", acount/tvesec(tinsert));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Insert  time: %f sec\n", tvesec(tinsert));
		kdb.print_time();
		// STATS_PRINT
	}

	/* * * * * * * * * * Read FASTA Reference * * * * * * * * * */
	if (rarg != NULL) {
		FILE *fin;
		sequence entry;
		int i, len;
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; /* add count */
		size_type tcount = 0; /* total bases count */
		size_type maxl = LOAD_MAX; /* max load count */

		if ((fin = fopen(rarg, "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", rarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "read");
		tinsert = tlookup = 0;
		kbuf.clear_counts();
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		// TRACE_START
		// STATS_START

		tget(start);
		for (i = 1; (len = Fasta_Read_Entry(fin, &entry)) > 0; i++) {
			acount += seq_add(entry.str, len, karg, i);
			tcount += len;
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
			free(entry.hdr);
			free(entry.str);
			/* Add k-mers until the specified load factor is reached */
			/* The first test is faster, the second is more accurate. */
			if (LOAD_COUNT >= maxl && kdb.load_factor() >= larg) break;
		}
		kbuf.flush_add();
		CACHE_SEND_ALL(kdb.acc)
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		// STATS_STOP
		// TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Insert count: %lu\n", (ulong_t)acount);
		printf("Insert  rate: %f ops/sec\n", acount/tvesec(tinsert));
		printf("Bases   rate: %f bp/sec\n", tcount/tvesec(trun));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Insert  time: %f sec\n", tvesec(tinsert));
		kdb.print_time();
		// STATS_PRINT
		fclose(fin);
		if (kdb.load_factor() < larg) {
			fprintf(stderr, " -- warning: did not reach load factor: %.4f\n", larg);
		}
	}

	/* * * * * * * * * * Display Database Stats * * * * * * * * * */
	if (flags & CFLAG || rarg != NULL) {
		if (flags & PFLAG) fprintf(stderr, "gather stats...\n");

		tget(start);
		kdb.print_stats();
		tget(finish);
		SHOW_HEAP
		printf("Stats   time: %f sec\n", tsec(finish, start));
	}

	/* * * * * * * * * * Lookup Random Keys * * * * * * * * * */
	if (flags & DFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; /* lookup count */
		RAND_VAR;

		if (flags & PFLAG) fprintf(stderr, "lookup");
		RAND_SEED(42);
		tinsert = tlookup = 0;
		kbuf.clear_counts();
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		TRACE_START
		STATS_START

		tget(start);
		while (lcount < kdb.size()) {
			kmer_t kmer = RAND_GEN;
			kbuf.lookup(kmer);
			lcount++;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kbuf.flush_lookup();
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)kbuf.hits, (double)kbuf.hits/lcount*100.0);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		kdb.print_time();
		STATS_PRINT
	}

	/* * * * * * * * * * Query Database * * * * * * * * * */
	if (qarg != NULL) {
		FILE *fin;
		sequence entry;
		int len;
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; /* lookup count */
		size_type tcount = 0; /* total bases count */

		if ((fin = fopen(qarg, "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", qarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "query");
		tinsert = tlookup = 0;
		kbuf.clear_counts();
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		TRACE_START
		STATS_START

		tget(start);
		while ((len = Fasta_Read_Entry(fin, &entry)) > 0) {
			lcount += seq_lookup(entry.str, len, karg);
			tcount += len;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
			free(entry.hdr);
			free(entry.str);
		}
		kbuf.flush_lookup();
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)kbuf.hits, (double)kbuf.hits/lcount*100.0);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
		printf("Bases   rate: %f bp/sec\n", tcount/tvesec(trun));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		kdb.print_time();
		STATS_PRINT
		fclose(fin);
	}

	/* * * * * * * * * * Workload * * * * * * * * * */
	if (warg && kdb.size()) {
		double zeta = 0.0;
		size_type i, j, rank, samples;
		key_type *wptr;
		key_type temp;
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; /* lookup count */
		RAND_VAR;
		// TODO: optionally allocate from scratchpad
		key_type *wload = NALLOC(key_type, warg);
		chk_alloc(wload, sizeof(key_type)*warg, "NALLOC workload array");

		RAND_SEED(43);

		// When Zipf is active, the miss distribution is independent
		// of the hit distribution. If a single distribution is wanted,
		// random ranks could be selected until the required number of
		// misses are generated. These ranks could be skipped during
		// hit generation. Also a fully random approach could be used.

		/* calculate zeta */
		if (zarg != 0.0) {
			// FIXME: for multimap, use key count
			size_type zN = kdb.size(); /* Zipf N, dictionary size  */
			if (zN > 10000000U) zN = 10000000U;
			if (flags & PFLAG) fprintf(stderr, "calc zeta... ");
			for (i = 1; i <= zN; i++) zeta += 1.0/pow((double)i, zarg);
			if (flags & PFLAG) fprintf(stderr, "%f\n", zeta);
		}

		/* generate misses by checking if random keys are in kdb */
		if (flags & PFLAG) fprintf(stderr, "generate misses...\n");
		wptr = wload;
		samples = (1.0-harg)*warg+0.5;
		if (samples > warg) samples = warg;
		rank = 1;
		for (i = 0; i < samples; i++) {
			/* generate a random item */
			do {
				temp = RAND_GEN;
			} while (kdb.count(temp));
			wptr[i] = temp;
#if 1
			/* repeat item */
			if (zarg != 0.0) {
				size_type count = ceil((1.0/(pow((double)rank, zarg)*zeta)) * samples);
				if (samples-i < count) count = samples-i;
				while (count > 1) {
					wptr[++i] = temp;
					count--;
				}
				rank++;
			}
#endif
		}

		/* generate hits by randomly selecting items from kdb */
		if (flags & PFLAG) fprintf(stderr, "generate hits...\n");
		wptr = wload + i;
		samples = warg - i;
		rank = 1;
		for (i = 0; i < samples; i++) {
			size_type bucket, steps;
			const_local_iterator clit;
			/* pick a random bucket until a non-empty one is found */
			do {
				bucket = rand() % kdb.bucket_count();
				clit = kdb.cbegin(bucket);
			} while (clit == kdb.cend(bucket));
			/* pick a random item from the bucket */
			for (steps = rand() % kdb.bucket_size(bucket); steps; steps--) clit++;
			wptr[i] = clit->first;
			/* repeat item */
			if (zarg != 0.0) {
				size_type count = ceil((1.0/(pow((double)rank, zarg)*zeta)) * samples);
				if (samples-i < count) count = samples-i;
				while (count > 1) {
					wptr[++i] = clit->first;
					count--;
				}
				rank++;
			}
		}

		/* shuffle keys */
		if (flags & PFLAG) fprintf(stderr, "shuffle keys...\n");
		/* Fisher-Yates shuffle or Knuth shuffle */
		for (i = warg-1; i > 0; --i) {
			j = rand() % (i+1);
			temp = wload[i]; /* swap */
			wload[i] = wload[j];
			wload[j] = temp;
		}

		/* save workload */
		if (sarg != NULL) {
			FILE *fout;
			char hdr[32];
			char str[48];
			sequence entry = {hdr, str};
			size_type pcount = PROGRESS_COUNT-1;
			size_type scount; /* save count */

			if ((fout = fopen(sarg, "w")) == NULL) {
				fprintf(stderr, " -- can't open file: %s\n", sarg);
				exit(EXIT_FAILURE);
			}

			if (flags & PFLAG) fprintf(stderr, "save workload");
			for (scount = 0; scount < warg; scount++) {
				sprintf(entry.hdr, "%lu", (ulong_t)scount);
				kton(wload[scount], karg, entry.str);
				Fasta_Write_File(fout, &entry, 1, karg);
				if ((flags & PFLAG) && scount >= pcount) {
					fputc('.', stderr);
					pcount += PROGRESS_COUNT;
				}
			}
			if (flags & PFLAG) fputc('\n', stderr);

			fclose(fout);
		}

		/* do lookup */
		if (flags & PFLAG) fprintf(stderr, "workload");
		tinsert = tlookup = 0;
		kbuf.clear_counts();
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		TRACE_START
		STATS_START

#if 1
		(void)pcount; /* silence warning */
		if (flags & PFLAG) fprintf(stderr, "...");
#if defined(SYSTEMC)
		struct timeval rt0;
		gettimeofday(&rt0,NULL);
#endif
		tget(start);
		kbuf.block_lookup(wload, warg);
		lcount = warg;
		tget(finish);
#if defined(SYSTEMC)
		struct timeval rt1;
		gettimeofday(&rt1,NULL);
#endif
#else
		tget(start);
		while (lcount < warg) {
			kbuf.lookup(wload[lcount]);
			lcount++;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kbuf.flush_lookup();
		tget(finish);
#endif

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)kbuf.hits, (double)kbuf.hits/lcount*100.0);
		printf("Lookup  zipf: %.2f\n", zarg);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
#if defined(SYSTEMC)
		printf("Real    time: %f sec\n", 
			(((unsigned long long)rt1.tv_sec*1000000UL+rt1.tv_usec)  -
			 ((unsigned long long)rt0.tv_sec*1000000UL+rt0.tv_usec)) /
			(double)1000000UL);
#endif
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		kdb.print_time();
		STATS_PRINT
	}

	/* * * * * * * * * * Wrap Up * * * * * * * * * */

	TRACE_CAP
	return EXIT_SUCCESS;
}

/* TODO:
 * Fix reaching load factor
 * Verify uniform and Zipf distributions
*/

/* DONE:
*/
