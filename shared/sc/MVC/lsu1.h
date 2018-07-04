
#ifndef _LSU_H
#define _LSU_H

#include "systemc.h"

#include "ports.h"
#include "ctlreg.h"

// FIXME: duplicated in aport.h, merge?
enum {
	LSU_nop,
	LSU_move,
	LSU_smove,
	LSU_index,
	LSU_index2,
	LSU_flush = 7,
};

#define status  vc.range(8-1,0)
#define command vc.range(CD*1+3-1,CD*1)
#define reqstat vc[CD*1+7]
#define address vc.range(CD*2+CD-1,CD*2)
#define size    vc.range(CD*3+30-1,CD*3)
#define inc     vc.range(CD*4+30-1,CD*4)
#define rep     vc.range(CD*5+30-1,CD*5)
#define index   vc.range(CD*4+30-1,CD*4)
#define trans   vc.range(CD*5+30-1,CD*5)


SC_MODULE(lsur)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_out<AXI_TD_T> m_dat;

	sc_fifo_out<AXI_AR_T> m_mem_ar;
	sc_fifo_in <AXI_R_T > m_mem_r ;

	sc_fifo<ACMD_T> acmd, arsp;
	// sc_fifo<ACMD_T> aout;
	sc_signal<sc_bv<CD*CR> > vcmd, vrsp;
	sc_signal<sc_bv<CR> > vwe;

	ctlreg u_ctlreg;

	// hack this for now or make a data mover engine?

	void ct_command()
	{
		ACMD cmd;
		sc_biguint<CD*CR> vc;
		while (true) {
			wait();
			cmd = acmd.read();
			vc = vcmd.read();
#ifndef __SYNTHESIS__
			cout << name() << "::ct_command ts: " << sc_time_stamp() << ", cmd: " << cmd << endl;
#endif
			switch (command.to_int()) {
			case LSU_nop   : break;
			case LSU_move  : {
				unsigned long *addr = (unsigned long *)address.to_ulong();
				unsigned len = size.to_uint() / sizeof(unsigned long);
				sc_biguint<DD> tdata;
				while (len--) {
					for (int i = 0; i < DD; i += sizeof(unsigned long)*8) {
						tdata(i+sizeof(unsigned long)*8-1,i) = *addr++;
					}
					// write flit
				}
				break; }
			case LSU_smove : break;
			case LSU_index : break;
			case LSU_index2: break;
			case LSU_flush : break;
			}
		}
	}

#if 0
	void ct_response()
	{
		while (true) {
			wait();
			if (vcmd.read()[CD*1+7] == 1) {
			// if (reqstat == 1) {
				ACMD rsp;
				rsp.sid = cmd.did;
				rsp.did = cmd.sid;
				rsp.srac.len = 1;
				rsp.srac.sel = 0;
				rsp.srac.wr  = 0;
				rsp.srac.go  = 1;
				rsp.drac = cmd.srac;
				arsp.write(rsp);
			}
		}
	}
