#ifndef _DECIMATE_2D_HPP
#define _DECIMATE_2D_HPP

#include <cstddef> // size_t
#include <algorithm> // min
#include "config.h"
#include "alloc.h" // FLOOR

/* * * * * * * * * * Access, Cache, Memory, and Stats * * * * * * * * * */

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

#define memcpy ::memcpy
#define memset ::memset
#endif

#if defined(SERVER) && defined(__microblaze__)
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

/* * * * * * * * * * Stream Support * * * * * * * * * */

#if defined(USE_STREAM)
#include "aport.h"

#define FWD_PN LSU0_PN

#if defined(__microblaze__)
#include "fsl.h"
#define RET_PN MCU0_PN
#else
#define RET_PN ARM0_PN
#endif

#if defined(ZYNQ)
#include "xparameters.h"
#define STREAM_DEVICE_ID XPAR_AXI_FIFO_0_DEVICE_ID
#else
#define STREAM_DEVICE_ID 0
#endif

class stream_port {
public:

	stream_t port;

	stream_port(void) {
		stream_init(&port, STREAM_DEVICE_ID);
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

class _Decimate2D
: public cache
{
public:
	size_t factor; // decimation factor

	// TODO: consider reordering these so they can
	// be sent contiguously in a command to the LSU

	void *ref_base; // base address of reference array
	size_t ref_w;
	size_t ref_h;
	size_t ref_stride; // reference stride includes decimation factor
	size_t ref_inc; // reference increment includes decimation factor

	size_t view_w;
	size_t view_h;
	size_t view_stride;
	size_t view_inc;

	/*
	TODO: Consider adding ref_off, view_off, and elem_sz members.
	The _inc values need not be equal to the elem_sz.
	(e.g. pick green byte from 3-byte pixel).
	*/

	/* increment view and reference pointers to next element */

	inline void _next_elem(char *&vptr, char *&rptr, char *&vnext, char *&rnext)
	{
		vptr += view_inc;
		rptr += ref_inc;
		if (vptr+view_inc > vnext) {
			vptr = vnext; vnext += view_stride;
			rptr = rnext; rnext += ref_stride;
		}
	}

public:

	/* setup data reorg engine */

	void setup(
		void *ref, size_t ref_width, size_t ref_height,
		size_t elem_sz, size_t decimate
		)
	{
		factor = decimate;

		ref_base = ref;
		ref_w = ref_width;
		ref_h = ref_height;
		ref_stride = elem_sz*ref_width*decimate;
		ref_inc = elem_sz*decimate;

		view_w = ref_width/decimate;
		view_h = ref_height/decimate;
		view_stride = elem_sz*(ref_width/decimate);
		view_inc = elem_sz;
	}

	/* fill assumptions */
	/* buf, buf_sz & offset are multiple of view_inc (view buffer is aligned) */
	/* buf_sz does not overreach end of view */

#ifdef __microblaze__

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t voff = offset;
		size_t roff = view_to_ref(voff);
		char *vptr = (char*)buf; /* pointer to current view element */
		char *rptr = (char*)ref_base + roff; /* pointer to current ref element */
		char *vnext = vptr + view_stride - (voff % view_stride); /* view next row */
		char *rnext = rptr + ref_stride - (roff % ref_stride); /* ref next row */
		char *last = (char*)buf + buf_sz; /* last element */

		while (vptr < last) {
			size_t n = (std::min(last, vnext) - vptr)/view_inc;
			//smemcpy(vptr, rptr, view_inc, view_inc, ref_inc, n);

			/* strided load */
			/* go=1, write=1, select=0, length=6, tid=gret_id, tdest=gfwd_id */
			putfslx(1 << 23 | 1 << 22 | 0 << 19 | 6 << 16 | gret_id << 8 | (gfwd_id+READ_CH), 0, FSL_DEFAULT);
			putfslx(0x00000000, 0, FSL_DEFAULT); /* clear status */
			putfslx(LSU_smove, 0, FSL_DEFAULT);  /* smove command */
			putfslx(rptr, 0, FSL_DEFAULT);       /* address */
			putfslx(view_inc, 0, FSL_DEFAULT);   /* size */
			putfslx(ref_inc, 0, FSL_DEFAULT);    /* increment */
			putfslx(n, 0, FSL_CONTROL);          /* repetitions */

			/* contiguous store (with stride command) */
			/* go=1, write=1, select=0, length=6, tid=gret_id, tdest=gfwd_id */
			putfslx(1 << 23 | 1 << 22 | 0 << 19 | 6 << 16 | gret_id << 8 | (gfwd_id+WRITE_CH), 0, FSL_DEFAULT);
			putfslx(0x00000000, 0, FSL_DEFAULT);       /* clear status */
			putfslx(0x80 | LSU_smove, 0, FSL_DEFAULT); /* request status, smove command */
			putfslx(vptr, 0, FSL_DEFAULT);             /* address */
			putfslx(view_inc, 0, FSL_DEFAULT);         /* size */
			putfslx(view_inc, 0, FSL_DEFAULT);         /* increment */
			putfslx(n, 0, FSL_CONTROL);                /* repetitions */

			/* receive store status */
			flit_t tmp;
			getfslx(tmp, 0, FSL_DEFAULT);
			getfslx(tmp, 0, FSL_CONTROL);

			vptr += view_inc*n;
			rptr += ref_inc*n;
			if (vptr+view_inc > vnext) {
				vptr = vnext; vnext += view_stride;
				rptr = rnext; rnext += ref_stride;
			}
		}
	}

#else // __microblaze__

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		size_t voff = FLOOR(offset, view_inc);
		size_t roff = view_to_ref(voff);
		size_t diff = offset - voff;
		char *vptr = (char*)buf - diff; /* pointer to current view element */
		char *rptr = (char*)ref_base + roff; /* pointer to current ref element */
		char *vnext = vptr + view_stride - (voff % view_stride); /* view next row */
		char *rnext = rptr + ref_stride - (roff % ref_stride); /* ref next row */
		size_t tmp1 = FLOOR(buf_sz, view_inc);
		size_t tmp2 = view_stride*view_h - voff;
		char *last = (char*)buf + std::min(tmp1, tmp2); /* last element */

		/* begin boundary case */
		if (diff) {
			memcpy(vptr+diff, rptr+diff, view_inc-diff);
			_next_elem(vptr, rptr, vnext, rnext);
		}

		/* mid block */
		while (vptr < last) {
//			memcpy(vptr, rptr, view_inc);
//			_next_elem(vptr, rptr, vnext, rnext);
			size_t n = (std::min(last, vnext) - vptr)/view_inc;
			smemcpy(vptr, rptr, view_inc, view_inc, ref_inc, n);
			vptr += view_inc*n;
			rptr += ref_inc*n;
			if (vptr+view_inc > vnext) {
				vptr = vnext; vnext += view_stride;
				rptr = rnext; rnext += ref_stride;
			}
		}

		/* end boundary case */
		if (vptr < (char*)buf+buf_sz && vptr < (char*)buf+tmp2) {
			ptrdiff_t diff = (char*)buf+buf_sz - vptr;
			memcpy(vptr, rptr, diff);
		}
	}

#endif // __microblaze__

