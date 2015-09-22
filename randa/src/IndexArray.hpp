#ifndef _INDEX_ARRAY_HPP
#define _INDEX_ARRAY_HPP

#if defined (SERVER)
#define USE_LSU 1
#endif
#if defined(CLIENT) || defined(SERVER) || defined(USE_LSU)
#define USE_STREAM 1
#endif
#if defined(USE_LSU)
#define USE_INDEX 1
#endif

#include <cstddef> // size_t
#include <algorithm> // min

#define CEIL(n,s) ((((n)+((s)-1)) / s) * s)
#define FLOOR(n,s) (((n) / s) * s)

/* * * * * * * * * * Access, Cache, Memory, and Stats * * * * * * * * * */

#define load(p,i) (p)[i]

#if defined(USE_DMAC)
#include <cstring> // memcpy, memset
#include "dmac_cmd.h"
#define smemcpy dmac_smemcpy
#define memcpy dmac_memcpy
#define memset ::memset

#elif defined(USE_LSU)
#include <cstring> // memcpy, memset
#include "lsu_cmd.h"
#define smemcpy lsu_smemcpy
#define memcpy lsu_memcpy
#define memset ::memset

#else // use CPU
#include <cstring> // memcpy, memset

// TODO: move to host support
#if 0
void *smemcpy(void *dst, const void *src, size_t block_sz, size_t dst_inc, size_t src_inc, size_t n)
{
	register char *rdst = (char *)dst;
	register const char *rsrc = (const char *)src;
	while (n) {
		memcpy(rdst, rsrc, block_sz);
		rdst += dst_inc;
		rsrc += src_inc;
		n--;
	}
	return dst;
}
#endif // use CPU

#define memcpy ::memcpy
#define memset ::memset
#endif

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

// Invalidate single data cache line from addr
// Assumes C_DCACHE_USE_WRITEBACK is 0, therefore rB (i.e. r0) is not used
#define wdcc(addr) __asm__ __volatile__ ("wdc.clear\t%0,r0\n" :: "d" (addr))

/* * * * * * * * * * Stream Support * * * * * * * * * */

#ifdef USE_STREAM
#include "stream.h"

#define FWD_ID 2

#ifdef __microblaze__
#include "fsl.h"
#define DEVICE_ID 0
#define RET_ID 1
#else
#include "xparameters.h"
#define DEVICE_ID XPAR_AXI_FIFO_0_DEVICE_ID
#define RET_ID 0
#endif

class stream_port {
public:

	stream_t port;

	stream_port(void) {
		stream_init(&port, DEVICE_ID);
	}
};
#endif // USE_STREAM

/* * * * * * * * * * Data Reorganization Engine * * * * * * * * * */

// basic configurations
// host  : base class + client methods : with or without load-store unit
// client: base class + client methods + protocol
// server: base class + protocol
// thread: client + server : emulate stream with pthreads.

// Define a base class with all the fields and methods.
// Then derive server and client classes from the base class.
// Override base functions and call explicitly when needed.

