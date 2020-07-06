#ifndef _KV_STORE_HPP
#define _KV_STORE_HPP

#include <cstddef> // size_t
#include <cstdlib> // exit
#include <cstdint> // uintXX_t
#include <cstdio> // printf
#include "short.h" // shortNN_hash
#include "config.h"
#include "alloc.h"

/* * * * * * * * * * Access, Cache, Memory, and Stats * * * * * * * * * */

#if defined(USE_DMAC)
#include <cstring> // memcpy, memset
#include "dmac_cmd.h"
#define memcpy dmac_memcpy
#define memset ::memset

#elif defined(USE_LSU)
#include <cstring> // memcpy, memset
#include "lsu_cmd.h"
#define memcpy lsu_memcpy
#define memset ::memset

#else // use CPU
#include <cstring> // memcpy, memset
// #define memcpy ::memcpy
// #define memset ::memset
#endif

/* NOTE: if invalidate is used on non-cache aligned and sized allocations, */
/* it can corrupt the heap. */

/* NOTE: example ARM assembly for L1 cache management */
// mtcp(XREG_CP15_CACHE_SIZE_SEL, 0); /* Select L1 Data cache in CSSR */
// mtcp(XREG_CP15_CLEAN_DC_LINE_MVA_POC, ((unsigned int)addr)); // L1 flush line
// mtcp(XREG_CP15_INVAL_DC_LINE_MVA_POC, ((unsigned int)addr)); // L1 invalidate line
// mtcp(XREG_CP15_CLEAN_INVAL_DC_LINE_MVA_POC, ((unsigned int)addr)); L1 flush_invalidate line

