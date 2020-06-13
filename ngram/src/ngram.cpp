/*
$Id: $

Description: ngram test bench

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

#include <fstream>
#include <iostream>
#include <cstdint>
#include <cinttypes>

#if defined(_OPENMP)
#include <omp.h>
#endif

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
#define DEFAULT_KLEN 8
#define DEFAULT_HITR 0.98
#define DEFAULT_SKEW 0.0
#define DEFAULT_WORK 0
#define DEFAULT_DARG 4096

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

typedef uint64_t ngram_t;
#define KEY_SZ sizeof(ngram_t)
typedef unsigned int sid_t;
#define DAT_SZ sizeof(sid_t)
typedef unsigned long ulong_t;

int flags; /* argument flags */
unsigned earg = DEFAULT_ENT; /* maximum entries (keys) */
float larg = DEFAULT_LOAD; /* load factor */
unsigned barg = 1U<<DEFAULT_BLOCK_LSZ; /* buffer block length */
int karg = DEFAULT_KLEN; /* n-gram length */
float harg = DEFAULT_HITR; /* hit ratio */
float zarg = DEFAULT_SKEW; /* zipf skew */
uint64_t darg = DEFAULT_DARG; /* number of random keys to look up */
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
typedef block_multimap<ngram_t,sid_t> ndb_t;
#if defined(USE_ACC)
typedef typename ndb_t::mapped_type* result_type;
inline bool is_valid(const result_type &v) {return v != nullptr;}
#else // USE_ACC
typedef std::pair<typename ndb_t::iterator, typename ndb_t::iterator> result_type;
inline bool is_valid(const result_type &v) {return v.first != typename ndb_t::iterator(nullptr);}
#endif // USE_ACC

#else // MULTIMAP
typedef block_map<ngram_t,sid_t> ndb_t;
#if 1 || defined(USE_ACC)
typedef typename ndb_t::mapped_type result_type;
inline bool is_valid(const result_type &v) {return v != 0;}
#else // USE_ACC
typedef typename ndb_t::iterator result_type;
inline bool is_valid(const result_type &v) {return v != typename ndb_t::iterator(nullptr);}
#endif // USE_ACC

#endif // MULTIMAP

typedef typename ndb_t::key_type key_type;
typedef typename ndb_t::mapped_type mapped_type;
typedef typename ndb_t::value_type value_type;
typedef typename ndb_t::size_type size_type;
typedef typename ndb_t::const_local_iterator const_local_iterator;
typedef std::pair<key_type, mapped_type> kvpair_type;

ndb_t ndb;

#define LOAD_COUNT ndb.size()
#define LOAD_MAX (ndb.bucket_count() * larg + 0.5)
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
	distribution(0,0xFFFFFFFFFFFFFFFFull)

#define RAND_SEED(n) \
	generator.seed(n)

#define RAND_GEN \
	distribution(generator)

#endif

struct nmer_buf_t {
	size_type length; /* length of buffers */
	size_type lcount; /* count of items in lookup buffers */
	size_type acount; /* count of items in add buffer */

	ulong_t hits; /* number of search hits */

	key_type *keys;
	result_type *result;
	kvpair_type *kvpair;

	nmer_buf_t(size_type buf_len)
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

	~nmer_buf_t() {
		SP_NFREE(keys);
		SP_NFREE(result);
		SP_NFREE(kvpair);
	}

	void clear_counts(void)
	{
		hits = 0;
	}

#if defined(NO_BUFFER)

