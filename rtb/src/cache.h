/*
 * cache.h
 *
 *  Created on: Sep 16, 2014
 *      Author: lloyd23
 */

#ifndef CACHE_H_
#define CACHE_H_

//------------------ Cache ------------------//
// cache management on host and accelerator

/* NOTE: if invalidate is used on non-cache aligned and sized allocations, */
/* it can corrupt the heap. */

#if defined(USE_ACC)
#define CACHE_INVALIDATE(a,p,n) {a.cache_invalidate(p,n); host::cache_invalidate(p,n);}
#define CACHE_BARRIER(a) {a.cache_flush(); a.cache_invalidate(); host::cache_flush_invalidate();}

#else
#define CACHE_INVALIDATE(a,p,n) {/*host::cache_invalidate(p,n);*/}
#define CACHE_BARRIER(a) {host::cache_flush_invalidate();}
#endif

#if defined(USE_ACC)
#define CACHE_SEND_ALL(a) {host::cache_flush(); /*a.cache_flush(); a.cache_invalidate();*/}
#define CACHE_RECV_ALL(a) {/*a.cache_flush();*/ host::cache_flush_invalidate();}
// #define CACHE_SEND(a,p,n) {host::cache_flush(p,n); /*a.cache_invalidate(p,n);*/}
// #define CACHE_RECV(a,p,n) {/*a.cache_flush(p,n);*/ host::cache_invalidate(p,n);}
#define CACHE_SEND(a,p,n) {host::cache_flush(p,n); a.cache_invalidate(p,n);}
#define CACHE_RECV(a,p,n) {a.cache_flush(p,n); host::cache_invalidate(p,n);}

#else
#define CACHE_SEND_ALL(a)
#define CACHE_RECV_ALL(a)
#define CACHE_SEND(a,p,n)
#define CACHE_RECV(a,p,n)
#endif

#if defined(ZYNQ)
#include "xil_cache.h" // Xil_D*
#include "xil_cache_l.h" // Xil_L1D*
#include "xpseudo_asm.h" // mtcp, dsb
#include "xreg_cortexa9.h" // XREG_CP15_*
#include "xil_mmu.h" // Xil_SetTlbAttributes
namespace host {
inline void cache_init(void) {
	char *ptr;
	// Xil_ICacheEnable();
	// Xil_DCacheEnable();
	Xil_SetTlbAttributes((INTPTR)0x40000000, 0x4c0e); /* Inner Cacheable */
	Xil_SetTlbAttributes((INTPTR)0x40100000, 0x4c0e); /* Inner Cacheable */
	for (ptr = (char*)0x40200000; ptr < (char*)0x7fffffff; ptr += 0x100000)
		Xil_SetTlbAttributes((INTPTR)ptr, 0x15de6); /* Cacheable */
}
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_L1DCacheFlushRange((unsigned int)ptr, (unsigned)size);}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_L1DCacheInvalidateRange((unsigned int)ptr, (unsigned)size);}
} // namespace host

#else
namespace host {
inline void cache_init(void) {}
inline void cache_flush(void) {}
inline void cache_flush(const void *ptr, size_t size) {}
inline void cache_flush_invalidate(void) {}
inline void cache_flush_invalidate(const void *ptr, size_t size) {}
inline void cache_invalidate(void) {}
inline void cache_invalidate(const void *ptr, size_t size) {}
} // namespace host

/* ARM CP15 operations with mcr instruction */
#define mtcp(reg,val)
/* Data Synchronization Barrier */
#define dsb()

/* flush range of L1 & L2 data cache */
#define Xil_DCacheFlushRange(addr, size)

/* invalidate range of L1 & L2 data cache */
#define Xil_DCacheInvalidateRange(addr, size)

/* flush & invalidate entire L1 data cache */
#define Xil_L1DCacheFlush()

/* invalidate range of L1 data cache */
#define Xil_L1DCacheInvalidateRange(addr, size)

#endif

#endif /* CACHE_H_ */