// TODO: differentiate between management of scratchpad and DRAM space
// scratchpad only needs L1 (Xil_L1DCache*)
// DRAM space needs L1 and L2 (Xil_DCache*)
// MicroBlaze only has L1
// ARM has L1 and L2
#if defined(USE_STREAM) && defined(__microblaze__)
#include "xil_cache.h"
class cache {
public:
inline void cache_flush(void) {Xil_L1DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_flush_invalidate(void) {Xil_L1DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_invalidate(void) {Xil_L1DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_L1DCacheInvalidateRange((INTPTR)ptr, size);}
};

#elif defined(USE_STREAM) && defined(__ARM_ARCH)
//#include "xl2cc.h" // XPS_L2CC_*
#include "xil_cache.h"
class cache {
public:
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_L1DCacheInvalidateRange((INTPTR)ptr, size);}
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
#define cprint(str)
#define cprintf(format, ...)
#elif defined(ZYNQ)
#include "xil_printf.h"
#define cprint(str) print(str)
#define cprintf(format, ...) xil_printf(format, ## __VA_ARGS__)
#else // not __microblaze__, ZYNQ
#include <stdio.h>
#define cprint(str) fprintf(stderr, str)
#define cprintf(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#endif

// NOTE: no cycle counters are available on the MicroBlaze
#if defined(__microblaze__)
#define tget(t)
#else
#include "ticks.h"
#endif

/* * * * * * * * * * Stream Support * * * * * * * * * */

#if defined(USE_STREAM)
#include "aport.h"

/* no resource management of units is performed */
/* units are preassigned */
/* TODO: decide where to allocate and introduce stream ids to base class? */
#if defined(__microblaze__)
#define THIS_PN MCU0_PN
#else
#define THIS_PN ARM0_PN
#endif

#define THIS_ID getID(THIS_PN)
#define MCU0_ID getID(MCU0_PN)
#define LSU0_ID getID(LSU0_PN)
#define LSU1_ID getID(LSU1_PN)
#define HSU0_ID getID(HSU0_PN)
#define LSU2_ID getID(LSU2_PN)
#define PRU0_ID getID(PRU0_PN)

#if defined(ZYNQ)
#include "xparameters.h"
#define STREAM_DEVICE_ID XPAR_AXI_FIFO_0_DEVICE_ID
#else
#define STREAM_DEVICE_ID 0
#endif

inline void dump_reg(void)
{
	for (int i = 0; i < 8; i++) cprintf(" HSU0   [%d]:%x\r\n", i, aport_read(HSU0_ID, THIS_ID, i));
	for (int i = 0; i < 2; i++) cprintf(" PRU0   [%d]:%x\r\n", i, aport_read(PRU0_ID, THIS_ID, i));
	for (int i = 0; i < 6; i++) cprintf(" LSU1_RD[%d]:%x\r\n", i, aport_read(LSU1_ID+READ_CH, THIS_ID, i));
	for (int i = 0; i < 6; i++) cprintf(" LSU2_RD[%d]:%x\r\n", i, aport_read(LSU2_ID+READ_CH, THIS_ID, i));
	for (int i = 0; i < 6; i++) cprintf(" LSU2_WR[%d]:%x\r\n", i, aport_read(LSU2_ID+WRITE_CH, THIS_ID, i));
}

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

// Load vs. maximum probe sequence length (Max_PSL or MAX_SEARCH)
// with short32_hash():
// LINEAR (load,Max_PSL): (20%,13) (60%,95) (80%,529) (85%,600) (90%,1266) (95%,5141)
// RHOOD  (load,Max_PSL): (20%,7) (60%,19) (80%,36) (85%,49) (90%,76) (95%,133) (98%,268) (99%,425)
#if !defined(MAX_SEARCH)
#define MAX_SEARCH (512)
#endif
//#define BLOCK_HASH 1
#define BLOCK_HASH_LEN 16

#if defined(SPILL)
#include <unordered_map>
#endif

template<typename _Key, typename _Tp>
class _KVstore
: public cache
{
public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	// typedef std::pair<const key_type, mapped_type> value_type;
	typedef unsigned int psl_t;
	// typedef struct { // TODO: check that size and alignment matches hardware
		// value_type value;
		// psl_t probes;
	// } slot_s;
	typedef struct {
		key_type key;
		mapped_type value;
		psl_t probes;
	} slot_s;

	slot_s *data_base; // data array base address
	size_t data_len; // data array len
	const size_t key_sz; // key size up to sizeof(key_type)
	const size_t elem_sz; // element size sizeof(mapped_type)
	const mapped_type null_value; // null value;

	size_t elements;
	size_t hash_mask;
	size_t topsearch;

#if defined(SPILL)
	std::unordered_map<_Key, _Tp> spill;
#endif

	_KVstore()
	: key_sz(sizeof(key_type)),
	  elem_sz(sizeof(mapped_type)),
	  null_value(0)
	{ }

	/* setup key/value store */
	/* This setup is called by the server (accelerator) & client (host) */
	/* The slot memory was previously allocated from the system heap */

	void _setup(void *data, size_t dlen)
	{
		data_base = (slot_s *)data;
		data_len = dlen;
		elements = 0;
#if defined(BLOCK_HASH)
		hash_mask = (dlen/BLOCK_HASH_LEN)-1;
#else
		hash_mask = dlen-1;
#endif
		topsearch = 0;
#if defined(SERVER)
		if (data_base != NULL) {
			cache_invalidate(data_base, sizeof(slot_s)*(dlen+MAX_SEARCH-1));
		}
#endif
	}

	/* compute hash of key and form index */

	inline size_t hash_idx(key_type key)
	{
		uint32_t hash;

		short32_hash(&key, key_sz, 0xD, &hash);
		/* these overflow MAX_SEARCH when hashing real k-mers (e.g. SRR033549.fa) */
		// hash = key;
		// hash = std::hash<key_type>{}(key); // #include <functional> 
#if defined(BLOCK_HASH)
		return (hash & hash_mask) * BLOCK_HASH_LEN;
#else
		return hash & hash_mask;
#endif
	}

	/* search: search for key, return pointer to slot if found */

	slot_s* _search(const key_type key)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		/* search for key */
		while (data_base[idx].probes && idx < itop) {
			if (data_base[idx].key == key) {
				return &data_base[idx]; /* found */
			}
			idx++;
		}
		return nullptr;
	}

	/* get: search for key, return mapped value if found */

	bool _get(const key_type key, mapped_type &value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		/* search for key */
		while (data_base[idx].probes && idx < itop) {
			if (data_base[idx].key == key) {
				value = data_base[idx].value;
				return true; /* found */
			}
			idx++;
		}
		value = null_value;
		return false;
	}

#if defined(LINEAR)

	/* put: add key and mapped value, find new slot if needed */

	bool _put(const key_type key, const mapped_type &value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		size_t imax = idx + MAX_SEARCH;
		/* search for key */
		while (data_base[idx].probes && idx < imax) {
			if (data_base[idx].key == key) {
				data_base[idx].value = value;
#if 0 && defined(USE_STREAM)
				// TODO: use Xil_DCacheFlushLine((INTPTR)&data_base[idx].value); straddle cache line? - no
				Xil_DCacheFlushRange((INTPTR)&data_base[idx].value, sizeof(mapped_type));
#endif // USE_STREAM
				return true; /* found */
			}
			idx++;
		}
		/* add key */
		if (idx < imax) {
			data_base[idx].key = key;
			data_base[idx].probes = 1;
			data_base[idx].value = value;
#if 0 && defined(USE_STREAM)
			Xil_DCacheFlushRange((INTPTR)&data_base[idx], sizeof(slot_s));
#endif // USE_STREAM
			elements++;
			if (++idx > itop) topsearch += idx-itop;
			return false; /* not found, but added */
		}
#if defined(SPILL)
		spill.insert({key, value});
#else
		cprint(" -- error: _put: spill");
		exit(EXIT_FAILURE);
#endif
		return false; // spill
	}

	/* update1: return mapped value and pointer to slot, find new slot if needed */

	slot_s* _update1(const key_type key, mapped_type &value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		size_t imax = idx + MAX_SEARCH;
		/* search for key */
		while (data_base[idx].probes && idx < imax) {
			if (data_base[idx].key == key) {
				value = data_base[idx].value;
				return &data_base[idx]; /* found */
			}
			idx++;
		}
		/* add key */
		if (idx < imax) {
			data_base[idx].key = key;
			data_base[idx].probes = 1;
			value = data_base[idx].value = null_value;
			elements++;
			if (idx >= itop) topsearch += idx-itop+1;
			return &data_base[idx]; /* not found, but added */
		}
#if defined(SPILL)
		spill.insert({key, value});
#else
		cprint(" -- error: _update1: spill");
		exit(EXIT_FAILURE);
#endif
		return nullptr; // spill
	}

#else // LINEAR - Robin Hood insertion

	/* put: add key and mapped value, find new slot if needed */

	bool _put(const key_type key, const mapped_type &value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		size_t irh = idx;
		slot_s current, tmp;

		/* search for key */
		while (data_base[idx].probes && idx < itop) {
			if (data_base[idx].key == key) {
				data_base[idx].value = value;
#if 0 && defined(USE_STREAM)
				// TODO: use Xil_DCacheFlushLine((INTPTR)&data_base[idx].value); straddle cache line? - no
				Xil_DCacheFlushRange((INTPTR)&data_base[idx].value, sizeof(mapped_type));
#endif // USE_STREAM
				return true; /* found */
			}
			idx++;
		}

		/* find a new slot */
		current.key = key;
		current.value = value;
		current.probes = 0;
		for (;;) {
			current.probes++;
			if (current.probes > MAX_SEARCH) {
#if defined(SPILL)
				spill.insert({current.key, current.value});
#else
				cprint(" -- error: _put: spill");
				exit(EXIT_FAILURE);
#endif
				return false; // spill
			}
			/* swap entries */
			if (current.probes > data_base[irh].probes) {
				if (current.probes > topsearch) topsearch = current.probes;
				tmp = data_base[irh];
				data_base[irh] = current;
#if 0 && defined(USE_STREAM)
			Xil_DCacheFlushRange((INTPTR)&data_base[irh], sizeof(slot_s));
#endif // USE_STREAM
				if (!tmp.probes) {elements++; return false;} // added
				current = tmp;
			}
			irh++;
		}
	}

	/* update1: return mapped value and pointer to slot, find new slot if needed */

	slot_s* _update1(const key_type key, mapped_type &value)
	{
		size_t idx = hash_idx(key);
		size_t itop = idx + topsearch;
		size_t irh = idx;
		slot_s current, tmp, *slot = nullptr;

		/* search for key */
		while (data_base[idx].probes && idx < itop) {
			if (data_base[idx].key == key) {
				value = data_base[idx].value;
				return &data_base[idx]; /* found */
			}
			idx++;
		}

		/* find a new slot */
		current.key = key;
		value = current.value = null_value;
		current.probes = 0;
		for (;;) {
			current.probes++;
			if (current.probes > MAX_SEARCH) {
#if defined(SPILL)
				spill.insert({current.key, current.value});
#else
				cprint(" -- error: _update1: spill");
				exit(EXIT_FAILURE);
#endif
				return nullptr; // spill
			}
			/* swap entries */
			if (current.probes > data_base[irh].probes) {
				if (current.probes > topsearch) topsearch = current.probes;
				tmp = data_base[irh];
				data_base[irh] = current;
				if (slot == nullptr) {slot = &data_base[irh];}
				else {
#if 0 && defined(USE_STREAM)
			Xil_DCacheFlushRange((INTPTR)&data_base[irh], sizeof(slot_s));
#endif // USE_STREAM
				}
				if (!tmp.probes) {elements++; return slot;} // added
				current = tmp;
			}
			irh++;
		}
	}

#endif // LINEAR

	/* update2: update mapped value in specified slot */
	// If flush done by caller, slot_s needs to be exposed.

	inline void _update2(slot_s *slot, const mapped_type &value)
	{
#if defined(SPILL)
		if (slot == nullptr) return;
#endif
		if (slot->value != value || slot->value == 0) {
			slot->value = value;
#if 0 && defined(USE_STREAM) && defined(__microblaze__)
			// TODO: 
#elif 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
			// Xil_DCacheFlushLine((INTPTR)slot); // doesn't straddle cache line
			// Xil_DCacheFlushRange((INTPTR)slot, sizeof(slot_s));
			mtcp(XREG_CP15_CLEAN_DC_LINE_MVA_POC, (INTPTR)slot);
			// dsb(); Xil_Out32(XPS_L2CC_BASEADDR + XPS_L2CC_CACHE_CLEAN_PA_OFFSET, (INTPTR)slot);
#endif // USE_STREAM
		}
	}

public:

	/* hbin assumed to be topsearch in length */
	void psl_histo(unsigned *hbin)
	{
		size_t idx;
		size_t tslot = data_len + topsearch - 1;

		memset(hbin, 0, sizeof(unsigned)*topsearch);
		for (idx = 0; idx < tslot; idx++) {
			unsigned psl; /* psl minus 1 */
			if (data_base[idx].probes == 0) continue;
			psl = idx - hash_idx(data_base[idx].key);
			if (psl < topsearch) hbin[psl]++;
		}
	}

#if defined(USE_HASH)

	/* configure hash unit and load-store unit */

	void hash_config(void)
	{
		flit_t reg[5];

		/* hash unit setup */
		reg[1] = 0x00000000; /* clear status */
		reg[2] = hash_mask;  /* tlen_lo (mask_lo) */
		reg[3] = 0x00000000; /* tlen_hi (mask_hi) */
		/* seldi=1 seldo=0 seed=0xD tdest=LSU2_ID+READ_CH tid=HSU0_ID hlen=sizeof(flit_t) klen=key_sz */
		reg[4] = HSU_CMD(1,0,0xD,LSU2_ID+READ_CH,HSU0_ID,sizeof(flit_t),key_sz);
		aport_nwrite(HSU0_ID, THIS_ID, 0, 0, reg, 4);

		/* load-store unit setup */
		/* currently the data path from LSU1 to HSU0 is hardwired */
		/* so no action is needed to setup the data path */
	}

#endif // USE_HASH

	/* setup key/value store */
	/* This setup is called by the client (host) */
	/* The slot memory is allocated from the system heap */

	void setup(size_t elements)
	{
		slot_s *data;
		size_t dlen;
		size_t slots;

		// constraint: table length must be power of 2
		for (dlen = 1; dlen < elements; dlen <<= 1);
		slots = dlen+MAX_SEARCH-1;
#if 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
		{
			char *ptr;
			size_t size = CEIL((slots)*sizeof(slot_s), (1U<<20));
			data = (slot_s*)memalign((1U<<20), size);
			if ((uintptr_t)data & ((1U<<20)-1)) fprintf(stderr, " -- error: table not aligned\n");
			ptr = (char*)data;
			while (size) {
				Xil_SetTlbAttributes((INTPTR)ptr, 0x04de6);
				ptr += (1U<<20);
				size -= (1U<<20);
			}
		}
#else
		data = NALLOC(slot_s, slots);
#endif
		chk_alloc(data, sizeof(slot_s)*slots, "NALLOC data in setup()");
		memset(data, 0, sizeof(slot_s)*slots);
#if defined(USE_STREAM) && defined(__ARM_ARCH)
		// flush all caches on host for data range
		// FIXME: can't use cache_flush(ptr, size) because it uses L1D variant
		Xil_DCacheFlushRange((INTPTR)data, sizeof(slot_s)*slots);
#endif // USE_STREAM

		_setup(data, dlen);
		// Hash unit configuration is done here (by the client/host) to avoid
		// duplication, which would occur if hash_config() was inside
		// _KVstore<K,V>::_setup(). In that case, it would be called from
		// both client and server code.
#if defined(USE_HASH)
		hash_config();
#endif // USE_HASH
	}

	/* fill and drain assumptions */
	/* buffers are aligned */

#if defined(USE_HASH)

	/* fill buffer from store based on keys */
	/* TODO: make getfsl(), putfsl() version of fill() for MCU */

	void fill(void *buf, size_t len, const void *key, size_t stride)
	{
		uintptr_t paddr;
		flit_t reg[7];

		paddr = ATRAN(buf);
#if defined(CONTIG)
		/* LSU2 contiguous store (with move command) */
		reg[1] = 0x00000000;              /* clear status */
		reg[2] = LSU_ACMD(paddr,1,LSU_move); /* reqstat=1, command=move */
		reg[3] = flit_t(paddr);           /* address */
		reg[4] = sizeof(mapped_type)*len; /* size */
		aport_nwrite(LSU2_ID+WRITE_CH, THIS_ID, 1, 0, reg, 4); // go
		// aport_nwrite(LSU2_ID+WRITE_CH, THIS_ID, 0, 0, reg, 4); // debug
#else
		/* LSU2 contiguous store (with strided move command) */
		reg[1] = 0x00000000;           /* clear status */
		reg[2] = LSU_ACMD(paddr,1,LSU_smove); /* reqstat=1, command=smove */
		reg[3] = flit_t(paddr);        /* address */
		reg[4] = sizeof(mapped_type);  /* size */
		reg[5] = sizeof(mapped_type);  /* increment */
		reg[6] = len;                  /* repetitions */
		aport_nwrite(LSU2_ID+WRITE_CH, THIS_ID, 1, 0, reg, 6); // go
		// aport_nwrite(LSU2_ID+WRITE_CH, THIS_ID, 0, 0, reg, 6); // debug
#endif

		paddr = ATRAN(data_base);
		/* LSU2 index2 load */
		reg[1] = 0x00000000;               /* clear status */
		reg[2] = LSU_ACMD(paddr,0,LSU_index2); /* reqstat=0, command=index2 */
		reg[3] = flit_t(paddr);            /* base address */
		reg[4] = sizeof(slot_s);           /* size */
		reg[5] = 0x00000000;               /* index (spacer) */
		reg[6] = sizeof(slot_s)*topsearch; /* transfer size */
		aport_nwrite(LSU2_ID+READ_CH, THIS_ID, 0, 0, reg, 6);

		/* PRU0 configuration */
		reg[1] = 0x00000000;     /* clear status */
		reg[2] = topsearch-1;    /* plen, minus 1 */
		aport_nwrite(PRU0_ID, THIS_ID, 0, 0, reg, 2);

		paddr = ATRAN(key);
#if defined(CONTIG)
		/* start streaming keys */
		/* LSU1 contiguous load (with move command) */
		reg[1] = 0x00000000;           /* clear status */
		reg[2] = LSU_ACMD(paddr,0,LSU_move); /* reqstat=0, command=move */
		reg[3] = flit_t(paddr);        /* address */
		reg[4] = sizeof(key_type)*len; /* size */
		aport_nwrite(LSU1_ID+READ_CH, THIS_ID, 1, 0, reg, 4); // go
		// aport_nwrite(LSU1_ID+READ_CH, THIS_ID, 0, 0, reg, 4); // debug
#else
		/* start streaming keys */
		/* LSU1 contiguous load (with strided move command) */
		reg[1] = 0x00000000;           /* clear status */
		reg[2] = LSU_ACMD(paddr,0,LSU_smove); /* reqstat=0, command=smove */
		reg[3] = flit_t(paddr);        /* address */
		reg[4] = sizeof(key_type);     /* size */
		reg[5] = stride;               /* increment */
		reg[6] = len;                  /* repetitions */
		aport_nwrite(LSU1_ID+READ_CH, THIS_ID, 1, 0, reg, 6); // go
		// aport_nwrite(LSU1_ID+READ_CH, THIS_ID, 0, 0, reg, 6); // debug
#endif

		/* receive store status */
		stream_recv(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);
	}

#else // USE_HASH

	/* fill buffer from store based on keys */

	void fill(void *buf, size_t len, const void *key, size_t stride)
	{
		mapped_type *bptr = (mapped_type*)buf; /* pointer to current buffer element */
		unsigned char *kptr = (unsigned char *)key; /* pointer to current key */

		while (len--) {
			_get(*(const key_type *)kptr, *bptr++);
			kptr += stride;
		}
	}

#endif // USE_HASH

	/* drain buffer to store based on keys */

	void drain(void *buf, size_t len, const void *key, size_t stride)
	{
		mapped_type *bptr = (mapped_type*)buf; /* pointer to current buffer element */
		unsigned char *kptr = (unsigned char *)key; /* pointer to current key */

		while (len--) {
			_put(*(const key_type *)kptr, *bptr++);
			kptr += stride;
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
, public stream_port
#endif
{

#if defined(CLIENT) || defined(SERVER)

	typedef struct {
		unsigned int tdest :  8; /* forward route id */
		unsigned int tid   :  8; /* return route id */
		unsigned int tuser : 16; /* zero */

		unsigned int cmd  : 32; /* command */
	} header;

	typedef struct {
		flit_t data;
		flit_t dlen;
	} setup_args;

	typedef struct {
		flit_t buf;
		flit_t len;
		flit_t key;
		flit_t stride;
	} buf_args;

	typedef struct {
		flit_t buf;
		flit_t buf_sz;
	} cache_args;

	enum {C_NOP, C_SETUP, C_FILL, C_DRAIN, C_WAIT, C_CFLUSH, C_CINVAL};

#endif // defined(CLIENT) || defined(SERVER)

public:

	KVstore(void)
	{
		// options:
		// direct
		// direct with LSU support
		// client, start remote server - separate process, doesn't share memory
		// client, start thread server
#if defined(USE_LSU)
		lsu_setport(&port, LSU0_PN, THIS_PN);
#elif defined(USE_STREAM)
		aport_set(&port);
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
		flit_t reg[4];

		for (;;) {
			getfsl_interruptible(*(flit_t *)&hdr, 0); getfsl(hdr.cmd, 0);
			switch (hdr.cmd) {
			case C_SETUP:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				_KVstore<K,V>::_setup((void *)reg[0], reg[1]);
				break;
			case C_FILL:
				getfsl(reg[0], 0); getfsl(reg[1], 0); getfsl(reg[2], 0); cgetfsl(reg[3], 0);
				_KVstore<K,V>::fill((void *)reg[0], reg[1], (void *)reg[2], reg[3]);
				break;
			case C_DRAIN:
				getfsl(reg[0], 0); getfsl(reg[1], 0); getfsl(reg[2], 0); cgetfsl(reg[3], 0);
				_KVstore<K,V>::drain((void *)reg[0], reg[1], (void *)reg[2], reg[3]);
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
				putfsl(*(flit_t *)&hdr, 0); putfsl(hdr.cmd, 0);
				putfsl(kvstore_c::elements, 0); cputfsl(kvstore_c::topsearch, 0);
			} else {
				putfsl(*(flit_t *)&hdr, 0); cputfsl(hdr.cmd, 0);
			}
		}
	}

#else // __microblaze__

	void command_server(void)
	{
		header hdr;
		flit_t reg[4];

		for (;;) {
			stream_recv(&port, &hdr, sizeof(hdr), F_BEGP);
			switch (hdr.cmd) {
			case C_SETUP:
				stream_recv(&port, reg, sizeof(flit_t)*2, F_ENDP);
				_KVstore<K,V>::_setup((void *)reg[0], reg[1]);
				break;
			case C_FILL:
				stream_recv(&port, reg, sizeof(flit_t)*4, F_ENDP);
				_KVstore<K,V>::fill((void *)reg[0], reg[1], (void *)reg[2], reg[3]);
				break;
			case C_DRAIN:
				stream_recv(&port, reg, sizeof(flit_t)*4, F_ENDP);
				_KVstore<K,V>::drain((void *)reg[0], reg[1], (void *)reg[2], reg[3]);
				break;
			case C_WAIT:
				stream_recv(&port, reg, sizeof(flit_t)*1, F_ENDP);
				break;
			case C_CFLUSH:
				stream_recv(&port, reg, sizeof(flit_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) _KVstore<K,V>::cache_flush();
				else _KVstore<K,V>::cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				stream_recv(&port, reg, sizeof(flit_t)*2, F_ENDP);
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
				stream_send(&port, reg, sizeof(flit_t)*2, F_ENDP);
			} else {
				stream_send(&port, &hdr, sizeof(hdr), F_BEGP|F_ENDP);
			}
		}
	}

#endif // __microblaze__

#endif // SERVER

#if defined(CLIENT)

	/* setup key/value store */

	void setup(size_t elements)
	{
		header hdr;
		header res;
		setup_args args;

		_KVstore<K,V>::setup(elements);
		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_SETUP;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.data = ATRAN((_KVstore<K,V>::data_base));
		args.dlen = _KVstore<K,V>::data_len;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	/* fill buffer from store based on keys */

	void fill(void *buf, size_t len, const void *key, size_t stride)
	{
		header hdr;
		header res;
		buf_args args;

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_FILL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ATRAN(buf);
		args.len = len;
		args.key = ATRAN(key);
		args.stride = stride;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	/* drain buffer to store based on keys */

	void drain(void *buf, size_t len, const void *key, size_t stride)
	{
		header hdr;
		header res;
		buf_args args;
		flit_t reg[2];

		hdr.tdest = MCU0_ID;
		hdr.tid = THIS_ID;
		hdr.tuser = 0;
		hdr.cmd = C_DRAIN;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ATRAN(buf);
		args.len = len;
		args.key = ATRAN(key);
		args.stride = stride;
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
		flit_t args;

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

		args.buf = ATRAN(ptr);
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

		args.buf = ATRAN(ptr);
		args.buf_sz = size;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

#endif // CLIENT

}; // class KVstore

// remove application visibility
#undef memcpy
#undef memset

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
