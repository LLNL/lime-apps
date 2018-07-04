
#ifndef _LSU_H
#define _LSU_H

#include <iomanip> // hex, setw
#include "systemc.h"

#include "ports.h"
#include "ctlreg.h"

#define DMAC_DEPTH 4
#define OUTSTANDING_READ 16
#define OUTSTANDING_WRITE 16

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
#define TAG_T  TAG::ui_t
#define DMAC_T DMAC::ui_t
#define DMAS_T DMAS::ui_t
#else
#define TAG_T  TAG
#define DMAC_T DMAC
#define DMAS_T DMAS
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
	sc_signal<sc_uint<CD> > c_vcmd[CR], c_vrsp[CR];
	sc_signal<bool> c_vwe[CR];
	sc_signal<sc_uint<CI> > c_tid;

	ctlreg u_ctlreg;

#define _status(reg)  reg[0](8-1,0)
#define _command(reg) reg[1](3-1,0)
#define _reqstat(reg) reg[1][7]
#define _address(reg) reg[2]
#define _size(reg)    reg[3](30-1,0)
#define _inc(reg)     reg[4](30-1,0)
#define _rep(reg)     reg[5](30-1,0)
#define _index(reg)   reg[4](30-1,0)
#define _trans(reg)   reg[5](30-1,0)

	void ct_command()
	{
		ACMD acmd;
		sc_uint<CD> vcmd[CR];
		DMAC dmac;
		while (true) {
			wait();
			acmd = c_acmd.read();
			for (int i = 0; i < CR; i++) vcmd[i] = c_vcmd[i].read();
			c_tid.write(acmd.did);
#ifndef __SYNTHESIS__
			// if (strstr(name(),"u_lsu2.u_lsur") != NULL)
			// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", acmd: " << acmd << endl;
			// for (int i = 0; i < CR; i++) cout << "reg:" << i << ':' << vcmd[i] << endl;
#endif
			switch (_command(vcmd).to_uint()) {
			case LSU_nop   : break;
			case LSU_move  : {
				dmac.size = _size(vcmd);
				dmac.last = 1;
				dmac.addr = _address(vcmd);
				dmac.tag.did = acmd.sid;
				dmac.tag.req = _reqstat(vcmd);
				// cout << name() << "::ct_command,LSU_move ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
				m_dmac.write(dmac);
				break; }
			case LSU_smove : {
				// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", _rep(vcmd): " << _rep(vcmd) << endl;
				while (_rep(vcmd).to_uint()) {
					sc_uint<1> last = _rep(vcmd) == 1;
					dmac.size = _size(vcmd);
					dmac.last = last;
					dmac.addr = _address(vcmd);
					dmac.tag.did = acmd.sid;
					dmac.tag.req = _reqstat(vcmd) & last;
					// cout << name() << "::ct_command,LSU_smove ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
					m_dmac.write(dmac);
					_address(vcmd) = _address(vcmd) + _inc(vcmd);
					_rep(vcmd) = _rep(vcmd) - 1;
					wait();
				}
				break; }
			case LSU_index :
			case LSU_index2: {
				// if (strstr(name(),"u_lsu2.u_lsur") != NULL)
					// cout << _index(vcmd) << endl;
				// cout << name() << "::ct_command,LSU_index(2) ts: " << sc_time_stamp() << hex << ", _index(vcmd): " << _index(vcmd) << endl;
				// ", _address(vcmd): " << _address(vcmd) << endl;
				dmac.size = (_command(vcmd) == LSU_index) ? _size(vcmd) : _trans(vcmd);
				dmac.last = 1;
				dmac.addr = _index(vcmd) * _size(vcmd) + _address(vcmd);
				dmac.tag.did = acmd.sid;
				dmac.tag.req = _reqstat(vcmd);
				// if (strstr(name(),"u_lsu2.u_lsur") != NULL) {
				// cout << name() << "::ct_command,LSU_index(2) ts: " << sc_time_stamp() << ", dmac: " << dmac << ", _index(vcmd): " << _index(vcmd).to_uint() << endl;
				// uint64 *atmp = (uint64*)dmac.addr.to_uint();
				// for (int i = 0; i < 4; i++) cout << hex << *atmp++ << ' ' << *atmp++ << endl;
				// }
				m_dmac.write(dmac);
				break; }
			case LSU_flush : break;
			}
		}
	}

	void ct_response()
	{
		DMAS dmas;
		for (int i = 0; i < CR; i++) {
			c_vrsp[i].write(0);
			c_vwe[i].write(false);
		}
		while (true) {
			wait();
			dmas = s_dmas.read();
			// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", dmas: " << dmas << endl;
			while (c_arsp.num_free() < 1) wait();
			c_vrsp[0].write(c_vcmd[0].read() | dmas);
			c_vwe[0].write(true);
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

#undef _status
#undef _command
#undef _reqstat
#undef _address
#undef _size
#undef _inc
#undef _rep
#undef _index
#undef _trans

	SC_CTOR(lsuctl) :
		c_acmd("c_acmd", 1), // must be 1 to prevent register overwrite
		c_arsp("c_arsp", 1), // must be 1 to prevent register overwrite
		u_ctlreg("u_ctlreg")
	{
		u_ctlreg.clk(clk);
		u_ctlreg.reset(reset);
		u_ctlreg.s_ctl(s_ctl);
		u_ctlreg.m_ctl(m_ctl);
		u_ctlreg.s_acmd(c_arsp);
		u_ctlreg.m_acmd(c_acmd);
		for (int i = 0; i < CR; i++) {
			u_ctlreg.m_reg[i](c_vcmd[i]);
			u_ctlreg.s_reg[i](c_vrsp[i]);
			u_ctlreg.s_we[i](c_vwe[i]);
		}

		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(mm2s) // AXI version
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <DMAC_T> s_dmac;
	sc_fifo_out<DMAS_T> m_dmas;

	sc_fifo_out<AXI_TD_T> m_dat;

	sc_fifo_out<AXI_AR_T> m_mem_ar;
	sc_fifo_in <AXI_R_T > m_mem_r ;

	sc_fifo_in <DMAC_T> c_dmac;

	void ct_command()
	{
		DMAC dmac;
		while (true) {
			wait();
			dmac = s_dmac.read();
			c_dmac.write(dmac);
#ifndef __SYNTHESIS__
			// if (strstr(name(),"lsu2") != NULL)
			// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
#endif
			// AXI unaligned transfer? Take floor of start, ceil of end address?
			AXI_AR mem_ar;
			mem_ar.arid    = 0; // TODO: specify ID with template parameter
			mem_ar.araddr  = dmac.addr & ~(MD/8-1);
			mem_ar.arlen   = ; // minus one
			mem_ar.arsize  = 3; // log2rp(MD/8)
			mem_ar.arburst = 1; // increment
			mem_ar.arlock  = 0;
			mem_ar.arcache = 0;
			mem_ar.arprot  = 2;
			while (len) {
				m_mem_ar.write(mem_ar);
			}
		}
	}

#if 0
			{
				typedef unsigned int word;
				word *addr = (word *)dmac.addr.to_ulong();
				unsigned len = (dmac.size.to_uint()+((DD+7)/8)-1) / ((DD+7)/8);
				AXI_TD flit;
				flit.tid = 0;
				flit.tdest = 0;
				flit.tuser = 0;
				while (len--) {
					wait();
					for (int i = 0; i < (DD/8); i += sizeof(word)) {
						flit.tdata((i+sizeof(word))*8-1,i*8) = *addr++;
						flit.tkeep(i+sizeof(word)-1,i) = ((1U << sizeof(word))-1);
					}
					flit.tlast = len == 0 && dmac.last.to_uint();
					// if (strstr(name(),"lsu1") != NULL)
					// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", flit: " << flit << endl;
					m_dat.write(flit);
				}
			}
#endif

	void ct_response()
	{
		DMAC dmac;
		DMAS dmas;
		while (true) {
			wait();
			dmac = s_dmac.read();
			do {
			} while ();
			dmas.tag = dmac.tag;
			dmas.interr = 0;
			dmas.decerr = 0;
			dmas.slverr = 0;
			dmas.okay = 1;
			m_dmas.write(dmas);
		}
	}

	SC_CTOR(mm2s) :
		c_tag("c_tag", OUTSTANDING_READ)
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
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
		DMAS dmas;
		while (true) {
			wait();
			dmac = s_dmac.read();
#ifndef __SYNTHESIS__
			// if (strstr(name(),"lsu2") != NULL)
			// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
#endif
			{
				typedef unsigned int word;
				word *addr = (word *)dmac.addr.to_ulong();
				unsigned len = (dmac.size.to_uint()+((DD+7)/8)-1) / ((DD+7)/8);
				AXI_TD flit;
				while (len--) {
					wait();
					flit = s_dat.read();
					// if (strstr(name(),"lsu2") != NULL)
					// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", flit: " << flit << endl;
					for (int i = 0; i < (DD/8); i += sizeof(word)) {
						if (flit.tkeep(i+sizeof(word)-1,i) == ((1U << sizeof(word))-1)) {
							*addr++ = flit.tdata((i+sizeof(word))*8-1,i*8).to_uint();
						}
					}
				}
			}
			dmas.tag = dmac.tag;
			dmas.interr = 0;
			dmas.decerr = 0;
			dmas.slverr = 0;
			dmas.okay = 1;
			m_dmas.write(dmas);
		}
	}

	SC_CTOR(s2mm)
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

	SC_CTOR(lsu) :
		c_ctlri("c_ctlri", 2),
		c_ctlro("c_ctlro", 2),
		c_ctlwi("c_ctlwi", 2),
		c_ctlwo("c_ctlwo", 2),
		c_dmacr("c_dmacr", DMAC_DEPTH),
		c_dmasr("c_dmasr", 2),
		c_dmacw("c_dmacw", DMAC_DEPTH),
		c_dmasw("c_dmasw", 2),
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