template<typename index_t>
class _IndexArray
: public cache
{
public:
	void *ref_base; // reference array base address
	size_t ref_elem_sz; // reference element size

	const index_t *idx_base; // index array base address
	size_t idx_len; // index array length

	/* increment view and reference pointers to next element */

	inline void _next_elem(char *&vptr, char *&rptr, size_t &idx)
	{
		idx++;
		if (idx == idx_len) return;
		vptr += ref_elem_sz;
		rptr = (char*)ref_base + idx_base[idx]*ref_elem_sz;
	}

public:

	/* setup data reorg engine */

	void setup(
		void *ref, size_t elem_sz,
		const void *index, size_t len
		)
	{
		ref_base = ref;
		ref_elem_sz = elem_sz;
		idx_base = (const index_t *)index;
		idx_len = len;
	}

	/* fill and drain assumptions */
	/* buf, buf_sz & offset are multiple of ref_elem_sz (view buffer is aligned) */
	/* buf_sz does not overreach end of view */
	/* offset is zero */

#ifdef USE_INDEX

#ifdef __microblaze__

#define RIDX_DELAY //__asm__ __volatile__("nop\n\tnop");

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		unsigned n = buf_sz/ref_elem_sz;
//		volatile index_t tmp; // preload indexes
//		for (unsigned i = 0; i < n; i++) tmp |= idx_base[i];

		/* indexed load */
		/* go=0, write=1, select=0, length=4, tid=gret_id, tdest=gfwd_id */
		putfslx(1 << 22 | 0 << 19 | 4 << 16 | gret_id << 8 | (gfwd_id+READ_CH), 0, FSL_DEFAULT);
		putfslx(0x00000000, 0, FSL_DEFAULT);            /* clear status */
		putfslx(CMD_index, 0, FSL_DEFAULT);             /* index command */
		putfslx((unsigned)ref_base, 0, FSL_DEFAULT);    /* base address */
		putfslx((unsigned)ref_elem_sz, 0, FSL_CONTROL); /* size */

		/* contiguous store (with stride command) */
		/* go=1, write=1, select=0, length=6, tid=gret_id, tdest=gfwd_id */
		putfslx(1 << 23 | 1 << 22 | 0 << 19 | 6 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH), 0, FSL_DEFAULT);
		putfslx(0x00000000, 0, FSL_DEFAULT);            /* clear status */
		putfslx(0x80 | CMD_smove, 0, FSL_DEFAULT);      /* request status, smove command */
		putfslx((unsigned)vptr, 0, FSL_DEFAULT);        /* address */
		putfslx((unsigned)ref_elem_sz, 0, FSL_DEFAULT); /* size */
		putfslx((unsigned)ref_elem_sz, 0, FSL_DEFAULT); /* increment */
		putfslx((unsigned)n, 0, FSL_CONTROL);           /* repetitions */

		/* feed indexes */
		/* go=1, write=1, select=4, length=1, tid=gret_id, tdest=gfwd_id */
		register unsigned header = 1 << 23 | 1 << 22 | 4 << 19 | 1 << 16 | gret_id << 8 | (gfwd_id+READ_CH);
		register const index_t *ptr = &idx_base[idx];
		register const index_t *end = ptr + n;
		while (end-ptr >= 8) {

			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[0], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[1], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[2], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[3], 0, FSL_CONTROL);
			RIDX_DELAY
//		wdcc(&ptr[0]);

			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[4], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[5], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[6], 0, FSL_CONTROL);
			RIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[7], 0, FSL_CONTROL);
			RIDX_DELAY
//		wdcc(&ptr[4]);

			ptr += 8;
		}
		while (ptr < end) {
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(*ptr++, 0, FSL_CONTROL);
			RIDX_DELAY
//			if (((size_t)ptr & 0xF) == 0) wdcc(&ptr[-4]);
		}

		/* receive store status */
		getfslx(header, 0, FSL_DEFAULT);
		getfslx(header, 0, FSL_CONTROL);
	}

	/* with normal clocks (ARM:600MHz-4:2:1, DRE:166MHz, delay:0) 0 nops: hang, 2&4 nops same time, 8 nops: time increase */
	/* with emulation clocks (ARM:133MHz-4:2:1, DRE:62MHz, delay:12-452) 19 nops:hang, 20 nops:1.77x20, 24 nops:1.87x20, 28 nops:1.97x20 sec */
	/* with emulation clocks (ARM:133MHz-4:2:1, DRE:62MHz, delay:0-455, pipe:8) 4 nops:hang, 5 nops:0.167 sec, 6 nops:0.167 sec, 8 nops:0.170 sec, 12 nops:0.184 sec, 16 nops:0.198 sec */
	/* with emulation clocks (ARM:133MHz-4:2:1, DRE:62MHz, delay:0-455, pipe:16) 0 nops:0.139, 1 nop:0.140, 2 nops:0.144, 3 nops:0.147, 4 nops:0.150, 5 nops:0.154 sec */
