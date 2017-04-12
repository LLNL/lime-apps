/*
$Id: $

Description: read test bench

$Log: $
*/

#define ALIGN_SZ 256
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h> /* malloc, free, exit, rand, atoi, atof, strtoul */
#include <ctype.h> /* toupper, tolower, isalpha, isspace, isdigit, isgraph */
#include <string.h> /* strcmp, strlen, memset, memcmp */

#include "fasta.h"
#include "path.h"

#include "config.h"
#include "alloc.h"
#include "cache.h"
#include "monitor.h"
#include "ticks.h"
#include "clocks.h"

#include "block_map.h"

// Arguments when STANDALONE
// #define ARGS (char*)"-ab"
// #define ARGS (char*)"-e1K", (char*)"-k16", (char*)"-rtestdb.fa", (char*)"-qtestqr.fa"
// #define ARGS (char*)"-p", (char*)"-e32Mi", (char*)"-l.60", (char*)"-rsrr_nr.fa", (char*)"-qsrr_sh.fa"
#define ARGS (char*)"-p", (char*)"-e32Mi", (char*)"-l.20", (char*)"-rsrr_nr.fa", (char*)"-qsrr_sh.fa"
// #define ARGS (char*)"-p", (char*)"-e32Mi", (char*)"-l.20", (char*)"-a", (char*)"-qsrr_sh.fa"
// #define ARGS (char*)"-e8Mi", (char*)"-ramr_cur.fa", (char*)"-qamr_cur.fa"
// #define ARGS (char*)"-e8Mi", (char*)"-ramr_cur.fa", (char*)"-qsrr_sh1.fa"

#define DEFAULT_ENT 100000
#define DEFAULT_KLEN 18
#define DEFAULT_LOAD 0.95
#define DEFAULT_BLOCK_LSZ 10 // log 2 size

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
float larg = DEFAULT_LOAD; /* load factor */
unsigned int earg = DEFAULT_ENT; /* maximum entries (keys) */
int karg = DEFAULT_KLEN; /* k-mer length */
unsigned barg = 1U<<DEFAULT_BLOCK_LSZ; /* buffer block length */
char *qarg = NULL; /* query file name (in) */
char *rarg = NULL; /* reference file name (in) */
#if defined(ENABLE_PLOT)
int xarg = 0; /* plot x range */
char *garg = NULL; /* plot command file name (out) */
char *targ = NULL; /* plot terminal file name (out) */
#endif // ENABLE_PLOT

// TODO: find a better place for these globals

#if defined(STATS) || defined(TRACE) 
XAxiPmon apm;
#endif // STATS || TRACE

typedef block_multimap<kmer_t,sid_t> kdb_t;
typedef typename kdb_t::size_type size_type;
kdb_t kdb;

#define LOAD_COUNT kdb.size()

struct {
	size_t length; /* length of buffers */
	size_t lcount; /* count of items in lookup buffers */
	size_t acount; /* count of items in add buffer */

	ulong_t lookups; /* number of lookups */
	ulong_t hits; /* number of search hits */

	typedef typename kdb_t::key_type key_t;
#if defined(USE_ACC)
	typedef typename kdb_t::mapped_type* value_t;
	inline bool is_valid_value(const value_t &v) {return v != nullptr;}
#else // USE_ACC
	typedef std::pair<typename kdb_t::iterator, typename kdb_t::iterator> value_t;
	inline bool is_valid_value(const value_t &v) {return v.first != kdb.end();}
#endif // USE_ACC
	typedef std::pair<typename kdb_t::key_type, typename kdb_t::mapped_type> kvpair_t;
	// typedef typename kdb_t::value_type kvpair_t; // has const key_type

	key_t* key;
	value_t* value;
	kvpair_t* kvpair;

	void clear_counts(void)
	{
		lookups = 0;
		hits = 0;
	}

	void init(size_t buf_len)
	{
		length = buf_len;
		acount = 0;
		lcount = 0;

		key = SP_NALLOC(key_t, length); // used on lookup
		chk_alloc(key, sizeof(key_t)*length, "SP_NALLOC key in init()");

		value = SP_NALLOC(value_t, length); // used on lookup
		chk_alloc(value, sizeof(value_t)*length, "SP_NALLOC value in init()");

		kvpair = SP_NALLOC(kvpair_t, length); // used on add
		chk_alloc(kvpair, sizeof(kvpair_t)*length, "SP_NALLOC kvpair in init()");

	}

#if defined(NO_BUFFER)

	inline void lookup(kmer_t kmer)
	{
		lookups++;
		if (kdb.equal_range(kmer).first != kdb.end()) hits++;
	}

	inline void add(kmer_t kmer, sid_t sid)
	{
		kdb.insert({kmer, sid});
	}

#else

	// TODO: use std::container to emplace
	inline void lookup(kmer_t kmer)
	{
		if (lcount == length) flush_lookup();
		key[lcount++] = kmer;
	}

