
#ifndef STREAM_SC_HPP_
#define STREAM_SC_HPP_

#include "systemc.h" // sc_fifo_*
#include "port_strm.h" // AXI_TC_T

#define MAX_PORTS 2

extern sc_fifo_in <AXI_TC_T> *s_gctl[MAX_PORTS];
extern sc_fifo_out<AXI_TC_T> *m_gctl[MAX_PORTS];

#endif // STREAM_SC_HPP_