#define WIDX_DELAY //__asm__ __volatile__("nop");

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		unsigned n = buf_sz/ref_elem_sz;
//		volatile index_t tmp; // preload indexes
//		for (unsigned i = 0; i < n; i++) tmp |= idx_base[i];

		/* indexed store */
		/* go=0, write=1, select=0, length=4, tid=gret_id, tdest=gfwd_id */
		putfslx(1 << 22 | 0 << 19 | 4 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH), 0, FSL_DEFAULT);
		putfslx(0x00000000, 0, FSL_DEFAULT);            /* clear status */
		putfslx(CMD_index, 0, FSL_DEFAULT);             /* index command */
		putfslx((unsigned)ref_base, 0, FSL_DEFAULT);    /* base address */
		putfslx((unsigned)ref_elem_sz, 0, FSL_CONTROL); /* size */

		/* contiguous load (with stride command) */
		/* go=1, write=1, select=0, length=6, tid=gret_id, tdest=gfwd_id */
		putfslx(1 << 23 | 1 << 22 | 0 << 19 | 6 << 16 | gret_id << 8 | (gfwd_id+READ_CH), 0, FSL_DEFAULT);
		putfslx(0x00000000, 0, FSL_DEFAULT);            /* clear status */
		putfslx(CMD_smove, 0, FSL_DEFAULT);             /* smove command */
		putfslx((unsigned)vptr, 0, FSL_DEFAULT);        /* address */
		putfslx((unsigned)ref_elem_sz, 0, FSL_DEFAULT); /* size */
		putfslx((unsigned)ref_elem_sz, 0, FSL_DEFAULT); /* increment */
		putfslx((unsigned)n, 0, FSL_CONTROL);           /* repetitions */

		/* feed indexes */
		/* go=1, write=1, select=4, length=1, tid=gret_id, tdest=gfwd_id */
		register unsigned header = 1 << 23 | 1 << 22 | 4 << 19 | 1 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH);
		register const index_t *ptr = &idx_base[idx];
		register const index_t *end = ptr + (n-1);
		while (end-ptr >= 8) {

			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[0], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[1], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[2], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[3], 0, FSL_CONTROL);
			WIDX_DELAY
		wdcc(&ptr[0]);

			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[4], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[5], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[6], 0, FSL_CONTROL);
			WIDX_DELAY
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(ptr[7], 0, FSL_CONTROL);
			WIDX_DELAY
		wdcc(&ptr[4]);

			ptr += 8;
		}
		while (ptr < end) {
			putfslx(header, 0, FSL_DEFAULT);
			putfslx(*ptr++, 0, FSL_CONTROL);
			WIDX_DELAY
			if (((size_t)ptr & 0xF) == 0) wdcc(&ptr[-4]);
		}
		/* request response after last index */
		/* go=0, write=1, select=1, length=1, tid=gret_id, tdest=gfwd_id */
		putfslx(1 << 22 | 1 << 19 | 1 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH), 0, FSL_DEFAULT);
		putfslx(0x80|CMD_index, 0, FSL_CONTROL);
		/* last index */
		putfslx(header, 0, FSL_DEFAULT);
		putfslx(*ptr++, 0, FSL_CONTROL);
		if (((size_t)ptr & 0xF) == 0) wdcc(&ptr[-4]);

		/* receive store status */
		getfslx(header, 0, FSL_DEFAULT);
		getfslx(header, 0, FSL_CONTROL);
	}

#else // __microblaze__

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		unsigned n = buf_sz/ref_elem_sz;
		unsigned reg[7];

		/* indexed load */
		reg[1] = 0x00000000;            /* clear status */
		reg[2] = CMD_index;             /* index command */
		reg[3] = (unsigned)ref_base;    /* base address */
		reg[4] = (unsigned)ref_elem_sz; /* size */
		aport_nwrite(gfwd_id+READ_CH, gret_id, 0, 0, reg, 4);

		/* contiguous store (with stride command) */
		reg[1] = 0x00000000;            /* clear status */
		reg[2] = 0x80 | CMD_smove;      /* request status, smove command */
		reg[3] = (unsigned)vptr;        /* address */
		reg[4] = (unsigned)ref_elem_sz; /* size */
		reg[5] = (unsigned)ref_elem_sz; /* increment */
		reg[6] = (unsigned)n;           /* repetitions */
		aport_nwrite(gfwd_id+WRITE_CH, gret_id, 1, 0, reg, 6);

		/* feed indexes */
		/* go=1, write=1, select=4, length=1, tid=gret_id, tdest=gfwd_id */
		reg[0] = 1 << 23 | 1 << 22 | 4 << 19 | 1 << 16 | gret_id << 8 | (gfwd_id+READ_CH);
		for (size_t i = 0; i < n; i++) {
			reg[1] = idx_base[idx++];
			stream_send(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);
		}

		/* receive store status */
		stream_recv(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);
	}

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		unsigned n = buf_sz/ref_elem_sz;
		unsigned reg[7];

		/* indexed store */
		reg[1] = 0x00000000;            /* clear status */
		reg[2] = CMD_index;             /* index command */
		reg[3] = (unsigned)ref_base;    /* base address */
		reg[4] = (unsigned)ref_elem_sz; /* size */
		aport_nwrite(gfwd_id+WRITE_CH, gret_id, 0, 0, reg, 4);

		/* contiguous load (with stride command) */
		reg[1] = 0x00000000;            /* clear status */
		reg[2] = CMD_smove;             /* smove command */
		reg[3] = (unsigned)vptr;        /* address */
		reg[4] = (unsigned)ref_elem_sz; /* size */
		reg[5] = (unsigned)ref_elem_sz; /* increment */
		reg[6] = (unsigned)n;           /* repetitions */
		aport_nwrite(gfwd_id+READ_CH, gret_id, 1, 0, reg, 6);

		/* feed indexes */
		/* go=1, write=1, select=4, length=1, tid=gret_id, tdest=gfwd_id */
		reg[0] = 1 << 23 | 1 << 22 | 4 << 19 | 1 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH);
		for (size_t i = 0; i < n; i++) {
			reg[1] = idx_base[idx++];
			/* request response after last index */
			if (i == n-1) aport_write(gfwd_id+WRITE_CH, gret_id, 0, 1, 0x80|CMD_index);
			stream_send(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);
		}

		/* receive store status */
		stream_recv(gport, reg, 2*sizeof(unsigned), F_BEGP|F_ENDP);
	}

