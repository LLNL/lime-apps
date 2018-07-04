
#ifndef _LSU_H
#define _LSU_H

#include <iomanip> // hex, setw
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "port_strm.h"
#include "port_dma.h"
#include "ctlreg.h"

#define DMAC_DEPTH 4
#define OUTSTANDING_READ 16
#define OUTSTANDING_WRITE 16
// TODO: calculate from MD and max burst length
#define MAX_BLOCK_SIZE 64

// FIXME: duplicated in aport.h, merge?
enum {
	LSU_nop,
	LSU_move,
	LSU_smove,
	LSU_index,
	LSU_index2,
	LSU_flush = 7,
};

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

#ifndef __SYNTHESIS__
struct TRANS {
	uint64        tsv;
	unsigned long addr;
	unsigned long size;
	unsigned char tag;
	bool last;
};

inline std::ostream & operator<< (std::ostream &o, const TRANS &flit)
{
	return o << hex << flit.last << ':' << flit.tag << ':' << flit.size << ':' << flit.addr;
}
#endif

SC_MODULE(mm2s)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <DMAC_T> s_dmac;
	sc_fifo_out<DMAS_T> m_dmas;

	sc_fifo_out<AXI_TD_T> m_dat;

#ifndef __SYNTHESIS__
	tlm_utils::simple_initiator_socket<mm2s> m_mem_r;
	sc_fifo<TRANS> c_trans;
#endif

	void ct_command()
	{
		DMAC dmac;
		while (true) {
			wait();
			dmac = s_dmac.read();
#ifndef __SYNTHESIS__
			// if (strstr(name(),"lsu2") != NULL)
			// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
			{
				unsigned long addr = dmac.addr.to_ulong(); // assumes the address is aligned
				unsigned size = dmac.size.to_uint();
				unsigned bsize = MD/8*16; // burst size in bytes
				TRANS trans;
				trans.tag = TAG::ui_t(dmac.tag).to_uint();
				while (size) {
					wait(6); // simulate address calculation overhead
					trans.addr = addr;
					trans.size = (size < bsize) ? size : bsize;
					// if (strstr(name(),"lsu1") != NULL)
					// printf("1,R,0x%08X,%u,%u,%u\n", (unsigned)trans.addr, (unsigned)trans.size, (strstr(name(),"lsu1")?2:4), (unsigned)(sc_time_stamp().value()/800));
					size -= trans.size;
					addr += trans.size;
					trans.last = size == 0 && dmac.last.to_uint();
					trans.tsv = sc_time_stamp().value();
					c_trans.write(trans);
				}
			}
#endif
		}
	}

	void ct_response()
	{
		DMAS dmas;
		while (true) {
			wait();
#ifndef __SYNTHESIS__
			{
				TRANS trans = c_trans.read();
				// unsigned mflits = (trans.size+(MD/8)-1) / (MD/8); // memory mapped flits
				uint64 curtsv = sc_time_stamp().value();
				// uint64 endtsv = trans.tsv + mflits*800 + 61*1000; // memory read latency 61 ns
				uint64 endtsv = trans.tsv /*+ mflits*800*/ + 100*1000; // memory read latency 100 ns, trying to simulate LiME
				if (curtsv < endtsv) {wait((endtsv-curtsv)/800); /*cout << name() << "::rpticks: " << (endtsv-curtsv)/800 << endl;*/}
				// else {cout << name() << "::rnticks: " << (curtsv-endtsv)/800 << endl;}
				typedef unsigned int word;
				word *addr = (word *)trans.addr; // assumes the address is aligned
				unsigned len = (trans.size+((DD+7)/8)-1) / ((DD+7)/8); // data stream flits
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
					flit.tlast = len == 0 && trans.last;
					// if (strstr(name(),"lsu1") != NULL)
					// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", flit: " << flit << endl;
					m_dat.write(flit);
				}
				dmas.tag = trans.tag;
			}
#endif
			dmas.interr = 0;
			dmas.decerr = 0;
			dmas.slverr = 0;
			dmas.okay = 1;
			m_dmas.write(dmas);
		}
	}

	SC_CTOR(mm2s)
