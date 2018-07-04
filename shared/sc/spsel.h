
#ifndef _SPSEL_H
#define _SPSEL_H

#include "systemc.h"

#include "port_strm.h"


SC_MODULE(spsel)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	// Top-level ports are restricted to: ap_mem_if, AXI4M_bus_port,
	// sc_in_clk, sc_in, sc_out, sc_inout, sc_fifo_in, sc_fifo_out
	// (UG902 High-Level Synthesis, pg 402)

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <AXI_TD_T> s_dat1;
	sc_fifo_out<AXI_TD_T> m_dat1;
	sc_fifo_in <AXI_TD_T> s_dat2;
	sc_fifo_out<AXI_TD_T> m_dat2;

	// Do not use simple variables for communication between threads,
	// e.g. int, bool; instead use sc_signal, sc_fifo.
	// (UG902 High-Level Synthesis, pg 401)

	void ct_ctl()
	{
		// This will invoke default ctor for flit (reset behavior).
		AXI_TC flit;
		while (true) {
			// The first wait separates reset behavior from operational
			// behavior (SystemC Synthesizable Subset Version 1.4.7, pg 12).
			wait();
			flit = s_ctl.read();
			m_ctl.write(flit);
#ifndef __SYNTHESIS__
			// cout << name() << "::ct_ctl ts: " << sc_time_stamp() << ", flit: " << flit << endl;
#endif
		}
	}

	void ct_dat()
	{
		// This will invoke default ctor for flit (reset behavior).
		AXI_TD flit;
		while (true) {
			// The first wait separates reset behavior from operational
			// behavior (SystemC Synthesizable Subset Version 1.4.7, pg 12).
			wait();
			flit = s_dat1.read();
			m_dat1.write(flit);
			m_dat2.write(flit);
#ifndef __SYNTHESIS__
			// cout << name() << "::ct_dat ts: " << sc_time_stamp() << ", flit: " << flit << endl;
#endif
		}
	}

	SC_CTOR(spsel)
		// Warning! Synthesis will NOT invoke default ctor for these.
		// (SystemC Synthesizable Subset Version 1.4.7, pg 13).
		// : clk("clk"),
		// reset("reset")
	{
		// The module constructor can only define or instantiate modules.
		// It cannot contain any functionality.
		// (UG902 High-Level Synthesis, pg 389)
		SC_CTHREAD(ct_ctl, clk.pos());
		reset_signal_is(reset, RLEVEL);
		SC_CTHREAD(ct_dat, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

#endif // _SPSEL_H
