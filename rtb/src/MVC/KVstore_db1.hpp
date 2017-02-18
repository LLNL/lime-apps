#ifndef _KV_STORE_HPP
#define _KV_STORE_HPP

#if defined(CLIENT) || defined(SERVER) || defined(DIRECT)
#define USE_LSU 1
#endif
#if defined(CLIENT) || defined(SERVER) || defined(USE_LSU)
#define USE_STREAM 1
#endif
#if defined(USE_LSU)
#define USE_HASH 1
// #define DEBUG1 1
// #define DEBUG2 1
// #define DEBUG3 1
// #define DEBUG4 1
#endif

#include <cstddef> // size_t
#include <algorithm> // min
#define SOFT 1
#include "short.h" // shortNN_hash
#include "alloc.h"


/* * * * * * * * * * Access, Cache, Memory, and Stats * * * * * * * * * */

#if defined(USE_DMAC)
#include <cstring> // memcpy, memset
#include "dmac_cmd.h"
#define memcpy dmac_memcpy
#define memset ::memset

#elif defined(USE_LSU) || defined(USE_HASH)
#include <cstring> // memcpy, memset
#include "lsu_cmd.h"
// #define memcpy lsu_memcpy
// #define memset ::memset

#else // use CPU
#include <cstring> // memcpy, memset
#define memcpy ::memcpy
#define memset ::memset
#endif

/* NOTE: if invalidate is used on non-cache aligned and sized allocations, */
/* it can corrupt the heap. */

// TODO: differentiate between management of scratchpad and DRAM space
// scratchpad only needs L1 (Xil_L1DCache*)
// DRAM space needs L1 and L2 (Xil_DCache*)
// MicroBlaze only has L1
// ARM has L1 and L2
#if defined(SERVER) && defined(__microblaze__)
#include "xil_cache.h"
class cache {
public:
inline void cache_flush(void) {Xil_L1DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_flush_invalidate(void) {Xil_L1DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_invalidate(void) {Xil_L1DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_L1DCacheInvalidateRange((unsigned int)ptr, (unsigned)size);}
};

#elif defined(DIRECT) && defined(__arm__)
#include "xil_cache.h"
class cache {
public:
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_L1DCacheInvalidateRange((unsigned int)ptr, (unsigned)size);}
};

#else
class cache {
public:
inline void cache_flush(void) {}
inline void cache_flush(const void *ptr, size_t size) {}
inline void cache_flush_invalidate(void) {}
inline void cache_flush_invalidate(const void *ptr, size_t size) {}
inline void cache_invalidate(void) {}
inline void cache_invalidate(const void *ptr, size_t size) {}
};
#endif

#if defined(__microblaze__)
// Invalidate single data cache line from addr
// Assumes C_DCACHE_USE_WRITEBACK is 0, therefore rB (i.e. r0) is not used
#define wdcc(addr) __asm__ __volatile__ ("wdc.clear\t%0,r0\n" :: "d" (addr))
#endif

#if defined(__microblaze__)
#define cprint(s)
#else
#define cprint(s) print(s)
#endif

/* * * * * * * * * * Stream Support * * * * * * * * * */

#if defined(USE_STREAM)
#include "stream.h"

// TODO: move to aport.h (or lsu_cmd.h)
#define ARM0_PN 0 /* ARM 0 port number */
#define MCU0_PN 1 /* MCU 0 port number */
#define LSU0_PN 2 /* LSU 0 port number */
#define LSU1_PN 3 /* LSU 1 port number */
#define HSU0_PN 4 /* HSU 0 port number */
#define getID(pn) ((pn)<<1)
#define HSU_CMD(seldi,seldo,seed,tdest,tid,hlen,klen) \
	((seldi) << 29 | (seldo) << 28 | (seed) << 24 | (tdest) << 20 | (tid) << 16 | (hlen) << 8 | (klen))

/* no resource management of units is performed */
/* units are preassigned */
/* TODO: decide where to allocate and introduce stream ids to base class? */
#if defined(__microblaze__)
#define THIS_PN MCU0_PN
#define STREAM_DEVICE_ID 0
#elif defined(__arm__)
#include "xparameters.h"
#define THIS_PN ARM0_PN
#define STREAM_DEVICE_ID XPAR_AXI_FIFO_0_DEVICE_ID
#endif

#define THIS_ID getID(THIS_PN)
#define MCU0_ID getID(MCU0_PN)
#define LSU0_ID getID(LSU0_PN)
#define LSU1_ID getID(LSU1_PN)
#define HSU0_ID getID(HSU0_PN)

class stream_port {
public:

