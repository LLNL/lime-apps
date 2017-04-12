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
#include <stdint.h> /* uintXX_t */

#include "fasta.h"
#include "path.h"

#include "config.h"
#include "alloc.h"
#include "cache.h"
#include "monitor.h"
#include "ticks.h"
#include "clocks.h"

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
#define DEFAULT_BLOCK_LSZ 12 // log 2 size

#define AFLAG 0x01
#define BFLAG 0x02
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
unsigned varg = 1U<<DEFAULT_BLOCK_LSZ; /* view buffer block size */
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

tick_t t0, t1, t2, t2a, t2b, t2c, t3, t4, t5, t6, t7, t8, t9, t10;
unsigned long long tsetup, tfill, tdrain, tinsert, toper, tcache;
unsigned long long taccel, tscan, tcacheA;
unsigned long long tadd, tmov;

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

/*----------- kmer store beg -----------*/

// The major differences of block_multimap from unordered_multimap are:
// 1) The search and store interface is block oriented through
//    arrays of elements.
// 2) Keys and values are not paired in the block_multimap class, but
//    rather are stored in separate arrays facilitating DMA access.

#include "KVstore.hpp"
#if defined(ADD_MAP)
#include <unordered_map>
#endif
#if defined(ADD_PRIORITY)
#include <queue>
#endif
#include <utility> // std::pair

#define CHUNK_SZ 4
#define MAX_SID 8
#define restrict __restrict__

// TODO: move to block_multimap.h, block_multimap_acc.cpp, block_multimap_std.cpp
template<class _Key, class _Tp //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
	>
class block_multimap {
public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<key_type, mapped_type> value_type;
	typedef size_t size_type;

	typedef mapped_type *mapped_pointer;
	// NOTE: separate from KVstore since it may have different architecture
	typedef unsigned int status_type;

	KVstore<key_type, mapped_pointer> acc; // Key-Value Accelerator
#if defined(ADD_PRIORITY)
	std::priority_queue<key_type> pque;
#endif

	// use array of struct {key, value, data}?
	// TODO: make two key arrays with separate counts, one for lookup and one for add
	key_type * restrict key; // key array base address, used during lookup & add
	mapped_pointer * restrict value; // value array base address, result of lookup
	value_type *kvpair; // kvpair array base address, used during add
	// mapped_type *data; // data array base address, used during add
	size_type length; // array length
	size_type count; // count of entries in key, value, kvpair & data arrays
	size_type key_sz; // key size in bytes

	status_type * restrict status; // status array base address
	status_type elements; // number of elements
	status_type maxelem; // maximum number of elements per key

	size_type lookups; /* number of lookups */
	size_type hits; /* number of search hits */

	/* TODO:
	// keep counts internally

	iterator
	insert(const value_type& __x)
	{ return ; }

	// for block insert with array of pair<key,data>, no status
	void
	insert(pointer __arr, size_type __n) // const_pointer?
	{ ; }

	*/

	/* TODO:

	std::pair<iterator, iterator>
	equal_range(const key_type& __x)
	{ return ; }

	// consider with non-standard range (mapped_type* __rng)
	void
	equal_range(
		std::pair<iterator, iterator>* __rng,
		const key_type* __x,
		size_type __n)
	{ ; }

	*/

	/* TODO:

	iterator
	find(const key_type& __x)
	{ return ; }

	void
	find(iterator* __fnd, const key_type* __x)
	{ ; }

	*/

	size_type keys() const noexcept // number of keys
	{
		return acc.elements;
	}

	size_type max_elem_per_key() const noexcept // maximum number of elements per key
	{
		return maxelem;
	}

	size_type max_psl() const noexcept // maximum probe sequence length
	{
		return acc.topsearch;
	}

	size_type size() const noexcept // number of elements
	{
		return elements;
	}

	size_type bucket_used() const noexcept // number of buckets used
	{
		return acc.elements;
	}

	size_type bucket_count() const noexcept // number of buckets total
	{
		return acc.data_len; // acc.data_len + acc.topsearch?
	}

