/*
 * alloc.h
 *
 *  Created on: Sep 16, 2014
 *      Author: lloyd23
 */

#ifndef ALLOC_H_
#define ALLOC_H_

//------------------ Allocation ------------------//

/* Since cache management is done explicitly, allocations must be */
/* aligned to the cache line size which is 32 bytes for the ARM A9 */
/* and 64 bytes for the x86. */

#ifdef __cplusplus

#if defined(USE_LSU) || defined(USE_DMAC) || defined(CLIENT)
// FIXME: NEWA doesn't construct, only works for simple types
// TODO: make allocator for accelerator
#include <malloc.h> // memalign, free
#define ALLOCATOR(t)
#define NEWA(t,n) (t*)memalign(32,(n)*sizeof(t))
#define DELETEA(p) free(p)
#define NALLOC(t,n) (t*)memalign(32,(n)*sizeof(t))
#define NFREE(p) free(p)

#else
#include <memory> // std::allocator
#include <cstdlib> // std::malloc, std::free
#define ALLOCATOR(t) std::allocator<t>
#define NEWA(t,n) new t [n]
#define DELETEA(p) delete[] p
#define NALLOC(t,n) (t*)std::malloc((n)*sizeof(t))
#define NFREE(p) std::free(p)
#endif

#else // __cplusplus

#if defined(USE_LSU) || defined(USE_DMAC) || defined(CLIENT)
#include <malloc.h> // memalign, free
#define NALLOC(t,n) (t*)memalign(32,(n)*sizeof(t))
#define NFREE(p) free(p)

#else
#include <stdlib.h> // malloc, free
#define NALLOC(t,n) (t*)malloc((n)*sizeof(t))
#define NFREE(p) free(p)
#endif

#endif // __cplusplus

#if defined(USE_SP)
/* This feature is currently only supported on the emulator */
/* Scratch Pad (SP) memory must not have L2 cache enabled. */
/* This means the ARM page table attribute must be outer non-cacheable. */
//#include "xparameters.h" // XPAR_PS7_RAM_0_S_AXI_BASEADDR
extern char _heap_start[];

#if defined(USE_OCM)
/* SRAM */
//#define SP_BEG (((unsigned)&_heap_start) & 0xC0000000U)
//#define SP_END ((((unsigned)&_heap_start) & 0xC0000000U) + 0x030000)
//#define SP_SIZE 0x030000
/* DRAM in SRAM address space, ADDR < 0x00100000 */
#define SP_BEG ((((unsigned)&_heap_start) & 0xC0000000U) + 0x080000)
#define SP_END ((((unsigned)&_heap_start) & 0xC0000000U) + 0x100000)
#define SP_SIZE 0x080000
#else /* USE_OCM */
/* DRAM in DRAM address space, ADDR >= 0x00100000 */
#define SP_BEG ((((unsigned)&_heap_start) & 0xC0000000U) + 0x100000)
#define SP_END ((((unsigned)&_heap_start) & 0xC0000000U) + 0x200000)
#define SP_SIZE 0x100000
#endif /* USE_OCM */
#endif /* USE_SP */

#endif /* ALLOC_H_ */