#endif

	SC_CTOR(lsur)
		: clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		m_dat("m_dat"),
		m_mem_ar("m_mem_ar"),
		m_mem_r ("m_mem_r" ),
		acmd("acmd", 1),
		arsp("arsp", 1),
		// aout("aout", MAX_OUTSTANDING), // max LSU commands or AXI transactions?
		vcmd("vcmd"),
		vrsp("vrsp"),
		vwe("vwe"),
		u_ctlreg("u_ctlreg")
	{
		u_ctlreg.clk(clk);
		u_ctlreg.reset(reset);
		u_ctlreg.s_ctl(s_ctl);
		u_ctlreg.m_ctl(m_ctl);
		u_ctlreg.s_cmd(arsp);
		u_ctlreg.m_cmd(acmd);
		u_ctlreg.m_vreg(vcmd);
		u_ctlreg.s_vreg(vrsp);
		u_ctlreg.s_vwe(vwe);

		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		// SC_CTHREAD(ct_response, clk.pos());
		// reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(lsuw)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <AXI_TD_T> s_dat;

	sc_fifo_out<AXI_AW_T> m_mem_aw;
	sc_fifo_out<AXI_W_T > m_mem_w;
	sc_fifo_in <AXI_B_T > m_mem_b;

	sc_fifo<ACMD_T> acmd, arsp;
	sc_signal<sc_bv<CD*CR> > vcmd, vrsp;
	sc_signal<sc_bv<CR> > vwe;

	ctlreg u_ctlreg;

	void ct_command()
	{
		ACMD cmd;
		while (true) {
			wait();
			cmd = acmd.read();
#ifndef __SYNTHESIS__
			cout << name() << "::ct_command ts: " << sc_time_stamp() << ", cmd: " << cmd << endl;
#endif
		}
	}

	SC_CTOR(lsuw)
		: clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		s_dat("s_dat"),
		m_mem_aw("m_mem_aw"),
		m_mem_w ("m_mem_w" ),
		m_mem_b ("m_mem_b" ),
		acmd("acmd", 2),
		arsp("arsp"),
		vcmd("vcmd"),
		vrsp("vrsp"),
		vwe("vwe"),
		u_ctlreg("u_ctlreg")
	{
		u_ctlreg.clk(clk);
		u_ctlreg.reset(reset);
		u_ctlreg.s_ctl(s_ctl);
		u_ctlreg.m_ctl(m_ctl);
		u_ctlreg.s_cmd(arsp);
		u_ctlreg.m_cmd(acmd);
		u_ctlreg.m_vreg(vcmd);
		u_ctlreg.s_vreg(vrsp);
		u_ctlreg.s_vwe(vwe);

		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

#if 0
	void proc_sequ()
	{
		AXI_TC ctmp_t;
		AXI_TD dtmp_t;
		AXI_AR mtmp_ar;
		AXI_R  mtmp_r;
		wait();
		while (true) {
//#pragma HLS PIPELINE II=1 //enable_flush
			ctmp_t = s_ctl.read();
			mtmp_ar.araddr = ctmp_t.tdata;
			m_mem_ar.write(mtmp_ar);
#ifndef __SYNTHESIS__
			cout << "lsu: ts: " << sc_time_stamp() << ", mtmp_ar.araddr: " << mtmp_ar.araddr << endl;
#endif
			wait();
#if 1
			mtmp_r = m_mem_r.read();
			dtmp_t.tdata = mtmp_r.rdata;
			m_dat.write(dtmp_t);
			m_ctl.write(0);
#ifndef __SYNTHESIS__
			cout << "lsu: ts: " << sc_time_stamp() << ", dtmp_t.tdata: " << dtmp_t.tdata << endl;
#endif
//			wait(); // this can add an extra cycle, why?
#endif
		}
	}
#endif

SC_MODULE(lsu)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <AXI_TD_T> s_dat;
	sc_fifo_out<AXI_TD_T> m_dat;

	sc_fifo_out<AXI_AW_T> m_mem_aw;
	sc_fifo_out<AXI_W_T > m_mem_w ;
	sc_fifo_in <AXI_B_T > m_mem_b ;
	sc_fifo_out<AXI_AR_T> m_mem_ar;
	sc_fifo_in <AXI_R_T > m_mem_r ;

	sc_fifo<AXI_TC_T> i_ctlw, o_ctlw;
	sc_fifo<AXI_TC_T> i_ctlr, o_ctlr;

	ctree<2> u_ctree;
	lsuw u_lsuw;
	lsur u_lsur;

	SC_CTOR(lsu)
		: clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		s_dat("s_dat"),
		m_dat("m_dat"),
		m_mem_aw("m_mem_aw"),
		m_mem_w ("m_mem_w" ),
		m_mem_b ("m_mem_b" ),
		m_mem_ar("m_mem_ar"),
		m_mem_r ("m_mem_r" ),
		i_ctlw("i_ctlw", 2),
		o_ctlw("o_ctlw", 2),
		i_ctlr("i_ctlr", 2),
		o_ctlr("o_ctlr", 2),
		u_ctree("u_ctree"),
		u_lsuw("u_lsuw"),
		u_lsur("u_lsur")
	{
		u_ctree.clk(clk);
		u_ctree.reset(reset);
		u_ctree.s_root(s_ctl);
		u_ctree.m_root(m_ctl);
		u_ctree.s_port[0](o_ctlr);
		u_ctree.m_port[0](i_ctlr);
		u_ctree.s_port[1](o_ctlw);
		u_ctree.m_port[1](i_ctlw);

		u_lsur.clk(clk);
		u_lsur.reset(reset);
		u_lsur.s_ctl(i_ctlr);
		u_lsur.m_ctl(o_ctlr);
		u_lsur.m_dat(m_dat);
		u_lsur.m_mem_ar(m_mem_ar);
		u_lsur.m_mem_r(m_mem_r);

		u_lsuw.clk(clk);
		u_lsuw.reset(reset);
		u_lsuw.s_ctl(i_ctlw);
		u_lsuw.m_ctl(o_ctlw);
		u_lsuw.s_dat(s_dat);
		u_lsuw.m_mem_aw(m_mem_aw);
		u_lsuw.m_mem_w(m_mem_w);
		u_lsuw.m_mem_b(m_mem_b);
	}

};

#endif // _LSU_H
