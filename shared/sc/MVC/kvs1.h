 
#ifndef _KVS_H
#define _KVS_H

#include "systemc.h"

#include "port_strm.h"
#include "cswitch.h"
#include "lsu.h"
#include "hsu.h"
#include "spsel.h"
#include "indel.h"

// #define ARM0_PN 0 /* ARM 0 port number */
// #define MCU0_PN 1 /* MCU 0 port number */
// #define LSU0_PN 2 /* LSU 0 port number */
// #define LSU1_PN 3 /* LSU 1 port number */
// #define HSU0_PN 4 /* HSU 0 port number */
// #define LSU2_PN 5 /* LSU 2 port number */
// #define PRU0_PN 6 /* PRU 0 port number */

SC_MODULE(kvs)
{
	// ports
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl0;
	sc_fifo_out<AXI_TC_T> m_ctl0;

	// sc_fifo_in <AXI_TC_T> s_ctl1; // MCU0 not used for kvs
	// sc_fifo_out<AXI_TC_T> m_ctl1;

	// sc_fifo_out<AXI_AW_T> m_mem0_aw; // LSU0 not used for kvs
	// sc_fifo_out<AXI_W_T > m_mem0_w ;
	// sc_fifo_in <AXI_B_T > m_mem0_b ;
	// sc_fifo_out<AXI_AR_T> m_mem0_ar;
	// sc_fifo_in <AXI_R_T > m_mem0_r ;

	sc_fifo_out<AXI_AW_T> m_mem1_aw;
	sc_fifo_out<AXI_W_T > m_mem1_w ;
	sc_fifo_in <AXI_B_T > m_mem1_b ;
	sc_fifo_out<AXI_AR_T> m_mem1_ar;
	sc_fifo_in <AXI_R_T > m_mem1_r ;

	sc_fifo_out<AXI_AW_T> m_mem2_aw;
	sc_fifo_out<AXI_W_T > m_mem2_w ;
	sc_fifo_in <AXI_B_T > m_mem2_b ;
	sc_fifo_out<AXI_AR_T> m_mem2_ar;
	sc_fifo_in <AXI_R_T > m_mem2_r ;

	// channels - named in context of master
	sc_fifo<AXI_TC_T> csw1_c; // MCU0 not used for kvs
	sc_fifo<AXI_TC_T> csw2_c; // LSU0 not used for kvs
	sc_fifo<AXI_TC_T> csw3_c;
	sc_fifo<AXI_TC_T> csw4_c;
	sc_fifo<AXI_TC_T> csw5_c;
	sc_fifo<AXI_TC_T> csw6_c;
	sc_fifo<AXI_TC_T> csw7_c;
	// sc_fifo<AXI_TC_T> lsu0_c; // LSU0 not used for kvs
	// sc_fifo<AXI_TD_T> lsu0_d;
	sc_fifo<AXI_TC_T> lsu1_c;
	sc_fifo<AXI_TD_T> lsu1_d;
	sc_fifo<AXI_TC_T> lsu2_c;
	sc_fifo<AXI_TD_T> lsu2_d;
	sc_fifo<AXI_TC_T> ssu0_c;
	sc_fifo<AXI_TD_T> ssu0_d1;
	sc_fifo<AXI_TD_T> ssu0_d2;
	sc_fifo<AXI_TC_T> hsu0_c;
	sc_fifo<AXI_TD_T> hsu0_d1;
	sc_fifo<AXI_TD_T> hsu0_d2;
	sc_fifo<AXI_TC_T> idu0_c;
	sc_fifo<AXI_TD_T> idu0_d1;
	sc_fifo<AXI_TD_T> idu0_d2;
	sc_fifo<AXI_TD_T> stub_d1;
	sc_fifo<AXI_TD_T> stub_d2;

	// modules
	cswitch<8> u_csw0; // control switch 0
	// lsu        u_lsu0; // load store unit 0 not used for kvs
	lsu        u_lsu1; // load store unit 1
	lsu        u_lsu2; // load store unit 2
	spsel      u_ssu0; // split select unit 0
	hsu        u_hsu0; // hash unit 0
	indel      u_idu0; // insert delete unit 0

	SC_CTOR(kvs) :
		// ports

		// channels - named in context of master
		csw1_c ("csw1_c" , 2), // MCU0 not used for kvs
		csw2_c ("csw2_c" , 2), // LSU0 not used for kvs
		csw3_c ("csw3_c" , 2),
		csw4_c ("csw4_c" , 2),
		csw5_c ("csw5_c" , 2),
		csw6_c ("csw6_c" , 2),
		csw7_c ("csw7_c" , 2),
		// lsu0_c ("lsu0_c" , 2), // LSU0 not used for kvs
		// lsu0_d ("lsu0_d" , 2),
		lsu1_c ("lsu1_c" , 2),
		lsu1_d ("lsu1_d" , 2),
		lsu2_c ("lsu2_c" , 2),
		lsu2_d ("lsu2_d" , 2),
		ssu0_c ("ssu0_c" , 2),
		ssu0_d1("ssu0_d1", 256),
		ssu0_d2("ssu0_d2", 2),
		hsu0_c ("hsu0_c" , 2),
		hsu0_d1("hsu0_d1", 2),
		hsu0_d2("hsu0_d2", 2),
		idu0_c ("idu0_c" , 2),
		idu0_d1("idu0_d1", 2),
		idu0_d2("idu0_d2", 2),

		stub_d1("stub_d1", 2),
		stub_d2("stub_d2", 2),

		// modules
		u_csw0("u_csw0"),
		// u_lsu0("u_lsu0"), // LSU0 not used for kvs
		u_lsu1("u_lsu1"),
		u_lsu2("u_lsu2"),
		u_ssu0("u_ssu0"),
		u_hsu0("u_hsu0"),
		u_idu0("u_idu0")
	{
		u_csw0.clk(clk);
		u_csw0.reset(reset);
		u_csw0.s_port[0](s_ctl0);
		u_csw0.m_port[0](m_ctl0);
		u_csw0.s_port[1](csw1_c); // MCU0 not used for kvs
		u_csw0.m_port[1](csw1_c);
		u_csw0.s_port[2](csw2_c); // LSU0 not used for kvs
		u_csw0.m_port[2](csw2_c);
		u_csw0.s_port[3](lsu1_c);
		u_csw0.m_port[3](csw3_c);
		u_csw0.s_port[4](hsu0_c);
		u_csw0.m_port[4](csw4_c);
		u_csw0.s_port[5](lsu2_c);
		u_csw0.m_port[5](csw5_c);
		u_csw0.s_port[6](idu0_c);
		u_csw0.m_port[6](csw6_c);
		u_csw0.s_port[7](csw7_c); // not used for kvs
		u_csw0.m_port[7](csw7_c);

		u_lsu1.clk(clk);
		u_lsu1.reset(reset);
		u_lsu1.s_ctl(csw3_c);
		u_lsu1.m_ctl(lsu1_c);
		u_lsu1.s_dat(stub_d1);
		u_lsu1.m_dat(lsu1_d);
		u_lsu1.m_mem_aw(m_mem1_aw);
		u_lsu1.m_mem_w (m_mem1_w );
		u_lsu1.m_mem_b (m_mem1_b );
		u_lsu1.m_mem_ar(m_mem1_ar);
		u_lsu1.m_mem_r (m_mem1_r );

		u_lsu2.clk(clk);
		u_lsu2.reset(reset);
		u_lsu2.s_ctl(csw5_c);
		u_lsu2.m_ctl(lsu2_c);
		u_lsu2.s_dat(idu0_d1);
		u_lsu2.m_dat(lsu2_d);
		u_lsu2.m_mem_aw(m_mem2_aw);
		u_lsu2.m_mem_w (m_mem2_w );
		u_lsu2.m_mem_b (m_mem2_b );
		u_lsu2.m_mem_ar(m_mem2_ar);
		u_lsu2.m_mem_r (m_mem2_r );

		u_ssu0.clk(clk);
		u_ssu0.reset(reset);
		u_ssu0.s_ctl(ssu0_c);
		u_ssu0.m_ctl(ssu0_c);
		u_ssu0.s_dat1(lsu1_d);
		u_ssu0.m_dat1(ssu0_d1);
		u_ssu0.s_dat2(stub_d2);
		u_ssu0.m_dat2(ssu0_d2);

		u_hsu0.clk(clk);
		u_hsu0.reset(reset);
		u_hsu0.s_ctl(csw4_c);
		u_hsu0.m_ctl(hsu0_c);
		u_hsu0.s_dat1(ssu0_d2);
		u_hsu0.m_dat1(hsu0_d1);
		u_hsu0.s_dat2(hsu0_d2);
		u_hsu0.m_dat2(hsu0_d2);

		u_idu0.clk(clk);
		u_idu0.reset(reset);
		u_idu0.s_ctl(csw6_c);
		u_idu0.m_ctl(idu0_c);
		u_idu0.s_dat1(ssu0_d1);
		u_idu0.m_dat1(idu0_d1);
		u_idu0.s_dat2(lsu2_d);
		u_idu0.m_dat2(idu0_d2);
	}

};

#endif // _KVS_H