	float load_factor() const noexcept // load_factor = slots used / slots total
	{
		return (float)acc.elements/acc.data_len;
	}

	void clear_counts(void)
	{
		lookups = 0;
		hits = 0;
	}

	// TODO: move out of block_multimap
	void init(size_type elements, size_type key_size, size_type block_sz)
	{
		key_sz = key_size;
		length = block_sz/sizeof(mapped_pointer);
		count = 0;

		key = SP_NALLOC(key_type, length); // used on lookup
		chk_alloc(key, sizeof(key_type)*length, "SP_NALLOC key in init()");

		value = SP_NALLOC(mapped_pointer, length); // used on lookup
		chk_alloc(value, sizeof(mapped_pointer)*length, "SP_NALLOC value in init()");

		kvpair = SP_NALLOC(value_type, length); // used on add
		chk_alloc(kvpair, sizeof(value_type)*length, "SP_NALLOC kvpair in init()");

		// data = SP_NALLOC(mapped_type, length); // used on add, use malloc?
		// chk_alloc(data, sizeof(mapped_type)*length, "SP_NALLOC data in init()");

		status = SP_NALLOC(status_type, length+1);
		chk_alloc(status, sizeof(status_type)*length, "SP_NALLOC status in init()");

		tget(t0);
		CACHE_INVALIDATE(acc, key, sizeof(key_type)*length);
		CACHE_INVALIDATE(acc, value, sizeof(mapped_pointer)*length);
		CACHE_INVALIDATE(acc, kvpair, sizeof(value_type)*length);
		// CACHE_INVALIDATE(acc, data, sizeof(mapped_type)*length);
		CACHE_INVALIDATE(acc, status, sizeof(status_type)*(length+1));
		tget(t1);
		acc.setup(elements, key, length, key_sz, status);
		tget(t2);
		tinc(tsetup, tdiff(t2,t1));
	}

	// TODO: move out of block_multimap or use std::container to emplace
	void lookup(key_type kmer)
	{
		if (count == length) execute_lookup();
		key[count++] = kmer;
	}

	// TODO: move out of block_multimap or use std::container to emplace
	void add(key_type kmer, mapped_type sid)
	{
		size_type lo, hi, mi;

		if (count == length) execute_add();
#if defined(ADD_INSERT)
		tget(t2a);
		lo = 0; hi = count;
		for (;;) {
			mi = (lo + hi) >> 1;
			//printf("lo:%u hi:%u mi:%u\n", lo, hi, mi);
			if (lo == hi) break;
			if (kmer > key[mi]) {lo = mi+1; continue;}
			if (kmer < key[mi]) {hi = mi;   continue;}
			break;
		}
		//printf("mi:%u ct:%u\n", mi, count);
		tget(t2b);
		if (mi < count) {
			std::copy_backward(&key[mi], &key[count], &key[count+1]);
			std::copy_backward(&data[mi], &data[count], &data[count+1]);
			// memmove(&key[mi+1], &key[mi], sizeof(key_type)*(count-mi));
			// memmove(&data[mi+1], &data[mi], sizeof(mapped_type)*(count-mi));
		}
		tget(t2c);
		key[mi] = kmer;
		data[mi] = sid;
		count++;
		tinc(tadd, tdiff(t2b,t2a));
		tinc(tmov, tdiff(t2c,t2b));
#elif defined(ADD_PRIORITY)
		// tget(t2b);
		pque.push(kmer);
		data[count++] = sid; // FIXME: data out of order;
		// tget(t2c);
		// tinc(tmov, tdiff(t2c,t2b));
#elif 0 && defined(ADD_HEAP)
		tget(t2b);
		key[count] = kmer;
		data[count++] = sid; // FIXME: data out of order;
		// std::push_heap(&key[0], &key[count]);
		tget(t2c);
		tinc(tmov, tdiff(t2c,t2b));
#else
		// key[count] = kmer;
		// data[count++] = sid;
		kvpair[count++] = std::make_pair(kmer, sid); //{kmer, sid};
#endif
	}