	stream_t port;

	stream_port(void) {
		stream_init(&port, STREAM_DEVICE_ID);
	}
};
#endif // USE_STREAM

/* * * * * * * * * * Key-Value Engine * * * * * * * * * */

// basic configurations
// host  : base class + client methods : with or without load-store unit
// client: base class + client methods + protocol
// server: base class + protocol
// thread: client + server : emulate stream with pthreads.

// Define a base class with all the fields and methods.
// Then derive server and client classes from the base class.
// Override base functions and call explicitly when needed.

#define MAX_SEARCH (4*1024)
#define MAX_PSL 16
#define BLOCK_HASH 1

template<typename key_t, typename elem_t>
class _KVstore
: public cache
{
public:
	typedef unsigned int status_t;
	typedef unsigned int psl_t;
	typedef struct {
		key_t key;
		psl_t probes;
		elem_t value;
	} slot_s;

	slot_s *data_base; // data array base address
	size_t data_len; // data array len
	const key_t *key_base; // key array base address
	size_t key_len; // key array length
	size_t key_sz; // key size up to sizeof(key_t)
	status_t *status_base; // status array base address
	slot_s *scratch_base; // scratch array base address
	elem_t null_value; // null value;

	size_t elements;
	size_t elem_sz;
	size_t hash_mask;
	size_t topsearch;

	/* setup key/value store */
	/* This setup is called by the server (accelerator) & client (host) */
	/* The slot memory was previously allocated from the system heap */

	void _setup(
		void *data, size_t dlen,
		const void *key, size_t klen, size_t ksize,
		void *status, void *scratch
		)
	{
		data_base = (slot_s *)data;
		data_len = dlen;
		key_base = (const key_t *)key;
		key_len = klen;
		key_sz = ksize;
		status_base = (status_t *)status;
		scratch_base = (slot_s *)scratch;
		elements = 0;
		elem_sz = sizeof(elem_t);
#if defined(BLOCK_HASH)
		hash_mask = (dlen/MAX_PSL)-1;
#else
		hash_mask = dlen-1;
#endif
		topsearch = 0;
		null_value = NULL;
		// data_base and scratch are only used on the accelerator
		if (data_base != NULL) {
			cache_invalidate(data_base, sizeof(slot_s)*(dlen+MAX_SEARCH-1)); // null on arm
		}
		if (scratch != NULL) {
			cache_invalidate(scratch, sizeof(slot_s)*MAX_PSL*klen); // null on arm
		}
	}

	/* compute hash of key and form index */

	inline size_t hash_idx(key_t key)
	{
		uint32_t hash;

		short32_hash(&key, key_sz, 0xD, &hash);
#if defined(BLOCK_HASH)
		return (hash & hash_mask) * MAX_PSL;
#else
		return hash & hash_mask;
#endif
	}

#if defined(USE_HASH)

	/* configure hash unit and load-store unit */

	void hash_config(void)
	{
		unsigned int reg[5];

		/* hash unit setup */
		reg[1] = 0x00000000; /* clear status */
		reg[2] = hash_mask;  /* tlen_lo (mask_lo) */
		reg[3] = 0x00000000; /* tlen_hi (mask_hi) */
#if defined(DEBUG4)
		reg[4] = HSU_CMD(1,0,0xD,THIS_ID+READ_CH,HSU0_ID,sizeof(size_t),key_sz);
#else
		/* seldi=1 seldo=0 seed=0xD tdest=LSU0_ID+READ_CH tid=HSU0_ID hlen=sizeof(size_t) klen=key_sz */
		reg[4] = HSU_CMD(1,0,0xD,LSU0_ID+READ_CH,HSU0_ID,sizeof(size_t),key_sz);
#endif
		aport_nwrite(HSU0_ID, THIS_ID, 0, 0, reg, 4);

		/* load-store unit setup */
		/* currently the data path from LSU1 to HSU0 is hardwired */
		/* so no action is needed to setup the data path */
	}

#else // USE_HASH

	/* get: return pointer to element if found */

	bool _get(key_t key, char *&value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		/* search for key */
		while (data_base[idx].probes && idx < itop) {
			// if (memcmp(&data_base[idx].key, &key, key_sz) == 0) {
			if (data_base[idx].key == key) {
				value = (char *)&data_base[idx].value;
				return true; /* found */
			}
			idx++;
		}
		value = (char *)&null_value;
		return false;
	}

#endif // USE_HASH

	/* put: return pointer to element, allocate new slot if needed */

	bool _put(key_t key, char *&value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		size_t imax = idx + MAX_SEARCH;
		/* search for key */
		while (data_base[idx].probes && idx < imax) {
			// if (memcmp(&data_base[idx].key, &key, key_sz) == 0) {
			if (data_base[idx].key == key) {
				value = (char *)&data_base[idx].value;
				return true; /* found */
			}
			idx++;
		}
		/* add key */
		if (idx < imax) {
			// memcpy(&data_base[idx].key, &key, key_sz);
			data_base[idx].key = key;
			data_base[idx].probes = 1;
			value = (char *)&data_base[idx].value;
			elements++;
			if (++idx > itop) topsearch += idx-itop;
			return false; /* not found, but added */
		}
#if 0
		{ /* print out hash table free and used segments */
			int free = 0;
			int used = 0;
			int max_contig_free = 0;
			int max_contig_used = 0;
			int ibeg = 0;
			int contig = 0;
			int umode = 0;
			for (size_t i = 0; i < slots; i++) {
				if (umode) {
					if (data_base[i].probes != 0) {
						contig++; used++;
					} else {
						free++;
						if (contig > max_contig_used)
							max_contig_used = contig;
						printf("u ibeg:%d contig:%d\n", ibeg, contig);
						ibeg = i;
						contig = 1;
						umode = 0;
					}
				} else {
					if (data_base[i].probes == 0) {
						contig++; free++;
					} else {
						used++;
						if (contig > max_contig_free)
							max_contig_free = contig;
						printf("f ibeg:%d contig:%d\n", ibeg, contig);
						ibeg = i;
						contig = 1;
						umode = 1;
					}
				}
			}
			printf("%c ibeg:%d contig:%d\n", (umode)?'u':'f', ibeg, contig);
			printf("bidx:%zu eidx:%zu imax:%zu\n", imax - MAX_SEARCH, idx, imax);
			printf("free:%d used:%d mcfree:%d mcused:%d\n", free, used, max_contig_free, max_contig_used);
			printf("slots:%zu elements:%zu topsearch:%zu\n", slots, elements, topsearch);
		}
#endif
#if !defined(SERVER) || !defined(__microblaze__)
		fprintf(stderr, " -- hash table full, load factor:%f\n", (double)elements/data_len);
		fprintf(stderr, " -- key:%010llX ibeg:%u itop:%u imax:%u idx:%u\n", key, (unsigned)hash_idx(key), (unsigned)itop, (unsigned)imax, (unsigned)idx);
		fprintf(stderr, " -- elements:%u elem_sz:%u hash_mask:%X topsearch:%u\n", (unsigned)elements, (unsigned)elem_sz, (unsigned)hash_mask, (unsigned)topsearch);
		fprintf(stderr, " -- data_len:%u key_len:%u key_sz:%u\n", (unsigned)data_len, (unsigned)key_len, (unsigned)key_sz);
		fprintf(stderr, " --    data:%10p-%10p\n", data_base, data_base+data_len);
		fprintf(stderr, " --     key:%10p-%10p\n", key_base, key_base+key_len);
		fprintf(stderr, " --  status:%10p-%10p\n", status_base, status_base+key_len+1);
		fprintf(stderr, " -- scratch:%10p-%10p\n", scratch_base, scratch_base+key_len*MAX_PSL);
		exit(1);
#endif
		value = NULL; /* spill, will seg fault in memcpy */
		return false;
	}

public:

	/* setup key/value store */
	/* This setup is called by the client (host) */
	/* The slot memory is allocated from the system heap */

	void setup(
		size_t elements,
		const void *key, size_t klen, size_t ksize,
		void *status
		)
	{
		slot_s *data, *scratch;
		size_t dlen;
		size_t slots;

		// constraint: table length must be power of 2
		for (dlen = 1; dlen < elements; dlen <<= 1);
		slots = dlen+MAX_SEARCH-1;
		data = NALLOC(slot_s, slots);
		chk_alloc(data, sizeof(slot_s)*slots, "NALLOC data in setup()");
		cache_invalidate(data, sizeof(slot_s)*slots);
		memset(data, 0, sizeof(slot_s)*slots);
		// flush all caches on host for data range
		Xil_DCacheFlushRange((unsigned int)data, (unsigned)sizeof(slot_s)*slots);

#if defined(USE_HASH)
		scratch = SP_NALLOC(slot_s, MAX_PSL*klen); // TODO: use MAX_SEARCH?
		chk_alloc(scratch, sizeof(slot_s)*MAX_PSL*klen, "SP_NALLOC scratch in setup()");
		Xil_DCacheInvalidateRange((unsigned int)scratch, (unsigned)sizeof(slot_s)*MAX_PSL*klen);
#else // USE_HASH
		scratch = NULL;
#endif // USE_HASH

		_setup(data, dlen, key, klen, ksize, status, scratch);
		// Hash unit configuration is done here (by the client/host) to avoid
		// duplication, which would occur if hash_config() was inside
		// _KVstore<K,V>::_setup(). In that case, it would be called from
		// both client and server code.
#if defined(USE_HASH)
		hash_config();
#endif // USE_HASH
#if defined(DEBUG1) && defined(CLIENT)
xil_printf("    data:%x-%x\r\n", data_base, data_base+data_len);
xil_printf("     key:%x-%x\r\n", key_base, key_base+key_len);
xil_printf("  status:%x-%x\r\n", status_base, status_base+key_len+1);
xil_printf(" scratch:%x-%x\r\n", scratch_base, scratch_base+key_len*MAX_PSL);
#endif
	}

	/* fill and drain assumptions */
	/* buf, buf_sz & offset are a multiple of elem_sz (view buffer is aligned) */
	/* buf_sz does not overreach end of view */
	/* offset is zero */

#if defined(USE_HASH)

	/* scan: return pointer to element if found */

	bool _scan(slot_s *slot, size_t imax, key_t key, char *&value)
	{
		size_t idx = 0;
		/* search for key */
		while (slot[idx].probes && idx < imax) {
			// if (memcmp(&slot[idx].key, &key, key_sz) == 0) {
			if (slot[idx].key == key) {
				value = (char *)&slot[idx].value;
				return true; /* found */
			}
			idx++;
		}
		value = (char *)&null_value;
		return false;
	}

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / elem_sz;
		char *bptr = (char*)buf; /* pointer to current view element */
		char *dptr; /* pointer to current data element */
		unsigned n = buf_sz/elem_sz;
		unsigned reg[7];

		// if (n > key_len) n = key_len;
		cache_flush(); /* flush data_base before LSU access */

		/* LSU0 contiguous store (with stride command) */
		reg[1] = 0x00000000;             /* clear status */
		reg[2] = 0x80 | CMD_smove;       /* request status, smove command */
		reg[3] = (unsigned)scratch_base; /* address */
		reg[4] = sizeof(slot_s)*MAX_PSL; /* size */
		reg[5] = sizeof(slot_s)*MAX_PSL; /* increment */
		reg[6] = n;                      /* repetitions */
		aport_nwrite(LSU0_ID+WRITE_CH, THIS_ID, 1, 0, reg, 6);

		/* LSU0 indexed load */
		reg[1] = 0x00000000;             /* clear status */
		reg[2] = CMD_index;              /* index command */
		reg[3] = (unsigned)data_base;    /* base address */
		reg[4] = sizeof(slot_s)*MAX_PSL; /* size */ /* TODO: */
		/*reg[?] = sizeof(slot_s)*MAX_PSL;*/ /* trans size, repetitions */ /* TODO: reg 4 is index */
		aport_nwrite(LSU0_ID+READ_CH, THIS_ID, 0, 0, reg, 4);

		/* start streaming keys */
		/* LSU1 contiguous load (with strided move) */
		reg[1] = 0x00000000;         /* clear status */
		reg[2] = CMD_smove;          /* reqstat=0, command=smove */
		reg[3] = (unsigned)key_base; /* address */
		reg[4] = sizeof(key_t);      /* size */
		reg[5] = sizeof(key_t);      /* increment */
		reg[6] = n;                  /* repetitions */
		aport_nwrite(LSU1_ID+READ_CH, THIS_ID, 1, 0, reg, 6);

#if defined(DEBUG4)
		xil_printf(" idx:");
		const unsigned int rhead = AP_HEAD(1,1,4,1,HSU0_ID,THIS_ID+READ_CH); /* 0xE10X0Y */
		for (unsigned i = 0; i < n; i++) {
			unsigned rhash = hash_idx(key_base[i])/MAX_PSL;
			print(" r");
			stream_recv(gport, reg, sizeof(unsigned)*2, F_BEGP|F_ENDP);
			// if (reg[0] != rhead || reg[1] != rhash)
				// xil_printf("%u: %x,%x ~ %x\r\n", i, reg[0], reg[1], rhash);
			if (reg[1] != rhash)
				xil_printf("%u: %x ~ %x\r\n", i, reg[1], rhash);
			reg[0] = AP_HEAD(1,1,4,1,HSU0_ID,LSU0_ID+READ_CH);
			print(" w");
			stream_send(gport, reg, sizeof(unsigned)*2, F_BEGP|F_ENDP);
		}
		xil_printf(" idx done:");
#endif
		/* receive store status */
		stream_recv(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);

		/* scan scratch area for matches */
		// TODO: compare performance with entire data cache invalidate
		cache_invalidate(scratch_base, sizeof(slot_s)*MAX_PSL*n);
		status_base[0] = 0;
#if 1
		while (idx < n) {
			if (!_scan(&scratch_base[idx*MAX_PSL], topsearch, key_base[idx], dptr)) {
				status_base[++status_base[0]] = idx;
			}
			memcpy(bptr, dptr, elem_sz);
			idx++;
			bptr += elem_sz;
		}
#else
//while (idx < n)
		{
			if (!_scan(&data_base[hash_idx(key_base[idx])], topsearch, key_base[idx], dptr)) {
				status_base[++status_base[0]] = idx;
			}
			memcpy(bptr, dptr, elem_sz);
			idx++;
			bptr += elem_sz;
		}
//return;
		// scratch_base is missing first block of slots
		while (idx < n) {
			if (!_scan(&scratch_base[(idx-1)*MAX_PSL], topsearch, key_base[idx], dptr)) {
				status_base[++status_base[0]] = idx;
			}
			memcpy(bptr, dptr, elem_sz);
			idx++;
			bptr += elem_sz;
		}
#endif
	}

#else // USE_HASH

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / elem_sz;
		char *bptr = (char*)buf; /* pointer to current view element */
		char *dptr; /* pointer to current data element */
		unsigned n = buf_sz/elem_sz;

