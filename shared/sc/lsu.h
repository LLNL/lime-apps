
#ifndef _LSU_H
#define _LSU_H

#include <iomanip> // hex, setw
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"

#include "config.h" // TRACE
#include "port_strm.h"
#include "port_dma.h"
#include "ctlreg.h"

#define PEQ_FAC 3 // payload event queue factor
#define DMAC_DEPTH 4
#define OUTSTANDING_READ 16
#define OUTSTANDING_WRITE 16
// TODO: calculate from MD and max burst length
#define MAX_BLOCK_SIZE 128
#define ADDR_CALC_CYCLES 6 // 7 hangs HMCSim

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

// TODO: add DMA status flag to DMAC
// Use DMAC for now. Could be extended with more fields (byte enables, stream width).
#define TRANS DMAC
#if defined(__SYNTHESIS__)
#define TRANS_T TRANS::ui_t
#else
#define TRANS_T TRANS
#endif

struct extension: tlm::tlm_extension<extension>
{
	extension() : value(0) {}

	// Must override pure virtual clone method
	virtual tlm_extension_base* clone() const {
		extension* t = new extension;
		t->value = this->value;
		t->qdepth = this->qdepth;
		return t;
	}

	// Must override pure virtual copy_from method
	virtual void copy_from(tlm_extension_base const &ext) {
		value = static_cast<extension const &>(ext).value;
		qdepth = static_cast<extension const &>(ext).qdepth;
	}

	uint64 value;
	unsigned qdepth;
};

// Transaction payload memory manager
class tmanage : public tlm::tlm_mm_interface
{
public:
	typedef tlm::tlm_generic_payload payload_t;

	tmanage(int count = 16) : list(NULL)
	{
		for (int i = 0; i < count; i++) {
			payload_t *trans = new payload_t(this);
			unsigned char *data = new unsigned char[MAX_BLOCK_SIZE];
			trans->set_data_ptr(data);
			extension* ext = new extension;
			trans->set_extension(ext);
			free(trans);
		}
	}

	~tmanage()
	{
		while (list != NULL) {
			payload_t *trans = allocate();
			delete[] trans->get_data_ptr();
			extension* ext;
			trans->get_extension(ext);
			trans->clear_extension(ext);
			ext->free();
			delete trans;
		}
	}

	payload_t* allocate()
	{
		while (list == NULL) wait();
		payload_t *trans = list;
		list = reinterpret_cast<payload_t *>(list->get_address());
		return trans;
	}

	void free(payload_t *trans)
	{
		trans->set_address(reinterpret_cast<uint64>(list));
		list = trans;
	}

private:
	payload_t *list;
};

SC_MODULE(mm2s)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <DMAC_T> s_dmac;
	sc_fifo_out<DMAS_T> m_dmas;

	sc_fifo_out<AXI_TD_T> m_dat;

#ifndef __SYNTHESIS__
	tlm_utils::simple_initiator_socket<mm2s> m_mem_r;
	// peq simulates a FIFO for responses
	tlm_utils::peq_with_get<tmanage::payload_t> peq;
	unsigned peqcnt;
	tmanage tpool;