	/* wait for operation to finish */

	void wait(void)
	{
		// NOTE: what this wait really means is a write barrier after a fill
		// TODO: The load-store unit needs a read and write barrier command
	}

	/* given view coordinate, return view offset */

	size_t view_offset(int vx, int vy)
	{
		return view_stride*vy + view_inc*vx;
	}

	/* given view coordinate, return reference offset */

	size_t ref_offset(int vx, int vy)
	{
		return ref_stride*vy + ref_inc*vx;
	}

	/* map view offset to reference offset */

	size_t view_to_ref(size_t offset)
	{
		size_t y = offset / view_stride;
		size_t x = (offset % view_stride) / view_inc;
		return ref_offset(x, y);
	}

}; // class _Decimate2D


#ifdef CLIENT

/* minimal resource management for DRE hardware */
#define MAX_DRE 1
#define HST_PN ARM0_PN
#define MCU_PN MCU0_PN

class Decimate2D;

// associate DRE with client application class
Decimate2D *dre_client[MAX_DRE];

#endif // CLIENT


class Decimate2D
: public _Decimate2D
#ifdef USE_STREAM
, stream_port
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
		flit_t ref;
		flit_t ref_width;
		flit_t ref_height;
		flit_t elem_sz;
		flit_t decimate;
	} setup_args;

	typedef struct {
		flit_t buf;
		flit_t buf_sz;
		flit_t offset;
	} fill_args;

	typedef struct {
		flit_t buf;
		flit_t buf_sz;
	} cache_args;

	enum {C_NOP, C_SETUP, C_FILL, C_WAIT, C_CFLUSH, C_CINVAL};

#endif // defined(CLIENT) || defined(SERVER)