		// if (n > key_len) n = key_len;
		status_base[0] = 0;
		while (idx < n) {
			if (!_get(key_base[idx], dptr)) {
				status_base[++status_base[0]] = idx;
			}
			memcpy(bptr, dptr, elem_sz);
			idx++;
			bptr += elem_sz;
		}
	}

#endif // USE_HASH

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / elem_sz;
		char *bptr = (char*)buf; /* pointer to current view element */
		char *dptr; /* pointer to current data element */
		unsigned n = buf_sz/elem_sz;

		// if (n > key_len) n = key_len;
		status_base[0] = 0;
		while (idx < n) {
			if (!_put(key_base[idx], dptr)) {
				status_base[++status_base[0]] = idx;
			}
			memcpy(dptr, bptr, elem_sz);
			idx++;
			bptr += elem_sz;
		}
	}

	/* wait for operation to finish */

	void wait(void)
	{
		// NOTE: what this wait really means is a write barrier after a fill
		// TODO: The load-store unit needs a read and write barrier command
	}

}; // class _KVstore


template<typename K, typename V>
class KVstore
: public _KVstore<K,V>
#if defined(USE_STREAM)
, stream_port
#endif
{

#if defined(CLIENT) || defined (SERVER)

	typedef struct {
		unsigned int tdest :  8; /* forward route id */
		unsigned int tid   :  8; /* return route id */
		unsigned int tuser : 16; /* zero */

		unsigned int cmd  : 32; /* command */
	} header;

	typedef struct {
		void *data; size_t dlen;
		const void *key; size_t klen; size_t ksize;
		void *status; void *scratch;
	} setup_args;

	typedef struct {
		void *buf;
		size_t buf_sz;
		size_t offset;
	} buf_args;

	typedef struct {
		const void *buf;
		size_t buf_sz;
	} cache_args;

	enum {C_NOP, C_SETUP, C_FILL, C_DRAIN, C_WAIT, C_CFLUSH, C_CINVAL};

#endif // defined(CLIENT) || defined (SERVER)

public:

	KVstore(void)
	{
		// TODO: assign a load-store unit and control unit (port IDs)

		// options:
		// direct
		// direct with LSU support
		// client, start remote server - separate process, doesn't share memory
		// client, start thread server
#if defined(USE_LSU) || defined(USE_STREAM)
		lsu_setport(&port, LSU0_PN, THIS_PN);
#endif
#if defined(USE_DMAC)
		dmac_init(XPAR_XDMAPS_1_DEVICE_ID);
		dmac_setunit(0);
#endif
	}

#if defined(SERVER)

#if defined(__microblaze__)

	void command_server(void)
	{
		header hdr;
		size_t reg[7];

		for (;;) {
			getfsl_interruptible(*(unsigned *)&hdr, 0); getfsl(hdr.cmd, 0);
			switch (hdr.cmd) {
			case C_SETUP:
				getfsl(reg[0], 0); getfsl(reg[1], 0); getfsl(reg[2], 0); getfsl(reg[3], 0); getfsl(reg[4], 0); getfsl(reg[5], 0); cgetfsl(reg[6], 0);
				_KVstore<K,V>::_setup((void *)reg[0], reg[1], (const void *)reg[2], reg[3], reg[4], (void *)reg[5], (void *)reg[6]);
				break;
			case C_FILL:
				getfsl(reg[0], 0); getfsl(reg[1], 0); cgetfsl(reg[2], 0);
				_KVstore<K,V>::fill((void *)reg[0], reg[1], reg[2]);
				break;
			case C_DRAIN:
				getfsl(reg[0], 0); getfsl(reg[1], 0); cgetfsl(reg[2], 0);
				_KVstore<K,V>::drain((void *)reg[0], reg[1], reg[2]);
				break;
			case C_WAIT:
				cgetfsl(reg[0], 0);
				break;
			case C_CFLUSH:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) _KVstore<K,V>::cache_flush();
				else _KVstore<K,V>::cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) _KVstore<K,V>::cache_flush_invalidate(); // include flush
				else _KVstore<K,V>::cache_invalidate((void *)reg[0], reg[1]);
				break;
			default:
				break;
			}
			register unsigned int tmp = hdr.tid;
			hdr.tid = hdr.tdest;
			hdr.tdest = tmp;
			if (hdr.cmd == C_DRAIN) {
				typedef _KVstore<K,V> kvstore_c;
				putfsl(*(unsigned *)&hdr, 0); putfsl(hdr.cmd, 0);
				putfsl(kvstore_c::elements, 0); cputfsl(kvstore_c::topsearch, 0);
			} else {
				putfsl(*(unsigned *)&hdr, 0); cputfsl(hdr.cmd, 0);
			}
		}
	}