	inline void lookup(key_type key)
	{
		tget(t0);
#if defined(MULTIMAP)
		result_type res = ndb.equal_range(key);
#else
		result_type res = ndb.find(key);
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
		ndb.insert({key, mval});
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
		ndb.equal_range(result, keys, lcount);
#else
		ndb.find(result, keys, lcount);
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
		ndb.insert(reinterpret_cast<typename ndb_t::const_pointer>(kvpair), acount);
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
			ndb.equal_range(result, keys+i, count);
#else
			ndb.find(result, keys+i, count);
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

};

nmer_buf_t* nbuf;

/***************** string functions ****************/

#define READ_MODE_NULL       0
#define READ_MODE_STRING     1
#define READ_MODE_STRING_ESC 2
#define READ_MODE_HEX        3
#define OP_ADD               0
#define OP_LOOKUP            1

typedef struct {
	char *str;
	int entry_len;
} entry_t;


std::ifstream& operator>>(std::ifstream& i, entry_t& file_entry) {
	std::string buf;
	std::getline(i, buf);
	file_entry.entry_len = buf.length();
	if (file_entry.entry_len) {
		file_entry.str = (char*)malloc(file_entry.entry_len);
		memcpy(file_entry.str, buf.c_str(), file_entry.entry_len);
  }
	return i;
}

std::ofstream& operator<<(std::ofstream& o, entry_t& file_entry) {
	o.write(file_entry.str, file_entry.entry_len);
	return o;
}


static inline
int h2i(char c, unsigned char& v) {
	if (c >= 'a' && c <= 'f') {
		v = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		v = c - 'A' + 10;
	} else if (c >= '0' && c <= '9') {
		v = c - '0';
	} else {
		return -1;
	}
	return 0;
}

static inline
int hc2ic(char* b, unsigned char& v)
{
	v = 0;
	for (int i = 0; i < 2; i++) {
		int sh = 4 * i;
		unsigned char hv;
		if (h2i(b[i], hv) < 0) { return -1; }
		v |= hv << sh;
	}
	return 0;
}

static
int str_op(char *str, int slen, int nlen, int sid, int op)
{
	int ncnt = 0;
	int j; /* position of last byte in sequence */
	int k = 0; /* count of contiguous valid characters */
	ngram_t ngram = 0; /* canonical n-gram */
	int read_mode = READ_MODE_NULL;
	char hbuf[2];
	int hptr = 0;

	for (j = 0; j < slen; j++) {
		char c = str[j];
		unsigned char v = 0;
		switch (read_mode) {
			case READ_MODE_NULL: // skip to next valid character
				if (c == '"')      { read_mode = READ_MODE_STRING; }
				else if (c == '{') { read_mode = READ_MODE_HEX; }
				k = 0;
				continue;
			case READ_MODE_STRING: // read character literals
				if (c == '\\')      { read_mode = READ_MODE_STRING_ESC; continue; }
				if (c == '"')      { read_mode = READ_MODE_NULL; continue; }
				v = c;
				break;
			case READ_MODE_STRING_ESC:
				v = c;
				read_mode = READ_MODE_STRING;
				break;
			case READ_MODE_HEX: // read base-16 encoded character literals
				if (c == '}')      { read_mode = READ_MODE_NULL; continue; }
				hbuf[hptr++] = c;
				if (hptr == 2) {
					hptr = 0;
					if (hc2ic(hbuf, v) < 0) {
						// hex buf contained invalid data
						continue;
					}
				} else {
					// accumulate more characters
					continue;
				}
				break;
		}
		ngram >>= 8; // shift one character out of the buffer
		v &= 255;
		ngram |= ((uint64_t)v << (56)); // shift the new character into the buffer
		if (++k >= nlen) {
			ncnt++;
			/* do n-gram op here... */
			if (op == OP_ADD) {
				nbuf->add(ngram, sid);
			} else if (op == OP_LOOKUP) {
				nbuf->lookup(ngram);
			}
		}
	}
	return ncnt;
}

static inline
int str_lookup(char *str, int slen, int klen)
{
	return str_op(str, slen, klen, 0, OP_LOOKUP);
}

static inline
int str_add(char *str, int slen, int klen, int sid)
{
	return str_op(str, slen, klen, sid, OP_ADD);
}

static
std::size_t ktob(ngram_t ngram, int klen, char *buf)
{
	return sprintf(buf, "{ %" PRIx64 " }", ngram);
}

/***************** end string functions ****************/


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
	ndb.acc.wait(); /* wait for accelerator initialization */
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
				if (isdigit(s[1])) darg = atoulk(s+1);
				s += strlen(s+1);
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
	if ((unsigned)karg*2 > sizeof(ngram_t)*8) {
		fprintf(stderr, " -- n-gram length must be %lu or less.\n", (ulong_t)sizeof(ngram_t)*4);
		nok = 1;
	}
	// TODO: bounds check larg and harg
	if (nok || argc > 0) { // (argc > 0 && *argv[0] == '?')
		fprintf(stderr, "Usage: rtb {-flag} {-option<arg>} (example: rtb -cp -e16Mi -l.20)\n");
		fprintf(stderr, "      ---------- parameters ----------\n");
		fprintf(stderr, "  -e  max entries (keys) <int>, default %u\n", DEFAULT_ENT);
		fprintf(stderr, "  -l  load factor <float>, default %.4f\n", DEFAULT_LOAD);
		fprintf(stderr, "  -b  block length 2^n <int>, default: n=%u\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -k  n-gram length <int>, default %d\n", DEFAULT_KLEN);
		fprintf(stderr, "  -h  hit ratio <float>, default %.4f\n", DEFAULT_HITR);
		fprintf(stderr, "  -z  zipf skew <float>, default %.4f\n", DEFAULT_SKEW);
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "      ---------- operations ----------\n");
		fprintf(stderr, "  -c  add random keys\n");
		fprintf(stderr, "  -d  lookup random keys <int>, default %d\n", DEFAULT_DARG);
		fprintf(stderr, "  -q  query with file <in_file>\n");
		fprintf(stderr, "  -r  read reference <in_file>\n");
		fprintf(stderr, "  -s  save workload to file <out_file>\n");
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
	printf("n-gram length: %d\n", karg);
	printf("max entries: %u\n", earg);
	printf("key size: %lu data size: %lu\n", (ulong_t)KEY_SZ, (ulong_t)DAT_SZ);
#if defined(USE_ACC)
	printf("slot size: %lu\n", (ulong_t)sizeof(ndb_t::slot_s));
#endif // USE_ACC

