
#include "stream.h"

#if defined(SYSTEMC)

#include <stdint.h> /* uintXX_t */
#include "stream_sc.h"

#define XST_SUCCESS 0
#define XST_FAILURE 1

sc_fifo_in <AXI_TC_T> *s_gctl[MAX_PORTS];
sc_fifo_out<AXI_TC_T> *m_gctl[MAX_PORTS];

#define STREAM_WRITE(id,flit) m_gctl[id]->write(flit)
#define STREAM_READ(id) s_gctl[id]->read()


int stream_init(stream_t *port, unsigned id)
{
	if (id >= MAX_PORTS) return XST_FAILURE;
	port->id = id;
	return XST_SUCCESS;
}

int stream_send(stream_t *port, void *buf, size_t size, unsigned flags)
{
	uint32_t *wptr = (uint32_t*)buf;
	unsigned words = size/sizeof(uint32_t);;
	AXI_TC flit;

	if ((flags & F_BEGP) && words) {
		port->m_hdr = *(hdr_t *)wptr++; words--;
// printf("stream_send begp tdest:%02X tid:%02X tuser:%04X\n", port->m_hdr.tdest, port->m_hdr.tid, port->m_hdr.tuser);
	}
	flit.tdest = port->m_hdr.tdest;
	flit.tid   = port->m_hdr.tid;
	flit.tuser = port->m_hdr.tuser;
	flit.tlast = 0;
	while (words > 1) {
		flit.tdata = *wptr++; words--;
// printf("stream_send tdest:%02X tid:%02X tdata:%08X tlast:%1X\n", flit.tdest.to_uint(), flit.tid.to_uint(), flit.tdata.to_uint(), flit.tlast.to_uint());
		STREAM_WRITE(port->id,flit);
	}
	flit.tlast = (flags & F_ENDP) != 0;
	if (words) {
		flit.tdata = *wptr;
// printf("stream_send last tdest:%02X tid:%02X tdata:%08X tlast:%1X\n", flit.tdest.to_uint(), flit.tid.to_uint(), flit.tdata.to_uint(), flit.tlast.to_uint());
		STREAM_WRITE(port->id,flit);
	}

	return XST_SUCCESS;
}

int stream_recv(stream_t *port, void *buf, size_t size, unsigned flags)
{
	uint32_t *wptr = (uint32_t*)buf;
	unsigned words = size/sizeof(uint32_t);
	AXI_TC flit;

	if ((flags & F_BEGP) && words) {
		flit = STREAM_READ(port->id);
		port->s_hdr.tdest = flit.tdest;
		port->s_hdr.tid   = flit.tid;
		port->s_hdr.tuser = (RACC::ui_t)flit.tuser;
		*(hdr_t *)wptr++ = port->s_hdr; words--;
		if (words) {*wptr++ = flit.tdata.to_uint(); words--;}
	}
	while (words > 1) {
		flit = STREAM_READ(port->id);
		*wptr++ = flit.tdata.to_uint(); words--;
	}
	if (words) {
		flit = STREAM_READ(port->id);
		*wptr = flit.tdata.to_uint();
		if (((flags & F_ENDP) != 0) != flit.tlast) return XST_FAILURE;
	}

	return XST_SUCCESS;
}

#endif
