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

#if defined(USE_ACC)
#define CACHE_BARRIER {dre.cache_flush(); dre.cache_invalidate(); host::cache_flush_invalidate();}
#define CACHE_DISPOSE(p,n) {dre.cache_invalidate(p,n); host::cache_invalidate(p,n);}

#else
#define CACHE_BARRIER {host::cache_flush_invalidate();}
#define CACHE_DISPOSE(p,n) {host::cache_invalidate(p,n);}
#endif

#if defined(USE_ACC)
#define CACHE_SEND_ALL {host::cache_flush(); /*dre.cache_flush(); dre.cache_invalidate();*/}
#define CACHE_RECV_ALL {/*dre.cache_flush();*/ host::cache_flush_invalidate();}
#define CACHE_SEND(d,p,n) {host::cache_flush(p,n); /*d.cache_invalidate(p,n);*/}
#define CACHE_RECV(d,p,n) {/*d.cache_flush(p,n);*/ host::cache_invalidate(p,n);}

#else
#define CACHE_SEND_ALL
#define CACHE_RECV_ALL
#define CACHE_SEND(d,p,n)
#define CACHE_RECV(d,p,n)
#endif

#if defined(ZYNQ)
#include "xpseudo_asm.h" // mtcp*, dsb
#include "xil_cache.h" // Xil_D*
#include "xil_mmu.h" // Xil_SetTlbAttributes
#if defined(__aarch64__)
#define Xil_L1DCacheFlush Xil_DCacheFlush
#define Xil_L1DCacheInvalidateRange Xil_DCacheInvalidateRange
#define dc_CVAC(va) mtcpdc(CVAC,(INTPTR)(va))
#define dc_CIVAC(va) mtcpdc(CIVAC,(INTPTR)(va))
#else
#include "xil_cache_l.h" // Xil_L1D*
#define dc_CVAC(va) mtcp(XREG_CP15_CLEAN_DC_LINE_MVA_POC,(INTPTR)(va))
#define dc_CIVAC(va) mtcp(XREG_CP15_CLEAN_INVAL_DC_LINE_MVA_POC,(INTPTR)(va))
#endif
namespace host {
inline void cache_init(void) {
#if !defined(__aarch64__)
	char *ptr;
	// Xil_ICacheEnable();
	// Xil_DCacheEnable();
	Xil_SetTlbAttributes((INTPTR)0x40000000, 0x04c06); /* Inner Cacheable */
	Xil_SetTlbAttributes((INTPTR)0x40100000, 0x04c06); /* Inner Cacheable */
	for (ptr = (char*)0x40200000; ptr < (char*)0x7fffffff; ptr += 0x100000)
		Xil_SetTlbAttributes((INTPTR)ptr, 0x15de6); /* Cacheable */
#endif
}
inline void cache_flush(void) {Xil_DCacheFlush();}
inline void cache_flush(const void *ptr, size_t size) {Xil_DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_flush_invalidate(void) {Xil_DCacheFlush();}
inline void cache_flush_invalidate(const void *ptr, size_t size) {Xil_DCacheFlushRange((INTPTR)ptr, size);}
inline void cache_invalidate(void) {Xil_DCacheInvalidate();}
inline void cache_invalidate(const void *ptr, size_t size) {Xil_DCacheInvalidateRange((INTPTR)ptr, size);}
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

/* ARM cache line management */
#define dc_CVAC(va)
#define dc_CIVAC(va)

/* Data Synchronization Barrier */
#define dsb()

/* flush & invalidate entire L1 data cache */
#define Xil_L1DCacheFlush()
/* invalidate range of L1 data cache */
#define Xil_L1DCacheInvalidateRange(addr, size)

#endif

#endif /* CACHE_H_ */
