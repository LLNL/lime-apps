
#ifndef _LSU_H
#define _LSU_H

#include <iomanip> // hex, setw
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

#if defined(__SYNTHESIS__)
#define TAG_T    TAG::ui_t
#define DMAC_T   DMAC::ui_t
#define DMAS_T   DMAS::ui_t
#else
#define TAG_T    TAG
#define DMAC_T   DMAC
#define DMAS_T   DMAS
#endif

#define TAG_CONCAT \
	(req,did)

class TAG { // DMA tag
public:
	typedef sc_uint<(1+CI)> ui_t;

	sc_uint<CI> did; // destination id
	sc_uint<1>  req; // request status

	TAG(ui_t ui = 0)
	{
		TAG_CONCAT = ui;
	}

	TAG & operator=(ui_t ui)
	{
		TAG_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return TAG_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const TAG &flit)
{
	return o << hex << flit.req << ':' << flit.did;
}

#define DMAC_CONCAT \
	(tag.req,tag.did,addr,last,size)

class DMAC { // DMA command
public:
	typedef sc_biguint<((1+CI)+MA+1+24)> ui_t;

	sc_uint<24> size; // size in bytes to transfer
	sc_uint<1>  last; // last block of packet
	sc_uint<MA> addr; // address to start transfer
	TAG         tag ; // tag

	DMAC(ui_t ui = 0)
	{
		DMAC_CONCAT = ui;
	}

	DMAC & operator=(ui_t ui)
	{
		DMAC_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return DMAC_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const DMAC &flit)
{
	return o << hex << flit.tag << ':'  << flit.addr << ':' << flit.last << ':' << flit.size;
}

#define DMAS_CONCAT \
	(okay,slverr,decerr,interr,tag.req,tag.did)

class DMAS { // DMA status
public:
	typedef sc_biguint<(1+1+1+1+(1+CI))> ui_t;

	TAG        tag   ; // tag
	sc_uint<1> interr; // internal error
	sc_uint<1> decerr; // decode error
	sc_uint<1> slverr; // slave error
	sc_uint<1> okay  ; // transfer completed

	DMAS(ui_t ui = 0)
	{
		DMAS_CONCAT = ui;
	}

	DMAS & operator=(ui_t ui)
	{
		DMAS_CONCAT = ui;
		return *this;
	}

	operator ui_t() const
	{
		return DMAS_CONCAT;
	}
};

inline std::ostream & operator<< (std::ostream &o, const DMAS &flit)
{
	return o << hex << flit.okay << ':'  << flit.slverr << ':'  << flit.decerr << ':' << flit.interr << ':' << flit.tag;
}

SC_MODULE(lsuctl) // LSU control
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <DMAS_T> s_dmas;
	sc_fifo_out<DMAC_T> m_dmac;

	sc_fifo<ACMD_T> c_acmd, c_arsp;
	// TODO: refactor into:
	// sc_signal<sc_uint<CD> > c_vcmd[CR], c_vrsp[CR];
	// sc_signal<bool> c_vwe[CR];
	sc_signal<sc_bv<CD*CR> > c_vcmd, c_vrsp;
	sc_signal<sc_bv<CR> > c_vwe;
	sc_signal<sc_uint<CI> > c_tid;

	ctlreg u_ctlreg;

#define _status(vv)  vv.range(8-1,0)
#define _command(vv) vv.range(CD*1+3-1,CD*1)
#define _reqstat(vv) vv[CD*1+7]
#define _address(vv) vv.range(CD*2+CD-1,CD*2)
#define _size(vv)    vv.range(CD*3+30-1,CD*3)
#define _inc(vv)     vv.range(CD*4+30-1,CD*4)
#define _rep(vv)     vv.range(CD*5+30-1,CD*5)
#define _index(vv)   vv.range(CD*4+30-1,CD*4)
#define _trans(vv)   vv.range(CD*5+30-1,CD*5)