#ifndef __SYNTHESIS__
		: c_trans("c_trans", OUTSTANDING_READ-1)
#endif
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

#ifndef __SYNTHESIS__
	tlm_utils::simple_initiator_socket<s2mm> m_mem_w;
	sc_fifo<TRANS> c_trans;
#endif

	void ct_command()
	{
		DMAC dmac;
		while (true) {
			wait();
			dmac = s_dmac.read();
#ifndef __SYNTHESIS__
			// if (strstr(name(),"lsu2") != NULL)
			// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", dmac: " << dmac << endl;
			{
				unsigned long addr = dmac.addr.to_ulong(); // assumes the address is aligned
				unsigned size = dmac.size.to_uint();
				unsigned bsize = MD/8*16; // burst size in bytes
				TRANS trans;
				trans.tag = TAG::ui_t(dmac.tag).to_uint();
				while (size) {
					wait(6); // simulate address calculation overhead
					trans.addr = addr;
					trans.size = (size < bsize) ? size : bsize;
					// printf("1,W,0x%08X,%u,%u,%u\n", (unsigned)trans.addr, (unsigned)trans.size, (strstr(name(),"lsu1")?3:5), (unsigned)(sc_time_stamp().value()/800));
					size -= trans.size;
					addr += trans.size;
					trans.last = size == 0 && dmac.last.to_uint();
					trans.tsv = sc_time_stamp().value();
					c_trans.write(trans);
				}
			}
#endif
		}
	}

	void ct_response()
	{
		DMAS dmas;
		while (true) {
			wait();
#ifndef __SYNTHESIS__
			{
				TRANS trans = c_trans.read();
				typedef unsigned int word;
				word *addr = (word *)trans.addr;
				unsigned len = (trans.size+((DD+7)/8)-1) / ((DD+7)/8);
				AXI_TD flit;
				while (len--) {
					wait();
					flit = s_dat.read();
					// if (strstr(name(),"lsu2") != NULL)
					// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", flit: " << flit << endl;
					for (int i = 0; i < (DD/8); i += sizeof(word)) {
						if (flit.tkeep(i+sizeof(word)-1,i) == ((1U << sizeof(word))-1)) {
							*addr++ = flit.tdata((i+sizeof(word))*8-1,i*8).to_uint();
						}
					}
				}
				unsigned mflits = (trans.size+(MD/8)-1) / (MD/8); // memory mapped flits
				// uint64 curtsv = sc_time_stamp().value();
				// uint64 endtsv = trans.tsv + mflits*800 + 82*1000; // memory write latency 82 ns
				// uint64 endtsv = trans.tsv + mflits*800 + 10*1000; // memory write latency 10 ns, FIXME: hack for SRAM
				// if (curtsv < endtsv) wait((endtsv-curtsv)/800); else cout << "wnticks: " << (curtsv-endtsv)/800 << endl;
				wait(mflits + 10*1000/800);
				dmas.tag = trans.tag;
			}
#endif
			dmas.interr = 0;
			dmas.decerr = 0;
			dmas.slverr = 0;
			dmas.okay = 1;
			m_dmas.write(dmas);
		}
	}

	SC_CTOR(s2mm)
#ifndef __SYNTHESIS__
		: c_trans("c_trans", OUTSTANDING_WRITE-1)
#endif
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(lsu)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <AXI_TD_T> s_dat;
	sc_fifo_out<AXI_TD_T> m_dat;

	tlm::tlm_initiator_socket<> m_mem_w;
	tlm::tlm_initiator_socket<> m_mem_r;

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
		u_mm2s.m_mem_r(m_mem_r);

		u_s2mm.clk(clk);
		u_s2mm.reset(reset);
		u_s2mm.s_dmac(c_dmacw);
		u_s2mm.m_dmas(c_dmasw);
		u_s2mm.s_dat(s_dat);
		u_s2mm.m_mem_w(m_mem_w);
	}

};

#endif // _LSU_H