	// TODO: use std::container to emplace
	inline void add(kmer_t kmer, sid_t sid)
	{
		if (acount == length) flush_add();
		kvpair[acount++] = {kmer, sid};
	}

#endif // NO_BUFFER

	void flush_lookup(void)
	{
		if (lcount == 0) return;
		kdb.equal_range(value, key, lcount);
		lookups += lcount;
#if 1 // TODO: time without
		for (size_t i = 0; i < lcount; i++) {
			if (is_valid_value(value[i])) hits++;
		}
#endif
		lcount = 0;
	}

	void flush_add(void)
	{
		if (acount == 0) return;
		kdb.insert(reinterpret_cast<typename kdb_t::const_pointer>(kvpair), acount);
		acount = 0;
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
}

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
	kmer_t mask = ((kmer_t)1 << klen*2)-1; /* bits covering encoded k-mer */
	kmer_t forward = 0; /* forward k-mer */
	kmer_t reverse = 0; /* reverse k-mer */
	kmer_t kmer; /* canonical k-mer */

	// fprintf(stderr, "str:%s\n", str);
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
				fprintf(stderr, "forward:%s pos:%d\n", buf, j-klen+1);
				kton(reverse, klen, buf);
				fprintf(stderr, "reverse:%s pos:%d\n", buf, slen-j-1);
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
	kmer_t mask = ((kmer_t)1 << klen*2)-1; /* bits covering encoded k-mer */
	kmer_t forward = 0; /* forward k-mer */
	kmer_t reverse = 0; /* reverse k-mer */
	kmer_t kmer; /* canonical k-mer */