	void execute_lookup(void)
	{
		if (count == 0) return;
		tget(t3);
		CACHE_SEND(acc, key, sizeof(key_type)*count);
		tget(t4);
		acc.fill(value, sizeof(mapped_pointer)*count, key, sizeof(key_type));
		tget(t5);
		CACHE_RECV(acc, value, sizeof(mapped_pointer)*count);
		// CACHE_RECV(acc, status, sizeof(status_type)*(count+1));
		tget(t6);
		lookups += count;
		// hits += count - status[0];
		for (size_type i = 0; i < count; i++) {
			if (value[i] != NULL) hits++;
		}
#if 0
		for (size_type i = 0; i < count; i++) {
			char buf[64];
			kton(key[i], karg, buf);
			if (value[i] != NULL) {
				fprintf(stderr, "k-mer[%02d]:%s elems:%d\n", i, buf, value[i][0]);
			} else {
				fprintf(stderr, "k-mer[%02d]:%s elems:null\n", i, buf);
			}
		}
#endif
		count = 0;
		tinc(tcache, tdiff(t4,t3) + tdiff(t6,t5));
		tinc(tfill, tdiff(t5,t4));
	}

	void execute_add(void)
	{
		size_type i, j;

		// TODO: Only write back unique/changed values (mapped_pointers)?
		if (count == 0) return;
#if defined(ADD_SORT)
		tget(t2a);
		// FIXME: data out of order; include data with key in a structure? shadow sort? use <key, dptr>, symmetric with read?
		// std::sort(key, key+count); // uses combination of quicksort, heapsort, and insertion sort
		std::sort(kvpair, kvpair+count); // uses combination of quicksort, heapsort, and insertion sort
#elif defined(ADD_MAP)
		// Use a map to keep track of duplicate keys in the array.
		// A CAM would be efficient at keeping track of duplicate keys.
		tget(t2a);
		typedef std::unordered_multimap<key_type, size_type> umm_t;
		umm_t umm;
		for (i = 0; i < count; i++) umm.emplace(key[i], i);
#elif defined(ADD_PRIORITY)
		tget(t2a);
		for (i = 0; i < count; i++) {key[i] = pque.top(); pque.pop();}
#elif defined(ADD_HEAP)
		tget(t2a);
		std::make_heap(&key[0], &key[count]);
		std::sort_heap(&key[0], &key[count]);
#endif
		tget(t3);
		CACHE_SEND(acc, kvpair, sizeof(value_type)*count);
		tget(t4);
		acc.fill(value, sizeof(mapped_pointer)*count, kvpair, sizeof(value_type));
		tget(t5);
		CACHE_RECV(acc, value, sizeof(mapped_pointer)*count);
		// CACHE_RECV(acc, status, sizeof(status_type)*(count+1));
		tget(t6);

// fprintf(stderr, " add data:");
		for (i = 0; i < count; i++) {
			// fprintf(stderr, "\n i:%04u key:%010llX pval1:%10p", i, key[i], value[i]);
#if defined(ADD_MAP)
			auto update_val = [this,i](typename umm_t::value_type &x) {if (i != x.second) value[x.second] = value[i]; /*printf("key[%u]:%010llX x.key:%010llX x.idx:%u\n", i, key[i], x.first, x.second);*/};
#endif
			if (value[i] == NULL) {
				value[i] = (mapped_type *)malloc(sizeof(mapped_type)*CHUNK_SZ);
				chk_alloc(value[i], sizeof(mapped_type)*CHUNK_SZ, "malloc in execute_add()");
#if defined(ADD_SORT) || defined(ADD_INSERT) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
				// Replace all occurrences of value[] that have the same key
				for (j = i+1; j < count && kvpair[j].first == kvpair[i].first; j++) {
					value[j] = value[i];
				}
#elif defined(ADD_MAP)
				auto range = umm.equal_range(key[i]);
				for_each (range.first, range.second, update_val);
#else
				// Replace all occurrences of value[] that have the same key
				for (j = 0; j < count; j++) {
					if (j == i) continue;
					// if (memcmp(&key[j], &key[i], key_sz) == 0) value[j] = value[i];
					if (key[j] == key[i]) value[j] = value[i];
				}
#endif
				value[i][0] = 1;
				if (maxelem < 1) maxelem = 1;
				value[i][1] = kvpair[i].second;
				continue;
			}
			if (++value[i][0] > maxelem) maxelem = value[i][0];
			if (value[i][0] >= MAX_SID) continue;
			if (value[i][0] % CHUNK_SZ == 0) {
				unsigned chunks = value[i][0] / CHUNK_SZ;
				// fprintf(stderr, " p1:%10p", value[i]);
				value[i] = (mapped_type *)realloc(value[i], sizeof(mapped_type)*CHUNK_SZ*(chunks+1));
				chk_alloc(value[i], sizeof(mapped_type)*CHUNK_SZ*(chunks+1), "realloc in execute_add()");
				// fprintf(stderr, " p2:%10p", value[i]);
#if defined(ADD_SORT) || defined(ADD_INSERT) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
				// Replace all occurrences of value[] that have the same key
				for (j = i; j > 0 && kvpair[j-1].first == kvpair[i].first; j--) {
					value[j-1] = value[i];
				}
				for (j = i+1; j < count && kvpair[j].first == kvpair[i].first; j++) {
					value[j] = value[i];
				}
#elif defined(ADD_MAP)
				auto range = umm.equal_range(key[i]);
				for_each (range.first, range.second, update_val);
#else
				// Replace all occurrences of value[] that have the same key
				for (j = 0; j < count; j++) {
					if (j == i) continue;
					// if (memcmp(&key[j], &key[i], key_sz) == 0) value[j] = value[i];
					if (key[j] == key[i]) value[j] = value[i];
				}
#endif
			}
			value[i][value[i][0]] = kvpair[i].second;
			// fprintf(stderr, " pval2:%10p nelem:%02u data:%06u", value[i], value[i][0], data[i]);
		}
		elements += count;
		tget(t7);
		CACHE_SEND(acc, value, sizeof(mapped_pointer)*count);
		tget(t8);
		acc.drain(value, sizeof(mapped_pointer)*count, kvpair, sizeof(value_type));
		tget(t9);
		// CACHE_RECV(acc, status, sizeof(status_type)*(count+1));
		tget(t10);
		count = 0;
		tinc(tcache, tdiff(t4,t3) + tdiff(t6,t5) + tdiff(t8,t7) + tdiff(t10,t9));
		tinc(tfill, tdiff(t5,t4));
		tinc(tdrain, tdiff(t9,t8));
		tinc(tinsert, tdiff(t7,t6));
#if defined(ADD_SORT) || defined(ADD_MAP) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
		tinc(tadd, tdiff(t3,t2a));
#endif
	}

}; // class block_multimap