#else // __microblaze__

	void command_server(void)
	{
		header hdr;
		size_t reg[7];

		for (;;) {
			stream_recv(&port, &hdr, sizeof(hdr), F_BEGP);
			switch (hdr.cmd) {
			case C_SETUP:
				stream_recv(&port, reg, sizeof(size_t)*7, F_ENDP);
				_KVstore<K,V>::_setup((void *)reg[0], reg[1], (const void *)reg[2], reg[3], reg[4], (void *)reg[5], (void *)reg[6]);
				break;
			case C_FILL:
				stream_recv(&port, reg, sizeof(size_t)*3, F_ENDP);
				_KVstore<K,V>::fill((void *)reg[0], reg[1], reg[2]);
				break;
			case C_DRAIN:
				stream_recv(&port, reg, sizeof(size_t)*3, F_ENDP);
				_KVstore<K,V>::drain((void *)reg[0], reg[1], reg[2]);
				break;
			case C_WAIT:
				stream_recv(&port, reg, sizeof(size_t)*1, F_ENDP);
				break;
			case C_CFLUSH:
				stream_recv(&port, reg, sizeof(size_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) _KVstore<K,V>::cache_flush();
				else _KVstore<K,V>::cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				stream_recv(&port, reg, sizeof(size_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) _KVstore<K,V>::cache_flush_invalidate(); // include flush
				else _KVstore<K,V>::cache_invalidate((void *)reg[0], reg[1]);
				break;
			default:
				break;
			}
			register unsigned int tmp = hdr.tid;
			hdr.tid = hdr.tdest;
			hdr.tdest = tmp;
			if (hdr.cmd == C_DRAIN) {
				reg[0] = _KVstore<K,V>::elements;
				reg[1] = _KVstore<K,V>::topsearch;
				stream_send(&port, &hdr, sizeof(hdr), F_BEGP);
				stream_send(&port, reg, sizeof(size_t)*2, F_ENDP);
			} else {
				stream_send(&port, &hdr, sizeof(hdr), F_BEGP|F_ENDP);
			}
		}
	}

#endif // __microblaze__

#endif // SERVER

#if defined(CLIENT)

	/* setup key/value store */

	void setup(
		size_t elements,
		const void *key, size_t klen, size_t ksize,
		void *status
		)
	{
		header hdr;
		header res;
		setup_args args;

#if defined(DEBUG1)
xil_printf("port.inst.addr:%x ready:%x gport:%x\r\n", port.Instance.BaseAddress, port.Instance.IsReady, gport);
#endif
		_KVstore<K,V>::setup(elements, key, klen, ksize, status);
		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_SETUP;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.data = _KVstore<K,V>::data_base;
		args.dlen = _KVstore<K,V>::data_len;
		args.key = key;
		args.klen = klen;
		args.ksize = ksize;
		args.status = status;
		args.scratch = _KVstore<K,V>::scratch_base;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		header hdr;
		header res;
		buf_args args;

// xil_printf(" fill: buf:%x size:%x\r\n", buf, buf_sz);
		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_FILL;
// print(" hdr:");
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = buf;
		args.buf_sz = buf_sz;
		args.offset = offset;
// print(" arg:");
		stream_send(&port, &args, sizeof(args), F_ENDP);

// print(" res:");
		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
// print(" done:\r\n");
#if defined(DEBUG1)
for (int i = 0; i < 8; i++) xil_printf(" HSU0   [%d]:%x\r\n", i, aport_read(HSU0_ID, THIS_ID, i));
for (int i = 0; i < 6; i++) xil_printf(" LSU0_RD[%d]:%x\r\n", i, aport_read(LSU0_ID+READ_CH, THIS_ID, i));
for (int i = 0; i < 6; i++) xil_printf(" LSU0_WR[%d]:%x\r\n", i, aport_read(LSU0_ID+WRITE_CH, THIS_ID, i));
for (int i = 0; i < 6; i++) xil_printf(" LSU1_RD[%d]:%x\r\n", i, aport_read(LSU1_ID+READ_CH, THIS_ID, i));
#endif
#if defined(DEBUG2)
// xil_printf(" sb:%x sz:%u\r\n", _KVstore<K,V>::scratch_base, sizeof(typename _KVstore<K,V>::slot_s)*MAX_PSL*_KVstore<K,V>::key_len);
host::cache_invalidate(_KVstore<K,V>::scratch_base, sizeof(typename _KVstore<K,V>::slot_s)*MAX_PSL*_KVstore<K,V>::key_len);
print("[\r\n");
for (unsigned i = 0; i < MAX_PSL*_KVstore<K,V>::key_len; i++) {
	if (_KVstore<K,V>::scratch_base[i].probes == 0) continue;
	xil_printf(" [%d]: key:%x probes:%u value:%x\r\n", i,
	(size_t)_KVstore<K,V>::scratch_base[i].key,
	_KVstore<K,V>::scratch_base[i].probes,
	_KVstore<K,V>::scratch_base[i].value);
}
print("]\r\n");
#endif
#if defined(DEBUG3)
cache_flush(buf, buf_sz);
host::cache_invalidate(buf, buf_sz);
print("[\r\n");
{unsigned *vptr = (unsigned*)buf;
for (unsigned i = 0; i < _KVstore<K,V>::key_len; i++) {
	if (vptr[i] == 0) continue;
	xil_printf(" [%d]: value:%x\r\n", i, vptr[i]);
}
}
print("]\r\n");
#endif
#if defined(DEBUG1) || defined(DEBUG2)
// exit(0);
#endif
	}

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		header hdr;
		header res;
		buf_args args;
		size_t reg[2];

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_DRAIN;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = buf;
		args.buf_sz = buf_sz;
		args.offset = offset;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP);
		stream_recv(&port, reg, sizeof(size_t)*2, F_ENDP);
		_KVstore<K,V>::elements  = reg[0];
		_KVstore<K,V>::topsearch = reg[1];
	}

	/* wait for operation to finish */