	void ct_command()
	{
		ACMD acmd;
		sc_biguint<CD*CR> vcmd;
		DMAC dmac;
		while (true) {
			wait();
			acmd = c_acmd.read();
			c_tid = acmd.did;
			vcmd = c_vcmd.read();
#ifndef __SYNTHESIS__
			cout << name() << "::ct_command ts: " << sc_time_stamp() << ", acmd: " << acmd << endl;
#endif
			switch (_command(vcmd).to_uint()) {
			case LSU_nop   : break;
			case LSU_move  : {
				dmac.size = _size(vcmd);
				dmac.last = 1;
				dmac.addr = _address(vcmd);
				dmac.tag.did = acmd.sid;
				dmac.tag.req = _reqstat(vcmd);
				m_dmac.write(dmac);
				break; }
			case LSU_smove : {
				while (_rep(vcmd).to_uint()) {
					sc_uint<1> last = _rep(vcmd) == 1;
					dmac.size = _size(vcmd);
					dmac.last = last;
					dmac.addr = _address(vcmd);
					dmac.tag.did = acmd.sid;
					dmac.tag.req = _reqstat(vcmd) & last;
					m_dmac.write(dmac);
					_address(vcmd) = _address(vcmd) + _inc(vcmd);
					_rep(vcmd) = _rep(vcmd) - 1;
					wait();
				}
				break; }
			case LSU_index :
			case LSU_index2: {
				dmac.size = (_command(vcmd) == LSU_index) ? _size(vcmd) : _trans(vcmd);
				dmac.last = 1;
				dmac.addr = _index(vcmd) * _size(vcmd) + _address(vcmd);
				dmac.tag.did = acmd.sid;
				dmac.tag.req = _reqstat(vcmd);
				m_dmac.write(dmac);
				break; }
			case LSU_flush : break;
			}
		}
	}

	void ct_response()
	{
		DMAS dmas;
		sc_biguint<CD*CR> vcmd;
		sc_biguint<CD*CR> vrsp;
		while (true) {
			wait();
			c_vwe.write(0);
			dmas = s_dmas.read();
			vcmd = c_vcmd.read();
			vrsp = 0; _status(vrsp) = _status(vcmd) | dmas;
			c_vrsp.write(vrsp);
			c_vwe.write(1);
			if (dmas.tag.req == 1) {
				ACMD arsp;
				arsp.sid = c_tid.read();
				arsp.did = dmas.tag.did;
				arsp.srac.len = 1;
				arsp.srac.sel = 0;
				arsp.srac.wr  = 0;
				arsp.srac.go  = 1;
				arsp.drac = arsp.srac;
				c_arsp.write(arsp);
			}
		}
	}