#endif

	sc_fifo<TRANS_T> c_trans;

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
				unsigned long addr = dmac.addr.to_ulong();
				unsigned size = dmac.size.to_uint(); // DMA size
				const char *unit = strstr(name(),"lsu"); // TODO: get id from dmac or template arg
				unsigned id = (unit != NULL) ? (unit[3]-'0'-1)*2+1 : -1;
				// cout << name() << "::ct_command id: " << id << endl;
				while (size) {
					wait(ADDR_CALC_CYCLES); // simulate address calculation overhead
					unsigned bsize = MAX_BLOCK_SIZE - (addr % MAX_BLOCK_SIZE); // aligned block size
					unsigned tsize = (size < bsize) ? size : bsize; // transaction size
					// cout << "id:" << id << " peqcnt:" << peqcnt << endl;
					if (peqcnt > 0) wait(peqcnt*PEQ_FAC); // responses backing up, throttle requests
#if defined(TRACE)
					extern FILE *tfp;
					fprintf(tfp, "1,R,0x%08lX,%u,%u,%llu\n", addr, tsize, id, sc_time_stamp().value()/800);
#endif

#if 1
					// Used for timing only
					tmanage::payload_t *tpay = tpool.allocate();
					tlm::tlm_phase phase = tlm::BEGIN_REQ;
					sc_time delay;
					tlm::tlm_sync_enum status;
					tpay->acquire();
					tpay->set_read();
#if defined(HMCSIM)
					unsigned long baddr = addr & ~0xF; // aligned begin address
					unsigned long eaddr = (addr+tsize+0xF) & ~0xF; // aligned end address
					tpay->set_address(baddr); // can only address on 16 byte boundary
					tpay->set_data_length(eaddr-baddr); // HMCSim seg faults if size < 16
#else
					tpay->set_address(addr);
					tpay->set_data_length(tsize);
#endif
					tpay->set_byte_enable_ptr(0);
					tpay->set_dmi_allowed(false);
					tpay->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
					tpay->set_streaming_width(id); // non-standard for HMCSim
					tpay->get_extension<extension>()->value = sc_time_stamp().value();
					tpay->get_extension<extension>()->qdepth = c_trans.num_available();
					// cout << name() << "::ct_command  ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay->get_address() << ", id: " << tpay->get_streaming_width() << endl;
#if defined(HMCSIM)
					if (sc_time_stamp().value() % 1600 != 0) wait(); // align clock for HMCSim
#endif
					status = m_mem_r->nb_transport_fw(*tpay, phase, delay);
					if (status != tlm::TLM_UPDATED)
						cout << " -- error: " << name() << "::ct_command ts: " << sc_time_stamp() << ", tpay addr: " << tpay->get_address() << endl;
#endif

					TRANS trans;
					trans.addr = addr;
					trans.size = tsize;
					trans.last = size == tsize && dmac.last.to_uint();
					trans.tag  = dmac.tag;
					// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", trans: " << trans << ", qd: " << c_trans.num_available() << endl;
					c_trans.write(trans);

					addr += tsize;
					size -= tsize;
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
			// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", proc_kind: " << sc_get_current_process_handle().proc_kind() << endl;
			TRANS trans = c_trans.read();

#ifndef __SYNTHESIS__
			tmanage::payload_t *tpay;
			while ((tpay = peq.get_next_transaction()) == NULL) wait();
			peqcnt--;
			// fprintf(tfp, "1,R,0x%08llX,%u,%u,%llu,%llu,%u\n", tpay->get_address(), tpay->get_data_length(), tpay->get_streaming_width(), tpay->get_extension<extension>()->value/800, (sc_time_stamp().value()-tpay->get_extension<extension>()->value)/1000, tpay->get_extension<extension>()->qdepth);
			// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay->get_address() << ", id: " << tpay->get_streaming_width() << endl;
			tpay->release();

			// Do direct reads
			typedef unsigned int word;
			word *addr = (word *)trans.addr.to_uint64(); // assumes the address is aligned to a word
			unsigned len = (trans.size.to_uint()+((DD+7)/8)-1) / ((DD+7)/8); // data stream flits
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
				flit.tlast = len == 0 && trans.last.to_uint();
				// if (strstr(name(),"lsu1") != NULL)
				// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", flit: " << flit << endl;
				m_dat.write(flit);
			}
#endif

			if (trans.last.to_uint()) {
				DMAS dmas;
				dmas.tag = trans.tag;
				dmas.interr = 0;
				dmas.decerr = 0;
				dmas.slverr = 0;
				dmas.okay = 1;
				m_dmas.write(dmas);
			}
		}
	}

#ifndef __SYNTHESIS__
	tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& tpay, tlm::tlm_phase& phase, sc_time& delay)
	{
		// cout << name() << "::nb_transpor ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay.get_address() << ", id: " << tpay.get_streaming_width() << endl;
		if (phase==tlm::BEGIN_RESP) {
			peq.notify(tpay, delay);
			peqcnt++;
#if TRACE == _TALL_
			extern FILE *tfp;
			unsigned id = tpay.get_streaming_width(); // non-standard for HMCSim
			fprintf(tfp, "1,LR,,,%u,%llu\n", id, (sc_time_stamp().value()+delay.value())/800);
#endif
		}
		return tlm::TLM_ACCEPTED;
	}
#endif

	SC_CTOR(mm2s) :
		peq("peq"),
		tpool(OUTSTANDING_READ+1),
		c_trans("c_trans", OUTSTANDING_READ)
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);

#ifndef __SYNTHESIS__
		m_mem_r.register_nb_transport_bw(this, &mm2s::nb_transport_bw);
#endif
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
	// peq simulates a FIFO for responses
	tlm_utils::peq_with_get<tmanage::payload_t> peq;
	unsigned peqcnt;
	tmanage tpool;
