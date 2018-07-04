
#ifndef _HSU_H
#define _HSU_H

#include <iomanip> // hex, setw
#include "systemc.h"

#include "port_strm.h"
#include "ctlreg.h"
// #include "short.h" // for debug

#define KEYLW 8
#define KEYW 128
#define TAPW 256
#define NUMW (TAPW/4)
#define STAGES 12
#define C_INIT 0xDEADBEEFDEADBEEFULL

typedef sc_biguint<NUMW> num_t;


SC_MODULE(short_hash)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <sc_biguint<KEYLW+KEYW> > s_dat;
	sc_fifo_in <sc_biguint<TAPW> > s_tap;
	sc_fifo_out<sc_biguint<TAPW> > m_tap;

	sc_signal<num_t> a[STAGES];
	sc_signal<num_t> b[STAGES];
	sc_signal<num_t> c[STAGES];
	sc_signal<num_t> d[STAGES];
	sc_signal<sc_bv<STAGES> > v; // valid bit for each stage

	inline void emix(
		sc_signal<num_t> (&y)[STAGES],
		sc_signal<num_t> (&x)[STAGES],
		sc_signal<num_t> (&u)[STAGES],
		sc_signal<num_t> (&t)[STAGES],
		const unsigned ro, // rotate
		const unsigned ns) // next stage
	{
		num_t xt = x[ns-1].read();
		num_t xr = sc_bv<NUMW>(xt).lrotate(ro);
		t[ns].write(t[ns-1].read());
		u[ns].write(u[ns-1].read());
		x[ns].write(xr);
		y[ns].write((y[ns-1].read() ^ xt) + xr);
	}

	void ct_mix()
	{
		num_t len;
		num_t data[2];
		num_t tapi[4];
		sc_biguint<TAPW> tapo;
		while (true) {
			wait();
			if (s_dat.num_available() && s_tap.num_available()) {
				(len(KEYLW-1,0),data[1],data[0]) = s_dat.read();
				(tapi[3],tapi[2],tapi[1],tapi[0]) = s_tap.read();
				v.write((v.read() << 1) | 1);
			} else v.write(v.read() << 1);
			// cout << name() << "::ct_mix ts: " << sc_time_stamp() << ", data[0]: " << hex << data[0] << endl;
			// cout << name() << "::ct_mix ts: " << sc_time_stamp() << ", tapi: " << hex << tapi[3] << ',' << tapi[2] << ',' << tapi[1] << ',' << tapi[0] << endl;
			a[0].write(tapi[0] ^  len);
			b[0].write(tapi[1] ^ ~len);
			c[0].write(tapi[2] + data[0]);
			d[0].write(tapi[3] + data[1]);

			emix(d,c,b,a,15, 1);
			emix(a,d,c,b,52, 2);
			emix(b,a,d,c,26, 3);

			emix(c,b,a,d,51, 4);
			emix(d,c,b,a,28, 5);
			emix(a,d,c,b, 9, 6);
			emix(b,a,d,c,47, 7);

			emix(c,b,a,d,54, 8);
			emix(d,c,b,a,32, 9);
			emix(a,d,c,b,25,10);
			emix(b,a,d,c,63,11);

			tapo = (d[11],c[11],b[11],a[11]);
			if (v.read()[11].to_bool()) m_tap.write(tapo);
		}
	}

	SC_CTOR(short_hash)
	{
		SC_CTHREAD(ct_mix, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

SC_MODULE(hsu) // Hash Unit
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	sc_fifo_in <AXI_TD_T> s_dat1;
	sc_fifo_out<AXI_TD_T> m_dat1;
	sc_fifo_in <AXI_TD_T> s_dat2;
	sc_fifo_out<AXI_TD_T> m_dat2;

	sc_fifo<ACMD_T> c_acmd, c_arsp;
	sc_signal<sc_uint<CD> > c_vcmd[CR], c_vrsp[CR];
	sc_signal<bool> c_vwe[CR];

	sc_fifo<sc_biguint<KEYLW+KEYW> > c_key;
	sc_fifo<sc_biguint<TAPW> > c_tapi;
	sc_fifo<sc_biguint<TAPW> > c_tapo;

	ctlreg u_ctlreg;
	short_hash u_hash;

#define _status(reg)  reg[0](4,0)
#define _tlen(reg)    (reg[2],reg[1])
#define _keylen(reg)  reg[3](7,0)
#define _hashlen(reg) reg[3](15,8)
#define _tid(reg)     reg[3](19,16)
#define _tdest(reg)   reg[3](23,20)
#define _seed(reg)    reg[3](27,24)
#define _seldo(reg)   reg[3][28]
#define _seldi(reg)   reg[3][29]
#define _data(reg)    (reg[5],reg[4])
#define hreg 6
#define _hash(reg)    (reg[hreg+1],reg[hreg])

#if 0
	void ms_status()
	{
		if (reset.read() == RLEVEL) {
			for (int i = 0; i < CR; i++) c_vrsp[i].write(0);
		} else {
		}
	}
#endif

	void ct_key_in()
	{
		ACMD acmd;
		sc_uint<CD> vcmd[CR];
		sc_biguint<KEYW> key;
		while (true) {
			wait();
			for (int i = 0; i < CR; i++) vcmd[i] = c_vcmd[i].read();
			if (_seldi(vcmd) && s_dat1.num_available()) {
				key = s_dat1.read().tdata;
			} else if (!_seldi(vcmd) && c_acmd.num_available()) {
				acmd = c_acmd.read();
				key = _data(vcmd);
			} else continue;
#ifndef __SYNTHESIS__
			// cout << name() << "::ct_key_in ts: " << sc_time_stamp() << ", key: " << hex << key << endl;
			// for (int i = 0; i < CR; i++) cout << "reg:" << i << ':' << vcmd[i] << endl;
#endif
// {
// uint64 ktmp = key.to_uint64();
// int ltmp = _keylen(vcmd).to_int();
// int stmp = _seed(vcmd).to_int();
// uint64 otmp;
// short64_hash(&ktmp, ltmp, stmp, &otmp);
// cout << hex << "key: " << ktmp << ", index: " << (otmp & _tlen(vcmd).to_uint64()) << endl;
// }
			c_key.write((_keylen(vcmd),key));
			c_tapi.write((sc_uint<64>(C_INIT),sc_uint<64>(C_INIT),sc_uint<64>(_seed(vcmd)),sc_uint<64>(_seed(vcmd))));
		}
	}

	void ct_hash_out()
	{
		sc_uint<CD> vcmd[CR];
		sc_biguint<64> tapo, index;
		AXI_TD dat1;
		ACMD arsp;
		for (int i = 0; i < CR; i++) {
			c_vrsp[i].write(0);
			c_vwe[i].write(false);
		}
		while (true) {
			wait();
			tapo = c_tapo.read()(63,0);
			for (int i = 0; i < CR; i++) vcmd[i] = c_vcmd[i].read();
			index = tapo & sc_biguint<64>(_tlen(vcmd));
			// cout << hex << "index: " << index << endl;

			// prepare data out
			dat1.tdata = index;
			dat1.tid = _tid(vcmd);
			dat1.tdest = _tdest(vcmd);
			dat1.tkeep = -1;
			dat1.tlast = 1;

			// prepare response
			arsp.sid = _tid(vcmd);
			arsp.did = _tdest(vcmd);
			arsp.srac.len = (_hashlen(vcmd)+3) >> 2;
			arsp.srac.sel = hreg;
			arsp.srac.wr  = 0;
			arsp.srac.go  = 0;
			arsp.drac.len = arsp.srac.len;
			arsp.drac.sel = 4;
			arsp.drac.wr  = 1;
			arsp.drac.go  = 1;

			if (_seldo(vcmd)) {
				// write hash to data out
				m_dat1.write(dat1);
			} else {
				while (c_arsp.num_free() < 1) wait();
				// write hash to vrsp registers and send response
				c_vrsp[hreg  ].write(sc_uint<CD>(index(31, 0)));
				c_vrsp[hreg+1].write(sc_uint<CD>(index(63,32)));
				c_vwe[hreg  ].write(true);
				c_vwe[hreg+1].write(true);
				c_arsp.write(arsp);
			}
#if 0 // TODO: update status with count of keys in hash unit
			c_vrsp[0].write(_status(vcmd) | kcount);
			c_vwe[0].write(true);
#endif
		}
	}

#undef _status
#undef _tlen
#undef _keylen
#undef _hashlen
#undef _tid
#undef _tdest
#undef _seed
#undef _seldo
#undef _seldi
#undef _data
#undef hreg
#undef _hash

	SC_CTOR(hsu) :
		c_acmd("c_acmd", 1), // must be 1 to prevent register overwrite
		c_arsp("c_arsp", 1), // must be 1 to prevent register overwrite
		c_key("c_key", 2),
		c_tapi("c_tapi", 2),
		c_tapo("c_tapo", 2),
		u_ctlreg("u_ctlreg"),
		u_hash("u_hash")
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

		u_hash.clk(clk);
		u_hash.reset(reset);
		u_hash.s_dat(c_key);
		u_hash.s_tap(c_tapi);
		u_hash.m_tap(c_tapo);

		SC_CTHREAD(ct_key_in, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(ct_hash_out, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}

};

#endif // _HSU_H