#if 0
	void wait(void)
	{
		header hdr;
		header res;
		size_t args;

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_WAIT;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);
		stream_send(&port, &args, sizeof(args), F_ENDP);
		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}
#endif

	void cache_flush(const void *ptr = NULL, size_t size = 0)
	{
		header hdr;
		header res;
		cache_args args;

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_CFLUSH;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ptr;
		args.buf_sz = size;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	void cache_invalidate(const void *ptr = NULL, size_t size = 0)
	{
		header hdr;
		header res;
		cache_args args;

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_CINVAL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ptr;
		args.buf_sz = size;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

#endif // CLIENT

}; // class KVstore

// let them be used by application
//#undef memcpy
//#undef memset

#endif // _KV_STORE_HPP

/*
 * Arguments to setup?
   - elements, elem_sz
   - key_base, key_len, key_sz
   - status_base, up to key_len+1 (optional)
 * How to return status of batch fill (get) and drain (put)?
   - separate implementation issues from interface, e.g. fields in bucket
     status: count of not found (fail?) and list of indexes.
     Otherwise encode not found as null pointer/data on get (fill).
     Must use status if it is wanted for put (drain).
 * Manage allocations from host or MCU?
 * Single get and put?
 * How to handle spill?
 * Compatibility with STL?
   - use wrapper around fill/drain API?
 * What does fill offset mean?
   - start with a key other than index zero
**/
