
#include "reset.h"
#include "kvs.h"


SC_MODULE(tb_driver)
{
	sc_in<bool> clk;
	sc_in<bool> reset;

	sc_fifo_in <AXI_TC_T> s_ctl;
	sc_fifo_out<AXI_TC_T> m_ctl;

	int retval;

	void send_ctl()
	{
		AXI_TC ctmp_t;
		while (true) {
			wait();
#ifndef __SYNTHESIS__
			cout << "tb_driver::send_ctl ts: " << sc_time_stamp() << ", ctmp_t.tdata: " << ctmp_t.tdata << endl;
#endif
			m_ctl.write(ctmp_t);
			ctmp_t.tdata += 1;
		}
	}

	void recv_ctl()
	{
		AXI_TC ctmp_t;
		while (true) {
			wait();
			ctmp_t = s_ctl.read();
#ifndef __SYNTHESIS__
			cout << "tb_driver::recv_ctl ts: " << sc_time_stamp() << ", ctmp_t.tdata: " << ctmp_t.tdata << endl;
#endif
		}
	}

	SC_CTOR(tb_driver) :
		clk("clk"),
		reset("reset"),
		s_ctl("s_ctl"),
		m_ctl("m_ctl"),
		retval(0)
	{
		SC_CTHREAD(send_ctl, clk.pos());
		reset_signal_is(reset, RLEVEL);

		SC_CTHREAD(recv_ctl, clk.pos());
		reset_signal_is(reset, RLEVEL);
	}
};


int sc_main(int argc , char *argv[])
{
	sc_report_handler::set_actions("/IEEE_Std_1666/deprecated", SC_DO_NOTHING);
	sc_report_handler::set_actions(SC_ID_LOGIC_X_TO_BOOL_, SC_LOG);
	sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_, SC_LOG);
	sc_report_handler::set_actions(SC_ID_OBJECT_EXISTS_, SC_LOG);

	sc_clock clk("clk", 10, SC_NS); // create a 10ns period clock signal
	sc_signal<bool> reset("reset");

	sc_fifo<AXI_TC_T> s_ctl("s_ctl", 2);
	sc_fifo<AXI_TC_T> m_ctl("m_ctl", 2);

	sc_fifo<AXI_AW_T> m_mem0_aw("m_mem0_aw", 2);
	sc_fifo<AXI_W_T > m_mem0_w ("m_mem0_w" , 2);
	sc_fifo<AXI_B_T > m_mem0_b ("m_mem0_b" , 2);
	sc_fifo<AXI_AR_T> m_mem0_ar("m_mem0_ar", 2);
	sc_fifo<AXI_R_T > m_mem0_r ("m_mem0_r" , 2);

	tb_reset u_tb_reset("u_tb_reset");
	tb_driver u_tb_driver("u_tb_driver");
	kvs u_dut("u_dut");

	// connect reset
	u_tb_reset.clk(clk);
	u_tb_reset.reset(reset);

	// connect driver
	u_tb_driver.clk(clk);
	u_tb_driver.reset(reset);
	u_tb_driver.s_ctl(m_ctl);
	u_tb_driver.m_ctl(s_ctl);

	// connect DUT
	u_dut.clk(clk);
	u_dut.reset(reset);
	u_dut.s_ctl(s_ctl);
	u_dut.m_ctl(m_ctl);
	u_dut.m_mem0_aw(m_mem0_aw);
	u_dut.m_mem0_w (m_mem0_w );
	u_dut.m_mem0_b (m_mem0_b );
	u_dut.m_mem0_ar(m_mem0_ar);
	u_dut.m_mem0_r (m_mem0_r );

	cout << "INFO: Simulating " << endl;

	// start simulation 
	sc_start(200, SC_NS);

	if (u_tb_driver.retval != 0) {
		printf("Test failed!\n");
	} else {
		printf("Test passed!\n");
	}
	return u_tb_driver.retval;
}
