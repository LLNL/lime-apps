
#ifndef _CSWITCH_H
#define _CSWITCH_H

#include "systemc.h"

#include "port_strm.h"

// Xilinx specific:
// When depth is (1 >= D >= 11) internal FIFOs
// are implemented using shift registers.

#define QDEPTH 11

template <int N> // constraint: power of 2
SC_MODULE(csw_demux)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_port;
	sc_fifo_out<AXI_TC_T> m_port[N];

	void ct_demux()
	{
		AXI_TC flit;
		while (true) {
			wait();
			flit = s_port.read();
// printf("csw_demux<%d> in tdest:%02X tid:%02X tdata:%08X tlast:%1X\n", N, flit.tdest.to_uint(), flit.tid.to_uint(), flit.tdata.to_uint(), flit.tlast.to_uint());
			// FIXME: use another template arg to specify a routing algorithm,
			// or specify a routing table.
			if (N == 2) {
				// printf("csw_demux<%d> out port:%d\n", N, flit.tdest.to_uint() & (N-1));
				m_port[flit.tdest.to_uint() & (N-1)].write(flit);
			} else {
				// printf("csw_demux<%d> out port:%d\n", N, (flit.tdest.to_uint() >> 1) & (N-1));
				m_port[(flit.tdest.to_uint() >> 1) & (N-1)].write(flit);
			}
		}
	}

	SC_CTOR(csw_demux)
	{
		SC_CTHREAD(ct_demux, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

template <int N>
SC_MODULE(csw_mux)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_port[N];
	sc_fifo_out<AXI_TC_T> m_port;

	void ct_mux()
	{
		while (true) {
			wait();
			// printf("csw_mux<%d> wake\n", N);
			for (int i = 0; i < N; i++) // priority encoder & mux
				if (s_port[i].num_available()) {
					// printf("csw_mux<%d> in port:%d\n", N, i);
					m_port.write(s_port[i].read());
					break;
				}
		}
	}

	SC_CTOR(csw_mux)
	{
		SC_CTHREAD(ct_mux, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

// change the default size of the FIFO.
template <class T>
class sc_fifoQ : public sc_fifo<T> {
	public:
		explicit sc_fifoQ(int size_ = QDEPTH) : sc_fifo<T>(size_) {}
		explicit sc_fifoQ(const char* name_, int size_ = QDEPTH) : sc_fifo<T>(name_, size_) {}
};

// sc_fifo<AXI_TC_T>* qcreator(const char* name, size_t i)
// {
	// return new sc_fifo<AXI_TC_T>(name, QDEPTH);
// }

template <int N>
SC_MODULE(cswitch)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_port[N];
	sc_fifo_out<AXI_TC_T> m_port[N];

	sc_fifoQ<AXI_TC_T> queue[N][N];

	sc_vector<csw_demux<N> > u_csw_demux;
	sc_vector<csw_mux<N> > u_csw_mux;

	SC_CTOR(cswitch) :
		u_csw_demux("u_csw_demux", N),
		u_csw_mux("u_csw_mux", N)
	{
		for (int i = 0; i < N; i++) {
			u_csw_demux[i].clk(clk);
			u_csw_demux[i].reset(reset);
			u_csw_demux[i].s_port(s_port[i]);
			for (int j = 0; j < N; j++) {
				u_csw_demux[i].m_port[j](queue[i][j]);
			}

			u_csw_mux[i].clk(clk);
			u_csw_mux[i].reset(reset);
			for (int j = 0; j < N; j++) {
				u_csw_mux[i].s_port[j](queue[j][i]);
			}
			u_csw_mux[i].m_port(m_port[i]);
		}
	}
};

template <int N>
SC_MODULE(ctree)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_root;
	sc_fifo_out<AXI_TC_T> m_root;

	sc_fifo_in <AXI_TC_T> s_port[N];
	sc_fifo_out<AXI_TC_T> m_port[N];

	csw_demux<N> u_csw_demux;
	csw_mux<N> u_csw_mux;

	SC_CTOR(ctree) :
		u_csw_demux("u_csw_demux"),
		u_csw_mux("u_csw_mux")
	{
		u_csw_demux.clk(clk);
		u_csw_demux.reset(reset);
		u_csw_demux.s_port(s_root);
		for (int i = 0; i < N; i++) {
			u_csw_demux.m_port[i](m_port[i]);
		}

		u_csw_mux.clk(clk);
		u_csw_mux.reset(reset);
		for (int i = 0; i < N; i++) {
			u_csw_mux.s_port[i](s_port[i]);
		}
		u_csw_mux.m_port(m_root);
	}

};

#endif // _CSWITCH_H