	SC_CTOR(lsuctl)
		: clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		s_dmas("s_dmas"),
		m_dmac("m_dmac"),
		c_acmd("c_acmd", 1), // must be 1 to prevent register overwrite
		c_arsp("c_arsp", 1), // must be 1 to prevent register overwrite
		c_vcmd("c_vcmd"),
		c_vrsp("c_vrsp"),
		c_vwe("c_vwe"),
		u_ctlreg("u_ctlreg")
	{
		u_ctlreg.clk(clk);
		u_ctlreg.reset(reset);
		u_ctlreg.s_ctl(s_ctl);
		u_ctlreg.m_ctl(m_ctl);
		u_ctlreg.s_cmd(c_arsp);
		u_ctlreg.m_cmd(c_acmd);
		u_ctlreg.m_vreg(c_vcmd);
		u_ctlreg.s_vreg(c_vrsp);
		u_ctlreg.s_vwe(c_vwe);

		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(mm2s)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <DMAC_T> s_dmac;
	sc_fifo_out<DMAS_T> m_dmas;

	sc_fifo_out<AXI_TD_T> m_dat;

	sc_fifo_out<AXI_AR_T> m_mem_ar;
	sc_fifo_in <AXI_R_T > m_mem_r ;

	void ct_command()
	{
		DMAC dmac;
		while (true) {
			wait();
			dmac = s_dmac.read();
#ifndef __SYNTHESIS__
			cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
#endif
		}
	}

	SC_CTOR(mm2s)
		: clk("clk"),
		reset("reset"),
		s_dmac("s_dmac"),
		m_dmas("m_dmas"),
		m_dat("m_dat"),
		m_mem_ar("m_mem_ar"),
		m_mem_r ("m_mem_r" )
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(s2mm)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <DMAC_T> s_dmac;
	sc_fifo_out<DMAS_T> m_dmas;

	sc_fifo_in <AXI_TD_T> s_dat;

	sc_fifo_out<AXI_AW_T> m_mem_aw;
	sc_fifo_out<AXI_W_T > m_mem_w;
	sc_fifo_in <AXI_B_T > m_mem_b;

	void ct_command()
	{
		DMAC dmac;
		while (true) {
			wait();
			dmac = s_dmac.read();
#ifndef __SYNTHESIS__
			cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
#endif
		}
	}

	SC_CTOR(s2mm)
		: clk("clk"),
		reset("reset"),
		s_dmac("s_dmac"),
		m_dmas("m_dmas"),
		s_dat("s_dat"),
		m_mem_aw("m_mem_aw"),
		m_mem_w ("m_mem_w" ),
		m_mem_b ("m_mem_b" )
	{
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

	sc_fifo<AXI_TC_T> c_ctlri, c_ctlro;
	sc_fifo<AXI_TC_T> c_ctlwi, c_ctlwo;
	sc_fifo<DMAC_T> c_dmacr;
	sc_fifo<DMAS_T> c_dmasr;
	sc_fifo<DMAC_T> c_dmacw;
	sc_fifo<DMAS_T> c_dmasw;

	ctree<2> u_ctree;
	lsuctl u_lsur;
	lsuctl u_lsuw;
	mm2s u_mm2s;
	s2mm u_s2mm;

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
		c_ctlri("c_ctlri", 2),
		c_ctlro("c_ctlro", 2),
		c_ctlwi("c_ctlwi", 2),
		c_ctlwo("c_ctlwo", 2),
		c_dmacr("c_dmacr", 4),
		c_dmasr("c_dmasr", 4),
		c_dmacw("c_dmacw", 4),
		c_dmasw("c_dmasw", 4),
		u_ctree("u_ctree"),
		u_lsur("u_lsur"),
		u_lsuw("u_lsuw"),
		u_mm2s("u_mm2s"),
		u_s2mm("u_s2mm")
	{
		u_ctree.clk(clk);
		u_ctree.reset(reset);
		u_ctree.s_root(s_ctl);
		u_ctree.m_root(m_ctl);
		u_ctree.s_port[0](c_ctlro);
		u_ctree.m_port[0](c_ctlri);
		u_ctree.s_port[1](c_ctlwo);
		u_ctree.m_port[1](c_ctlwi);

		u_lsur.clk(clk);
		u_lsur.reset(reset);
		u_lsur.s_ctl(c_ctlri);
		u_lsur.m_ctl(c_ctlro);
		u_lsur.s_dmas(c_dmasr);
		u_lsur.m_dmac(c_dmacr);

		u_lsuw.clk(clk);
		u_lsuw.reset(reset);
		u_lsuw.s_ctl(c_ctlwi);
		u_lsuw.m_ctl(c_ctlwo);
		u_lsuw.s_dmas(c_dmasw);
		u_lsuw.m_dmac(c_dmacw);

		u_mm2s.clk(clk);
		u_mm2s.reset(reset);
		u_mm2s.s_dmac(c_dmacr);
		u_mm2s.m_dmas(c_dmasr);
		u_mm2s.m_dat(m_dat);
		u_mm2s.m_mem_ar(m_mem_ar);
		u_mm2s.m_mem_r(m_mem_r);

		u_s2mm.clk(clk);
		u_s2mm.reset(reset);
		u_s2mm.s_dmac(c_dmacw);
		u_s2mm.m_dmas(c_dmasw);
		u_s2mm.s_dat(s_dat);
		u_s2mm.m_mem_aw(m_mem_aw);
		u_s2mm.m_mem_w(m_mem_w);
		u_s2mm.m_mem_b(m_mem_b);
	}

};

#endif // _LSU_H