	// fprintf(stderr, "str:%s\n", str);
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
				fprintf(stderr, "forward:%s pos:%d\n", buf, j-klen+1);
				kton(reverse, klen, buf);
				fprintf(stderr, "reverse:%s pos:%d\n", buf, slen-j-1);
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


int main(int argc, char *argv[])
{
	int nok = 0;
	char *s;

#ifdef STANDALONE
	char *args[] = {(char*)__FILE__, ARGS, 0};
	argc = sizeof(args)/sizeof(char *)-1;
	argv = args;
#endif

	host::cache_init();
	MONITOR_INIT
#if defined(USE_ACC)
	kdb.acc.wait(); // wait for accelerator initialization
#endif // USE_ACC
	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s; s++)
			switch (*s) {
			/* * * * * storage options * * * * */
			case 'e':
				if (isdigit(s[1])) {
					char *eptr;
					unsigned int k;
					earg = strtoul(s+1, &eptr, 0);
					k = (isalpha(eptr[0]) && toupper(eptr[1]) == 'I') ? 1024 : 1000;
					switch (toupper(*eptr)) {
					case 'K': earg *= k; break;
					case 'M': earg *= k*k; break;
					case 'G': earg *= k*k*k; break;
					}
				} else nok = 1;
				s += strlen(s+1);
				break;
			case 'l':
				if (s[1] == '.' || isdigit(s[1])) larg = atof(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 'v':
				barg = (1U << atoi(s+1));
				if (barg < 2) {
					fprintf(stderr, "block size must be 2 or greater.\n");
					nok = true;
				}
				break;
			case 'p':
				flags |= PFLAG;
				break;
			/* * * * * input options * * * * */
			case 'c':
				flags |= CFLAG;
				break;
			case 'd':
				flags |= DFLAG;
				break;
			case 'k':
				if (isdigit(s[1])) karg = atoi(s+1);
				else nok = 1;
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

	if (nok || argc > 0) { // (argc > 0 && *argv[0] == '?')
		fprintf(stderr, "Usage: rtb -cdp -e<int> -l<float> -b<int> -k<int> -r<in_file> -q<in_file>\n");
		fprintf(stderr, "  -e  max entries (keys) <int>, default %u\n", DEFAULT_ENT);
		fprintf(stderr, "  -l  load factor <float>, default %.4f\n", DEFAULT_LOAD);
		fprintf(stderr, "  -b  block length 2^n, default: n=%u\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "      ----------\n");
		fprintf(stderr, "  -c  add random keys\n");
		fprintf(stderr, "  -d  lookup random keys\n");
		fprintf(stderr, "  -k  k-mer length <int>, default %d\n", DEFAULT_KLEN);
		fprintf(stderr, "  -q  query with FASTA file <in_file>\n");
		fprintf(stderr, "  -r  read FASTA reference <in_file>\n");
		fprintf(stderr, "      ----------\n");
#if defined(ENABLE_PLOT)
		fprintf(stderr, "  -g  plot command <out_file>\n");
		fprintf(stderr, "  -t  plot terminal <out_file>\n");
		fprintf(stderr, "  -x  plot x range <int>, default auto\n");
#endif // ENABLE_PLOT
		exit(EXIT_FAILURE);
	}
#if defined(USE_SP)
	if (barg > SP_SIZE/sizeof(unsigned)/8) barg = SP_SIZE/sizeof(unsigned)/8; /* up to 1/8th of scratchpad */
#endif
	printf("block length:%u\n", barg);
	printf("k-mer length:%d\n", karg);
	printf("max entries:%u\n", earg);
	printf("key size:%lu data size:%lu\n", (ulong_t)KEY_SZ, (ulong_t)DAT_SZ);

	/* * * * * * * * * * Start Up * * * * * * * * * */
	kdb.reserve(earg);
	kbuf.init(barg);

	/* * * * * * * * * * Add Random Keys * * * * * * * * * */
	if (flags & CFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; // add count
		size_type tcount = 0; // total bases count
		size_type maxl = earg * larg; // max load count
		kmer_t mask = ((kmer_t)1 << karg*2) - 1;
		tick_t start, finish;

		srand(42);
		if (flags & PFLAG) fprintf(stderr, "add");

		tget(start);
		while (LOAD_COUNT < maxl) { /* reach load factor? */
			kmer_t kmer = (((kmer_t)rand() << sizeof(int)*8) ^ rand()) & mask;
			sid_t sid = ++acount;
			tcount += karg;
			kbuf.add(kmer, sid);
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kbuf.flush_add();
		// CACHE_SEND_ALL(kdb.acc)
		tget(finish);

		if (flags & PFLAG) fputc('\n', stderr);
		printf("bases per second:%f\n", tcount/tsec(finish, start));
		printf("add time in seconds:%f\n", tsec(finish, start));
		printf("adds per second:%f\n", acount/tsec(finish, start));
	}

	/* * * * * * * * * * Read FASTA Reference * * * * * * * * * */
	if (rarg != NULL) {
		FILE *fin;
		sequence entry;
		int i, len;
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; // add count
		size_type tcount = 0; // total bases count
		size_type maxl = earg * larg; // max load count
		tick_t start, finish;

		if ((fin = fopen(rarg, "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", rarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "read");
		kdb.clear_time();
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		// TRACE_START
		STATS_START

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
			if (LOAD_COUNT >= maxl) break; /* reach load factor? */
		}
		kbuf.flush_add();
		// CACHE_SEND_ALL(kdb.acc)
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		// TRACE_STOP
		CLOCKS_NORMAL
		if (flags & PFLAG) fputc('\n', stderr);
		printf("bases per second:%f\n", tcount/tesec(finish, start));
		printf("add time in seconds:%f\n", tesec(finish, start));
		printf("adds per second:%f\n", acount/tesec(finish, start));
		kdb.print_time(tdiff(finish,start));
		STATS_PRINT
		fclose(fin);
	}

	/* * * * * * * * * * Display Database Stats * * * * * * * * * */
	if (flags & CFLAG || rarg != NULL) {
		tick_t start, finish;

		if (flags & PFLAG) fprintf(stderr, "gather stats...\n");

		tget(start);
		kdb.print_stats();
		SHOW_HEAP
		tget(finish);
		printf("stats time in seconds:%f\n", tsec(finish, start));
	}

	/* * * * * * * * * * Lookup & Query Init * * * * * * * * * */
	kbuf.clear_counts();

	/* * * * * * * * * * Lookup Random Keys * * * * * * * * * */
	if (flags & DFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; // lookup count
		size_type tcount = 0; // total bases count
		kmer_t mask = ((kmer_t)1 << karg*2) - 1;
		tick_t start, finish;

		srand(42);
		if (flags & PFLAG) fprintf(stderr, "lookup");

		tget(start);
		while (lcount < kdb.size()) {
			kmer_t kmer = (((kmer_t)rand() << sizeof(int)*8) ^ rand()) & mask;
			kbuf.lookup(kmer);
			lcount++;
			tcount += karg;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kbuf.flush_lookup();
		tget(finish);

		if (flags & PFLAG) fputc('\n', stderr);
		printf("bases per second:%f\n", tcount/tsec(finish, start));
		printf("lookup time in seconds:%f\n", tsec(finish, start));
		printf("lookups per second:%f\n", lcount/tsec(finish, start));
	}

	/* * * * * * * * * * Query Database * * * * * * * * * */
	if (qarg != NULL) {
		FILE *fin;
		sequence entry;
		int len;
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; // lookup count
		size_type tcount = 0; // total bases count
		tick_t start, finish;

		if ((fin = fopen(qarg, "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", qarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "query");
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
		printf("bases per second:%f\n", tcount/tesec(finish, start));
		printf("query time in seconds:%f\n", tesec(finish, start));
		printf("lookups per second:%f\n", lcount/tesec(finish, start));
		kdb.print_time(tdiff(finish,start));
		STATS_PRINT
		fclose(fin);
	}

	/* * * * * * * * * * Display Query Stats * * * * * * * * * */
	if (flags & DFLAG || qarg != NULL) {
		printf("lookups:%lu hits:%lu %.2f%%\n",
			(ulong_t)kbuf.lookups, (ulong_t)kbuf.hits,
			(double)kbuf.hits/kbuf.lookups*100.0);
	}

	/* * * * * * * * * * Wrap Up * * * * * * * * * */

	TRACE_CAP
	return EXIT_SUCCESS;
}

/* TODO:
*/

/* DONE:
*/