#endif // __microblaze__

#else // USE_INDEX

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		char *rptr = (char*)ref_base + idx_base[idx] * ref_elem_sz; /* pointer to current ref element */
		unsigned n = buf_sz/ref_elem_sz;

		while (n--) {
			memcpy(vptr, rptr, ref_elem_sz);
			_next_elem(vptr, rptr, idx);
		}
	}

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		size_t idx = 0; // offset / ref_elem_sz;
		char *vptr = (char*)buf; /* pointer to current view element */
		char *rptr = (char*)ref_base + idx_base[idx] * ref_elem_sz; /* pointer to current ref element */
		unsigned n = buf_sz/ref_elem_sz;

		while (n--) {
			memcpy(rptr, vptr, ref_elem_sz);
			_next_elem(vptr, rptr, idx);
		}
	}

#endif // USE_INDEX

	/* wait for operation to finish */

	void wait(void)
	{
		// NOTE: what this wait really means is a write barrier after a fill
		// TODO: The load-store unit needs a read and write barrier command
	}

	/* map view offset to reference offset */

	size_t view_to_ref(size_t offset)
	{
		return idx_base[offset / ref_elem_sz];
	}

}; // class _IndexArray


template<typename T>
class IndexArray
: public _IndexArray<T>
#ifdef USE_STREAM
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
		void *ref;
		size_t elem_sz;
		const void *index;
		size_t len;
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

	IndexArray(void)
	{
		// TODO: assign a load-store unit and control unit (port IDs)

		// options:
		// direct
		// direct with LSU support
		// client, start remote server - separate process, doesn't share memory
		// client, start thread server
#ifdef USE_LSU
		lsu_setport(&port, FWD_ID, RET_ID);
#endif
#ifdef USE_DMAC
		dmac_init(XPAR_XDMAPS_1_DEVICE_ID);
		dmac_setunit(0);
#endif
	}

#ifdef SERVER

#ifdef __microblaze__

	void command_server(void)
	{
		header hdr;
		size_t reg[4];

		for (;;) {
			getfsl_interruptible(*(unsigned *)&hdr, 0); getfsl(hdr.cmd, 0);
			switch (hdr.cmd) {
			case C_SETUP:
				getfsl(reg[0], 0); getfsl(reg[1], 0); getfsl(reg[2], 0); cgetfsl(reg[3], 0);
				// Consider keeping a history of reference images, invalidate
				// if haven't seen or let client call invalidate explicitly.
				//cache_invalidate((void *)reg[0], reg[1]*reg[3]);
				_IndexArray<T>::setup((void *)reg[0], reg[1], (const void *)reg[2], reg[3]);
				break;
			case C_FILL:
				getfsl(reg[0], 0); getfsl(reg[1], 0); cgetfsl(reg[2], 0);
				_IndexArray<T>::fill((void *)reg[0], reg[1], reg[2]);
				//cache_flush((void *)reg[0], reg[1]);
				break;
			case C_DRAIN:
				getfsl(reg[0], 0); getfsl(reg[1], 0); cgetfsl(reg[2], 0);
				//cache_invalidate((void *)reg[0], reg[1]);
				_IndexArray<T>::drain((void *)reg[0], reg[1], reg[2]);
				break;
			case C_WAIT:
				cgetfsl(reg[0], 0);
				break;
			case C_CFLUSH:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) _IndexArray<T>::cache_flush();
				else _IndexArray<T>::cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) _IndexArray<T>::cache_flush_invalidate(); // include flush
				else _IndexArray<T>::cache_invalidate((void *)reg[0], reg[1]);
				break;
			default:
				break;
			}
			register unsigned int tmp = hdr.tid;
			hdr.tid = hdr.tdest;
			hdr.tdest = tmp;
			putfsl(*(unsigned *)&hdr, 0); cputfsl(hdr.cmd, 0);
		}
	}