#endif

	sc_fifo<TRANS_T> c_trans;

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
				unsigned long addr = dmac.addr.to_ulong();
				unsigned size = dmac.size.to_uint(); // DMA size
				const char *unit = strstr(name(),"lsu"); // TODO: get id from dmac or template arg
				unsigned id = (unit != NULL) ? (unit[3]-'0'-1)*2+0 : -1;
				// cout << name() << "::ct_command id: " << id << endl;
				while (size) {
					wait(ADDR_CALC_CYCLES); // simulate address calculation overhead
					unsigned bsize = MAX_BLOCK_SIZE - (addr % MAX_BLOCK_SIZE); // aligned block size
					unsigned tsize = (size < bsize) ? size : bsize; // transaction size
					// cout << "id:" << id << " peqcnt:" << peqcnt << endl;
					if (peqcnt > 0) wait(peqcnt*PEQ_FAC); // responses backing up, throttle requests
#if defined(TRACE)
					extern FILE *tfp;
					fprintf(tfp, "1,W,0x%08lX,%u,%u,%llu\n", addr, tsize, id, sc_time_stamp().value()/800);
#endif

					// Do direct writes
					typedef unsigned int word;
					word *taddr = (word *)addr; // assumes the address is aligned to a word
					unsigned len = tsize; // remaining bytes
					AXI_TD flit;
					while (len) {
						wait();
						flit = s_dat.read();
						// if (strstr(name(),"lsu2") != NULL)
						// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", flit: " << flit << endl;
						for (int i = 0; i < (DD/8); i += sizeof(word)) {
							if (flit.tkeep(i+sizeof(word)-1,i) == ((1U << sizeof(word))-1)) {
								*taddr++ = flit.tdata((i+sizeof(word))*8-1,i*8).to_uint();
								len -= sizeof(word);
							}
						}
					}
#if TRACE == _TALL_
					extern FILE *tfp;
					fprintf(tfp, "1,FW,,,%u,%llu\n", id, sc_time_stamp().value()/800);
#endif

#if 1
					// Used for timing only
					tmanage::payload_t *tpay = tpool.allocate();
					tlm::tlm_phase phase = tlm::BEGIN_REQ;
					sc_time delay;
					tlm::tlm_sync_enum status;
					tpay->acquire();
					tpay->set_write();
#if defined(HMCSIM)
					unsigned long baddr = addr & ~0xF; // aligned begin address
					unsigned long eaddr = (addr+tsize+0xF) & ~0xF; // aligned end address
					tpay->set_address(baddr); // can only address on 16 byte boundary
					tpay->set_data_length(eaddr-baddr); // HMCSim seg faults if size < 16
#else
					tpay->set_address(addr);
					tpay->set_data_length(tsize);
#endif
					tpay->set_byte_enable_ptr(0);
					tpay->set_dmi_allowed(false);
					tpay->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
					tpay->set_streaming_width(id); // non-standard for HMCSim
					tpay->get_extension<extension>()->value = sc_time_stamp().value();
					tpay->get_extension<extension>()->qdepth = c_trans.num_available();
					// cout << name() << "::ct_command  ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay->get_address() << ", id: " << tpay->get_streaming_width() << endl;
#if defined(HMCSIM)
					if (sc_time_stamp().value() % 1600 != 0) wait(); // align clock for HMCSim
#endif
					status = m_mem_w->nb_transport_fw(*tpay, phase, delay);
					if (status != tlm::TLM_UPDATED)
						cout << " -- error: " << name() << "::ct_command ts: " << sc_time_stamp() << ", tpay addr: " << tpay->get_address() << endl;
#endif

					TRANS trans;
					trans.addr = addr;
					trans.size = tsize;
					trans.last = size == tsize && dmac.last.to_uint();
					trans.tag  = dmac.tag;
					// cout << name() << "::ct_command ts: " << sc_time_stamp() << ", trans: " << trans << ", qd: " << c_trans.num_available() << endl;
					c_trans.write(trans);

					addr += tsize;
					size -= tsize;
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
			// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", proc_kind: " << sc_get_current_process_handle().proc_kind() << endl;
			TRANS trans = c_trans.read();

#ifndef __SYNTHESIS__
			tmanage::payload_t *tpay;
			while ((tpay = peq.get_next_transaction()) == NULL) wait();
			peqcnt--;
			// fprintf(tfp, "1,W,0x%08llX,%u,%u,%llu,%llu,%u\n", tpay->get_address(), tpay->get_data_length(), tpay->get_streaming_width(), tpay->get_extension<extension>()->value/800, (sc_time_stamp().value()-tpay->get_extension<extension>()->value)/1000, tpay->get_extension<extension>()->qdepth);
			// cout << name() << "::ct_response ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay->get_address() << ", id: " << tpay->get_streaming_width() << endl;
			tpay->release();
#endif

			if (trans.last.to_uint()) {
				DMAS dmas;
				dmas.tag = trans.tag;
				dmas.interr = 0;
				dmas.decerr = 0;
				dmas.slverr = 0;
				dmas.okay = 1;
				m_dmas.write(dmas);
			}
		}
	}

#ifndef __SYNTHESIS__
	tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& tpay, tlm::tlm_phase& phase, sc_time& delay)
	{
		// cout << name() << "::nb_transpor ts: " << sc_time_stamp() << ", tpay addr: " << hex << tpay.get_address() << ", id: " << tpay.get_streaming_width() << endl;
		if (phase==tlm::BEGIN_RESP) {
			peq.notify(tpay, delay);
			peqcnt++;
#if TRACE == _TALL_
			unsigned id = tpay.get_streaming_width(); // non-standard for HMCSim
			extern FILE *tfp;
			fprintf(tfp, "1,B,,,%u,%llu\n", id, (sc_time_stamp().value()+delay.value())/800);
#endif
		}
		return tlm::TLM_ACCEPTED;
	}
#endif

	SC_CTOR(s2mm) :
		peq("peq"),
		tpool(OUTSTANDING_WRITE+1),
		c_trans("c_trans", OUTSTANDING_WRITE)
	{
		SC_CTHREAD(ct_command, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_response, clk.pos());
		reset_signal_is(reset, RLEVEL);

#ifndef __SYNTHESIS__
		m_mem_w.register_nb_transport_bw(this, &s2mm::nb_transport_bw);
#endif
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