/*----------- kmer store end -----------*/

typedef block_multimap<kmer_t,sid_t> kdb_t;
typedef typename kdb_t::size_type size_type;
kdb_t kdb;

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
			kdb.lookup(kmer);
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
			kdb.add(kmer, sid);
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
				varg = (1U << atoi(s+1));
				if (varg < 8) {
					fprintf(stderr, "view buffer block size must be 8 or greater.\n");
					nok = true;
				}
				break;
			case 'p':
				flags |= PFLAG;
				break;
			/* * * * * input options * * * * */
			case 'a':
				flags |= AFLAG;
				break;
			case 'b':
				flags |= BFLAG;
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

	if (nok || (argc > 0 && *argv[0] == '?')) {
		fprintf(stderr, "Usage: rtb -abp -e<int> -l<float> -v<int> -k<int> -r<in_file> -q<in_file>\n");
		fprintf(stderr, "  -e  max entries (keys) <int>, default %u\n", DEFAULT_ENT);
		fprintf(stderr, "  -l  load factor <float>, default %.4f\n", DEFAULT_LOAD);
		fprintf(stderr, "  -v  view buffer block size 2^n, default: n=%u\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "      ----------\n");
		fprintf(stderr, "  -a  add random keys\n");
		fprintf(stderr, "  -b  lookup random keys\n");
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
	if (varg > SP_SIZE/8) varg = SP_SIZE/8; /* up to 1/8th of scratchpad */
#endif
	printf("block size: %u\n", varg);
	printf("k-mer length:%d\n", karg);
	printf("max entries:%u\n", earg);
	printf("key size:%lu data size:%lu\n", (unsigned long)KEY_SZ, (unsigned long)DAT_SZ);

	/* * * * * * * * * * Start Up * * * * * * * * * */
	kdb.init(earg, (karg*2+7)/8, varg);

	/* * * * * * * * * * Add Random Keys * * * * * * * * * */
	if (flags & AFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type acount = 0; // add count
		size_type maxb = kdb.bucket_count() * larg;
		kmer_t mask = ((kmer_t)1 << karg*2) - 1;
		tick_t start, finish;

		srand(42);
		if (flags & PFLAG) fprintf(stderr, "add");

		tget(start);
		while (kdb.bucket_used() < maxb) { /* reach load factor? */
			kmer_t kmer = (((kmer_t)rand() << sizeof(int)*8) ^ rand()) & mask;
			sid_t sid = acount++;
			kdb.add(kmer, sid);
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kdb.execute_add();
		tget(finish);

		if (flags & PFLAG) fputc('\n', stderr);
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
		size_type maxb = kdb.bucket_count() * larg;
		tick_t start, finish;

		if ((fin = fopen(rarg, "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", rarg);
			exit(EXIT_FAILURE);
		}

		if (flags & PFLAG) fprintf(stderr, "read");
		tfill = tdrain = tinsert = tcache = toper = 0;
		taccel = tscan = tcacheA = 0;
#if defined(ADD_SORT) || defined(ADD_MAP) || defined(ADD_INSERT) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
		tadd = tmov = 0;
#endif
		CLOCKS_EMULATE
		CACHE_BARRIER(kdb.acc)
		// TRACE_START
		STATS_START

		tget(start);
		for (i = 0; (len = Fasta_Read_Entry(fin, &entry)) > 0; i++) {
			acount += seq_add(entry.str, len, karg, i);
			tcount += len;
			if ((flags & PFLAG) && acount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
			free(entry.hdr);
			free(entry.str);
			if (kdb.bucket_used() >= maxb) break; /* reach load factor? */
		}
		kdb.execute_add();
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		// TRACE_STOP
		CLOCKS_NORMAL
		toper = tdiff(finish,start)-tfill-tdrain-tinsert-tcache;
#if defined(ADD_SORT) || defined(ADD_MAP) || defined(ADD_INSERT) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
		toper -= tadd + tmov;
#endif
		if (flags & PFLAG) fputc('\n', stderr);
		printf("bases per second:%f\n", tcount/tesec(finish, start));
		printf("add time in seconds:%f\n", tesec(finish, start));
		printf("adds per second:%f\n", acount/tesec(finish, start));
		printf("Fill    time: %f sec\n", tfill/(double)TICKS_ESEC);
#if defined(USE_ACC)
		printf(" Accel  time: %f sec\n", taccel/(double)TICKS_ESEC);
		printf(" Scan   time: %f sec\n", tscan/(double)TICKS_ESEC);
		printf(" CacheA time: %f sec\n", tcacheA/(double)TICKS_ESEC);
#endif
		printf("Drain   time: %f sec\n", tdrain/(double)TICKS_ESEC);
#if defined(ADD_SORT) || defined(ADD_MAP) || defined(ADD_INSERT) || defined(ADD_PRIORITY) || defined(ADD_HEAP)
		printf("Add     time: %f sec\n", tadd/(double)TICKS_ESEC);
		printf("Move    time: %f sec\n", tmov/(double)TICKS_ESEC);
#endif
		printf("Insert  time: %f sec\n", tinsert/(double)TICKS_ESEC);
		printf("Cache   time: %f sec\n", tcache/(double)TICKS_ESEC);
		printf("Oper.   time: %f sec\n", toper/(double)TICKS_ESEC);
		STATS_PRINT
		fclose(fin);
	}

	/* * * * * * * * * * Display Database Stats * * * * * * * * * */
	if (flags & AFLAG || rarg != NULL) {
		printf("size:%lu unique:%lu duplicates:%lu %.2f%%\n",
			(ulong_t)kdb.size(), (ulong_t)kdb.keys(),
			(ulong_t)(kdb.size()-kdb.keys()),
			(double)(kdb.size()-kdb.keys())/kdb.size()*100.0);
		printf("load_factor:%f\n", kdb.load_factor());
		printf("bucket_count:%lu\n", (ulong_t)kdb.bucket_count());
		printf("max_elem_per_key:%lu\n", (ulong_t)kdb.max_elem_per_key());
		printf("max_psl:%lu\n", (ulong_t)kdb.max_psl());
		SHOW_HEAP
	}

	/* * * * * * * * * * Lookup & Query Init * * * * * * * * * */
	kdb.clear_counts();

	/* * * * * * * * * * Lookup Random Keys * * * * * * * * * */
	if (flags & BFLAG) {
		size_type pcount = PROGRESS_COUNT;
		size_type lcount = 0; // lookup count
		kmer_t mask = ((kmer_t)1 << karg*2) - 1;
		tick_t start, finish;

		srand(42);
		if (flags & PFLAG) fprintf(stderr, "lookup");

		tget(start);
		while (lcount < kdb.size()) {
			kmer_t kmer = (((kmer_t)rand() << sizeof(int)*8) ^ rand()) & mask;
			kdb.lookup(kmer);
			lcount++;
			if ((flags & PFLAG) && lcount >= pcount) {
				fputc('.', stderr);
				pcount += PROGRESS_COUNT;
			}
		}
		kdb.execute_lookup();
		tget(finish);

		if (flags & PFLAG) fputc('\n', stderr);
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
		tfill = tdrain = tinsert = tcache = toper = 0;
		taccel = tscan = tcacheA = 0;
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
		kdb.execute_lookup();
		tget(finish);

		CACHE_BARRIER(kdb.acc)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		toper = tdiff(finish,start)-tfill-tdrain-tinsert-tcache;
		if (flags & PFLAG) fputc('\n', stderr);
		printf("bases per second:%f\n", tcount/tesec(finish, start));
		printf("query time in seconds:%f\n", tesec(finish, start));
		printf("lookups per second:%f\n", lcount/tesec(finish, start));
		printf("Fill    time: %f sec\n", tfill/(double)TICKS_ESEC);
#if defined(USE_ACC)
		printf(" Accel  time: %f sec\n", taccel/(double)TICKS_ESEC);
		printf(" Scan   time: %f sec\n", tscan/(double)TICKS_ESEC);
		printf(" CacheA time: %f sec\n", tcacheA/(double)TICKS_ESEC);
#endif
		printf("Drain   time: %f sec\n", tdrain/(double)TICKS_ESEC);
		printf("Insert  time: %f sec\n", tinsert/(double)TICKS_ESEC);
		printf("Cache   time: %f sec\n", tcache/(double)TICKS_ESEC);
		printf("Oper.   time: %f sec\n", toper/(double)TICKS_ESEC);
		STATS_PRINT
		fclose(fin);
	}

	/* * * * * * * * * * Display Query Stats * * * * * * * * * */
	if (flags & BFLAG || qarg != NULL) {
		printf("lookups:%lu hits:%lu %.2f%%\n",
			(ulong_t)kdb.lookups, (ulong_t)kdb.hits,
			(double)kdb.hits/kdb.lookups*100.0);
	}

	/* * * * * * * * * * Wrap Up * * * * * * * * * */

	TRACE_CAP
	return EXIT_SUCCESS;
}

/* TODO:
*/

/* DONE:
*/