#else // __microblaze__

	void command_server(void)
	{
		header hdr;
		size_t reg[4];

		for (;;) {
			stream_recv(&port, &hdr, sizeof(hdr), F_BEGP);
			switch (hdr.cmd) {
			case C_SETUP:
				stream_recv(&port, reg, sizeof(size_t)*4, F_ENDP);
				// Consider keeping a history of reference images, invalidate
				// if haven't seen or let client call invalidate explicitly.
				//cache_invalidate((void *)reg[0], reg[1]*reg[3]);
				_IndexArray<T>::setup((void *)reg[0], reg[1], (const void *)reg[2], reg[3]);
				break;
			case C_FILL:
				stream_recv(&port, reg, sizeof(size_t)*3, F_ENDP);
				_IndexArray<T>::fill((void *)reg[0], reg[1], reg[2]);
				//cache_flush((void *)reg[0], reg[1]);
				break;
			case C_DRAIN:
				stream_recv(&port, reg, sizeof(size_t)*3, F_ENDP);
				//cache_invalidate((void *)reg[0], reg[1]);
				_IndexArray<T>::drain((void *)reg[0], reg[1], reg[2]);
				break;
			case C_WAIT:
				stream_recv(&port, reg, sizeof(size_t)*1, F_ENDP);
				break;
			case C_CFLUSH:
				stream_recv(&port, reg, sizeof(size_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) _IndexArray<T>::cache_flush();
				else _IndexArray<T>::cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				stream_recv(&port, reg, sizeof(size_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) _IndexArray<T>::cache_flush_invalidate(); // include flush
				else _IndexArray<T>::cache_invalidate((void *)reg[0], reg[1]);
				break;
			default:
				break;
			}
			register unsigned int tmp = hdr.tid;
			hdr.tid = hdr.tdest;
			hdr.tdest = tmp;
			stream_send(&port, &hdr, sizeof(hdr), F_BEGP|F_ENDP);
		}
	}

#endif // __microblaze__

#endif // SERVER

#ifdef CLIENT

#define MCU_ID 1
#define HST_ID 0

/* minimal resource management for DRE hardware */
#define MAX_DRE 1

	// associate DRE with client application class
	static IndexArray<T> *dre_client[MAX_DRE];

	/* setup data reorg engine */

	void _setup(
		void *ref, size_t elem_sz,
		const void *index, size_t len
		)
	{
		header hdr;
		header res;
		setup_args args;

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
		hdr.tuser = 0;
		hdr.cmd = C_SETUP;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.ref = ref;
		args.elem_sz = elem_sz;
		args.index = index;
		args.len = len;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
		dre_client[0] = this;
	}

	void setup(
		void *ref, size_t elem_sz,
		const void *index, size_t len
		)
	{
		_IndexArray<T>::setup(ref, elem_sz, index, len);
		_setup(ref, elem_sz, index, len);
	}

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		header hdr;
		header res;
		buf_args args;

		if (dre_client[0] != this)
			_setup(_IndexArray<T>::ref_base, _IndexArray<T>::ref_elem_sz, _IndexArray<T>::idx_base, _IndexArray<T>::idx_len);

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
		hdr.tuser = 0;
		hdr.cmd = C_FILL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = buf;
		args.buf_sz = buf_sz;
		args.offset = offset;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	/* drain buffer starting from view offset */

	void drain(void *buf, size_t buf_sz, size_t offset)
	{
		header hdr;
		header res;
		buf_args args;

		if (dre_client[0] != this)
			_setup(_IndexArray<T>::ref_base, _IndexArray<T>::ref_elem_sz, _IndexArray<T>::idx_base, _IndexArray<T>::idx_len);

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
		hdr.tuser = 0;
		hdr.cmd = C_DRAIN;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = buf;
		args.buf_sz = buf_sz;
		args.offset = offset;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

	/* wait for operation to finish */

#if 0
	void wait(void)
	{
		header hdr;
		header res;
		size_t args;

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
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

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
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

		hdr.tdest = MCU_ID << 1;
		hdr.tid = HST_ID << 1;
		hdr.tuser = 0;
		hdr.cmd = C_CINVAL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ptr;
		args.buf_sz = size;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

#endif // CLIENT

}; // class IndexArray

#ifdef CLIENT
template<typename T> IndexArray<T> *IndexArray<T>::dre_client[MAX_DRE];
#endif

// let them be used by application
//#undef load
//#undef memcpy
//#undef memset

#endif // _INDEX_ARRAY_HPP