public:

	Decimate2D(void)
	{
		// options:
		// direct
		// direct with LSU support
		// client, start remote server - separate process, doesn't share memory
		// client, start thread server
#ifdef USE_LSU
		lsu_setport(&port, FWD_PN, RET_PN);
#elif defined(USE_STREAM)
		aport_set(&port);
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
		flit_t reg[5];

		for (;;) {
			getfsl_interruptible(*(flit_t *)&hdr, 0); getfsl(hdr.cmd, 0);
			switch (hdr.cmd) {
			case C_SETUP:
				getfsl(reg[0], 0); getfsl(reg[1], 0); getfsl(reg[2], 0); getfsl(reg[3], 0); cgetfsl(reg[4], 0);
				// Consider keeping a history of reference images, invalidate
				// if haven't seen or let client call invalidate explicitly.
				//cache_invalidate((void *)reg[0], reg[1]*reg[2]*reg[3]);
				setup((void *)reg[0], reg[1], reg[2], reg[3], reg[4]);
				break;
			case C_FILL:
				getfsl(reg[0], 0); getfsl(reg[1], 0); cgetfsl(reg[2], 0);
				fill((void *)reg[0], reg[1], reg[2]);
				//cache_flush((void *)reg[0], reg[1]);
				break;
			case C_WAIT:
				cgetfsl(reg[0], 0);
				break;
			case C_CFLUSH:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) cache_flush();
				else cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				getfsl(reg[0], 0); cgetfsl(reg[1], 0);
				if ((void *)reg[0] == NULL) cache_flush_invalidate(); // include flush
				else cache_invalidate((void *)reg[0], reg[1]);
				break;
			default:
				break;
			}
			register unsigned int tmp = hdr.tid;
			hdr.tid = hdr.tdest;
			hdr.tdest = tmp;
			putfsl(*(flit_t *)&hdr, 0); cputfsl(hdr.cmd, 0);
		}
	}

#else // __microblaze__

	void command_server(void)
	{
		header hdr;
		flit_t reg[5];

		for (;;) {
			stream_recv(&port, &hdr, sizeof(hdr), F_BEGP);
			switch (hdr.cmd) {
			case C_SETUP:
				stream_recv(&port, reg, sizeof(flit_t)*5, F_ENDP);
				// Consider keeping a history of reference images, invalidate
				// if haven't seen or let client call invalidate explicitly.
				//cache_invalidate((void *)reg[0], reg[1]*reg[2]*reg[3]);
				setup((void *)reg[0], reg[1], reg[2], reg[3], reg[4]);
				break;
			case C_FILL:
				stream_recv(&port, reg, sizeof(flit_t)*3, F_ENDP);
				fill((void *)reg[0], reg[1], reg[2]);
				//cache_flush((void *)reg[0], reg[1]);
				break;
			case C_WAIT:
				stream_recv(&port, reg, sizeof(flit_t)*1, F_ENDP);
				break;
			case C_CFLUSH:
				stream_recv(&port, reg, sizeof(flit_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) cache_flush();
				else cache_flush((void *)reg[0], reg[1]);
				break;
			case C_CINVAL:
				stream_recv(&port, reg, sizeof(flit_t)*2, F_ENDP);
				if ((void *)reg[0] == NULL) cache_flush_invalidate(); // include flush
				else cache_invalidate((void *)reg[0], reg[1]);
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

	/* setup data reorg engine */

	void _setup(
		void *ref, size_t ref_width, size_t ref_height,
		size_t elem_sz, size_t decimate
		)
	{
		header hdr;
		header res;
		setup_args args;

		hdr.tdest = getID(MCU_PN);
		hdr.tid = getID(HST_PN);
		hdr.tuser = 0;
		hdr.cmd = C_SETUP;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.ref = ATRAN(ref);
		args.ref_width = ref_width;
		args.ref_height = ref_height;
		args.elem_sz = elem_sz;
		args.decimate = decimate;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
		dre_client[0] = this;
	}

	void setup(
		void *ref, size_t ref_width, size_t ref_height,
		size_t elem_sz, size_t decimate
		)
	{
		_Decimate2D::setup(ref, ref_width, ref_height, elem_sz, decimate);
		_setup(ref, ref_width, ref_height, elem_sz, decimate);
	}

	/* fill buffer starting from view offset */

	void fill(void *buf, size_t buf_sz, size_t offset)
	{
		header hdr;
		header res;
		fill_args args;

		if (dre_client[0] != this)
			_setup(ref_base, ref_w, ref_h, view_inc, factor);

		hdr.tdest = getID(MCU_PN);
		hdr.tid = getID(HST_PN);
		hdr.tuser = 0;
		hdr.cmd = C_FILL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ATRAN(buf);
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
		flit_t args;

		hdr.tdest = getID(MCU_PN);
		hdr.tid = getID(HST_PN);
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

		hdr.tdest = getID(MCU_PN);
		hdr.tid = getID(HST_PN);
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

		hdr.tdest = getID(MCU_PN);
		hdr.tid = getID(HST_PN);
		hdr.tuser = 0;
		hdr.cmd = C_CINVAL;
		stream_send(&port, &hdr, sizeof(hdr), F_BEGP);

		args.buf = ATRAN(ptr);
		args.buf_sz = size;
		stream_send(&port, &args, sizeof(args), F_ENDP);

		stream_recv(&port, &res, sizeof(res), F_BEGP|F_ENDP);
	}

#endif // CLIENT

}; // class Decimate2D

// let them be used by application
//#undef memcpy
//#undef memset

#endif // _DECIMATE_2D_HPP
