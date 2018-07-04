
#ifndef _CTLREG_H
#define _CTLREG_H

#include "systemc.h"

#include "ports.h"


#if defined(__SYNTHESIS__)
#define ACMD_T   ACMD::ui_t
#else
#define ACMD_T   ACMD
#endif

#define ACMD_CONCAT \
	(drac.go,drac.wr,drac.sel,drac.len,srac.go,srac.wr,srac.sel,srac.len,did,sid)

class ACMD {
public:
	typedef sc_uint<(2*CI+2*CU)> ui_t;

	sc_uint<CI> sid; // source id
	sc_uint<CI> did; // destination id
	RACC srac;       // source register access command
	RACC drac;       // destination register access command

	ACMD(ui_t ui = 0)
	{
		ACMD_CONCAT = ui;
	}

	ACMD & operator=(ui_t ui)
	{
		ACMD_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return ACMD_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const ACMD &flit)
{
	return o << hex << flit.drac << ':' << flit.srac << ':' << flit.did << ':' << flit.sid;
}


SC_MODULE(ctlreg) // control port register access
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <ACMD_T> s_cmd;
	sc_fifo_out<ACMD_T> m_cmd;

	// TODO: refactor into:
	// sc_in <sc_uint<CD> > s_vreg[CR];
	// sc_in <bool> s_vwe[CR];
	// sc_out<sc_uint<CD> > m_vreg[CR];
	sc_in <sc_bv<CD*CR> > s_vreg;
	sc_in <sc_bv<CR> >    s_vwe;
	sc_out<sc_bv<CD*CR> > m_vreg;

	// TODO: rename with c_
	sc_fifo<ACMD_T> i_cmd;
	sc_signal<sc_bv<CD> > reg[CR];
	sc_signal<sc_uint<CD> > ain;
	sc_signal<sc_uint<CA> > asel;
	sc_signal<bool>         awe;

// can sc_fifo read-write be used with a SC_METHOD process?
// how to synchronize vector register write with response messages?
// use a response FIFO of depth 1

	void ct_command()
	{
		AXI_TC flit;
		while (true) {
			wait();
			ain.write(0);
			asel.write(0);
			awe.write(false);
			flit = s_ctl.read();
			if (flit.tuser.wr) { // write, signal write process
				sc_uint<CA> len = flit.tuser.len;
				sc_uint<CA> sel = flit.tuser.sel;
				cout << name() << "::ct_command ts: " << sc_time_stamp() << ", flit: " << flit << endl;
				ain.write(flit.tdata);
				asel.write(sel++);
				awe.write(true);
				while (--len) {
					wait();
					flit = s_ctl.read();
					cout << name() << "::ct_command ts: " << sc_time_stamp() << ", flit: " << flit << endl;
					ain.write(flit.tdata);
					asel.write(sel++);
					awe.write(true);
				}
			} else { // read, send to response process, tdata not used
				ACMD cmd;
				cmd.sid = flit.tdest; // swap source and destination
				cmd.did = flit.tid;
				cmd.srac = flit.tuser; // command is source of response
				cmd.drac = flit.tuser; // echo register access
				cout << name() << "::ct_command ts: " << sc_time_stamp() << ", cmd: " << cmd << endl;
				i_cmd.write(cmd);
			}
			if (flit.tuser.go) { // go, send to command interpreter, tdata not used
				ACMD cmd;
				cmd.sid = flit.tid;
				cmd.did = flit.tdest;
				cmd.srac = 0; // unknown source register access
				cmd.drac = flit.tuser;
				cout << name() << "::ct_command ts: " << sc_time_stamp() << ", cmd: " << cmd << endl;
				m_cmd.write(cmd);
			}
		}
	}

	void ct_response()
	{
		ACMD cmd;
		while (true) {
			wait();
			if (i_cmd.num_available()) cmd = i_cmd.read();
			else if (s_cmd.num_available()) cmd = s_cmd.read();
			else continue;
			cout << name() << "::ct_response ts: " << sc_time_stamp() << ", cmd: " << cmd << endl;
			while (cmd.srac.len--) {
				wait();
				AXI_TC flit;
				flit.tid = cmd.sid;
				flit.tdest = cmd.did;
				flit.tuser = cmd.drac;
				flit.tdata = reg[cmd.srac.sel++].read();
				flit.tlast = cmd.srac.len == 0;
				cout << name() << "::ct_response ts: " << sc_time_stamp() << ", flit: " << flit << endl;
				m_ctl.write(flit);
			}
		}
	}

	void ms_reg_write()
	{
		if (reset.read() == RLEVEL) {
			for (unsigned i = 0; i < CR; i++) reg[i].write(0);
		} else {
			sc_bv<CD*CR> vreg = s_vreg.read();
			sc_bv<CR> vwe = s_vwe.read();
			sc_uint<CD> in = ain.read();
			sc_uint<CA> sel = asel.read();
			bool we = awe.read();
			for (unsigned i = 0; i < CR; i++) {
				if (vwe[i] == 1) {
					reg[i].write(vreg(CD*i+(CD-1),CD*i));
				} else if (i == sel && we) {
					reg[i].write(in);
				}
			}
		}
	}

	void mc_assign()
	{
		sc_bv<CD*CR> tmp;
		for (unsigned i = 0; i < CR; i++) tmp(CD*i+(CD-1),CD*i) = reg[i].read();
		m_vreg.write(tmp);
	}

	SC_CTOR(ctlreg)
		: clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		s_cmd("s_cmd"),
		m_cmd("m_cmd"),
		s_vreg("s_vreg"),
		s_vwe("s_vwe"),
		m_vreg("m_vreg"),
		i_cmd("i_cmd", 2),
		ain("ain"),
		asel("asel"),
		awe("awe")
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);
		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);
		SC_METHOD(ms_reg_write);
		sensitive << clk.pos();
		SC_METHOD(mc_assign);
		for (unsigned i = 0; i < CR; i++) sensitive << reg[i];
	}

};

#endif // _CTLREG_H