	/* * * * * * * * * * Start Up * * * * * * * * * */
	tget(start);
	ndb.reserve(earg);
	nbuf = new nmer_buf_t(barg);
	tget(finish);
	printf("Startup time: %f sec\n", tsec(finish, start));

#if 0
	if (flags & CFLAG || rarg != NULL) {
		int klim = ceil(log(LOAD_MAX)/log(4));
		/* test for n-gram length too small for specified load factor */
		/* if n-gram length is too small, there are not enough permutations */
		if (karg < klim) { /* 4^karg < LOAD_MAX */
			fprintf(stderr, " -- n-gram length must be %u or greater.\n", klim);
			exit(EXIT_FAILURE);
		}
	}
#endif

	/* * * * * * * * * * Add Random Keys * * * * * * * * * */
	if (flags & CFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; /* add count */
		size_type maxl = LOAD_MAX; /* max load count */
		RAND_VAR;

		if (flags & PFLAG) fprintf(stderr, "add");
		RAND_SEED(42);
		tinsert = tlookup = 0;
		nbuf->clear_counts();
		ndb.clear_time();
		// CLOCKS_EMULATE
		// CACHE_BARRIER(ndb.acc)
		// TRACE_START
		// STATS_START

		tget(start);
		/* Add n-grams until the specified load factor is reached */
		/* The first test is faster, the second is more accurate. */
		while (LOAD_COUNT < maxl || ndb.load_factor() < larg) {
			ngram_t ngram = RAND_GEN;
			sid_t sid = ++acount;
			nbuf->add(ngram, sid);
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		nbuf->flush_add();
		CACHE_SEND_ALL(ndb.acc)
		tget(finish);

		// CACHE_BARRIER(ndb.acc)
		// STATS_STOP
		// TRACE_STOP
		// CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Insert count: %lu\n", (ulong_t)acount);
		printf("Insert  rate: %f ops/sec\n", acount/tvesec(tinsert));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Insert  time: %f sec\n", tvesec(tinsert));
		ndb.print_time();
		// STATS_PRINT
	}

	/* * * * * * * * * * Read string reference * * * * * * * * * */
	if (rarg != NULL) {
		std::ifstream fin(rarg);
		entry_t entry;
		int i;
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; /* add count */
		size_type tcount = 0; /* total bases count */
		size_type maxl = LOAD_MAX; /* max load count */

		if (!fin.is_open() && !fin.good()) {
			fprintf(stderr, " -- can't open file: %s\n", rarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "read");
		tinsert = tlookup = 0;
		nbuf->clear_counts();
		ndb.clear_time();
		// CLOCKS_EMULATE
		// CACHE_BARRIER(ndb.acc)
		// TRACE_START
		// STATS_START

		tget(start);
		for (i = 1;; i++) {
			fin >> entry;
			if (!entry.entry_len) { break; }
			acount += str_add(entry.str, entry.entry_len, karg, i);
			tcount += entry.entry_len;
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
			free(entry.str);
			/* Add n-grams until the specified load factor is reached */
			/* The first test is faster, the second is more accurate. */
			if (LOAD_COUNT >= maxl && ndb.load_factor() >= larg) break;
		}
		nbuf->flush_add();
		CACHE_SEND_ALL(ndb.acc)
		tget(finish);

		// CACHE_BARRIER(ndb.acc)
		// STATS_STOP
		// TRACE_STOP
		// CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Insert count: %lu\n", (ulong_t)acount);
		printf("Insert  rate: %f ops/sec\n", acount/tvesec(tinsert));
		printf("Byte    rate: %f b/sec\n", tcount/tvesec(trun));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Insert  time: %f sec\n", tvesec(tinsert));
		ndb.print_time();
		// STATS_PRINT
		fin.close();
		if (ndb.load_factor() < larg) {
			fprintf(stderr, " -- warning: did not reach load factor: %.4f\n", larg);
		}
	}

	/* * * * * * * * * * Display Database Stats * * * * * * * * * */
	if (flags & CFLAG || rarg != NULL) {
		if (flags & PFLAG) fprintf(stderr, "gather stats...\n");

		tget(start);
		ndb.print_stats();
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
		nbuf->clear_counts();
		ndb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(ndb.acc)
		TRACE_START
		STATS_START

		tget(start);
//		while (lcount < ndb.size()) {
		while (lcount < darg) {
			ngram_t ngram = RAND_GEN;
			nbuf->lookup(ngram);
			lcount++;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		nbuf->flush_lookup();
		tget(finish);

		CACHE_BARRIER(ndb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)nbuf->hits, (double)nbuf->hits/lcount*100.0);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		ndb.print_time();
		STATS_PRINT
	}

	/* * * * * * * * * * Query Database * * * * * * * * * */
	if (qarg != NULL) {
		std::ifstream fin(qarg);
		entry_t entry;
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; /* lookup count */
		size_type tcount = 0; /* total bases count */


		if (!fin.is_open() && !fin.good()) {
			fprintf(stderr, " -- can't open file: %s\n", qarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "query");
		tinsert = tlookup = 0;
		nbuf->clear_counts();
		ndb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(ndb.acc)
		TRACE_START
		STATS_START

		tget(start);
		while (true) {
			fin >> entry;
			if (!entry.entry_len) { break; }
			lcount += str_lookup(entry.str, entry.entry_len, karg);
			tcount += entry.entry_len;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
			free(entry.str);
		}
		nbuf->flush_lookup();
		tget(finish);

		CACHE_BARRIER(ndb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)nbuf->hits, (double)nbuf->hits/lcount*100.0);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
		printf("Byte    rate: %f b/sec\n", tcount/tvesec(trun));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		ndb.print_time();
		STATS_PRINT
		fin.close();
	}

	/* * * * * * * * * * Workload * * * * * * * * * */
	if (warg && ndb.size()) {
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
			size_type zN = ndb.size(); /* Zipf N, dictionary size  */
			if (zN > 10000000U) zN = 10000000U;
			if (flags & PFLAG) fprintf(stderr, "calc zeta... ");
			for (i = 1; i <= zN; i++) zeta += 1.0/pow((double)i, zarg);
			if (flags & PFLAG) fprintf(stderr, "%f\n", zeta);
		}

		/* generate misses by checking if random keys are in ndb */
		if (flags & PFLAG) fprintf(stderr, "generate misses...\n");
		wptr = wload;
		samples = (1.0-harg)*warg+0.5;
		if (samples > warg) samples = warg;
		rank = 1;
		for (i = 0; i < samples; i++) {
			/* generate a random item */
			do {
				temp = RAND_GEN;
			} while (ndb.count(temp));
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

		/* generate hits by randomly selecting items from ndb */
		if (flags & PFLAG) fprintf(stderr, "generate hits...\n");
		wptr = wload + i;
		samples = warg - i;
		rank = 1;
		for (i = 0; i < samples; i++) {
			size_type bucket, steps;
			const_local_iterator clit;
			/* pick a random bucket until a non-empty one is found */
			do {
				bucket = rand() % ndb.bucket_count();
				clit = ndb.cbegin(bucket);
			} while (clit == ndb.cend(bucket));
			/* pick a random item from the bucket */
			for (steps = rand() % ndb.bucket_size(bucket); steps; steps--) clit++;
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
			std::ofstream fout(sarg);

			char str[256];
			entry_t entry = {str, 0};
			size_type pcount = PROGRESS_COUNT-1;
			size_type scount; /* save count */

			if (!fout.is_open() && !fout.good()) {
				fprintf(stderr, " -- can't open file: %s\n", sarg);
				exit(EXIT_FAILURE);
			}

			if (flags & PFLAG) fprintf(stderr, "save workload");
			for (scount = 0; scount < warg; scount++) {
				entry.entry_len = ktob(wload[scount], karg, entry.str);
				fout << entry << std::endl;
				if ((flags & PFLAG) && scount >= pcount) {
					fputc('.', stderr);
					pcount += PROGRESS_COUNT;
				}
			}
			if (flags & PFLAG) fputc('\n', stderr);

			fout.close();
		}

		/* do lookup */
		if (flags & PFLAG) fprintf(stderr, "workload");
		tinsert = tlookup = 0;
		nbuf->clear_counts();
		ndb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(ndb.acc)
		TRACE_START
		STATS_START

#if 1
		(void)pcount; /* silence warning */
		if (flags & PFLAG) fprintf(stderr, "...");
		tget(start);
		nbuf->block_lookup(wload, warg);
		lcount = warg;
		tget(finish);
#else
		tget(start);
		while (lcount < warg) {
			nbuf->lookup(wload[lcount]);
			lcount++;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		nbuf->flush_lookup();
		tget(finish);
#endif

		CACHE_BARRIER(ndb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);

		trun = tdiff(finish, start);
		toper = trun-tinsert-tlookup;
		printf("Lookup count: %lu\n", (ulong_t)lcount);
		printf("Lookup  hits: %lu %.2f%%\n", (ulong_t)nbuf->hits, (double)nbuf->hits/lcount*100.0);
		printf("Lookup  zipf: %.2f\n", zarg);
		printf("Lookup  rate: %f ops/sec\n", lcount/tvesec(tlookup));
		printf("Run     time: %f sec\n", tvesec(trun));
		printf("Oper.   time: %f sec\n", tvesec(toper));
		printf("Lookup  time: %f sec\n", tvesec(tlookup));
		ndb.print_time();
		STATS_PRINT
	}

	/* * * * * * * * * * Wrap Up * * * * * * * * * */
	delete nbuf;

	TRACE_CAP
	return EXIT_SUCCESS;
}

/* TODO:
 * Fix reaching load factor
 * Verify uniform and Zipf distributions
*/

/* DONE:
*/
