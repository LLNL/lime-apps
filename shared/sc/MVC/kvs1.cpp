
#include "pulse.h"
#include "kvs.h"
#include "cpu.h"

int sc_main(int argc , char *argv[])
{
	sc_report_handler::set_actions("/IEEE_Std_1666/deprecated", SC_DO_NOTHING);
	// sc_report_handler::set_actions(SC_ID_LOGIC_X_TO_BOOL_, SC_LOG);
	// sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_, SC_LOG);
	// sc_report_handler::set_actions(SC_ID_OBJECT_EXISTS_, SC_LOG);

	sc_clock clk("clk", 800, SC_PS); // create a 800ps period clock signal
	// sc_clock clk("clk", 1, SC_NS); // create a 1ns period clock signal
	sc_signal<bool> reset("reset");

	// channels - named in context of accelerator
	sc_fifo<AXI_TC_T> s_ctl0("s_ctl0", 2);
	sc_fifo<AXI_TC_T> m_ctl0("m_ctl0", 2);

	// sc_fifo<AXI_TC_T> s_ctl1("s_ctl1", 2);
	// sc_fifo<AXI_TC_T> m_ctl1("m_ctl1", 2);

	// sc_fifo<AXI_AW_T> m_mem0_aw("m_mem0_aw", 2); // LSU0 not used for kvs
	// sc_fifo<AXI_W_T > m_mem0_w ("m_mem0_w" , 2);
	// sc_fifo<AXI_B_T > m_mem0_b ("m_mem0_b" , 2);
	// sc_fifo<AXI_AR_T> m_mem0_ar("m_mem0_ar", 2);
	// sc_fifo<AXI_R_T > m_mem0_r ("m_mem0_r" , 2);

	sc_fifo<AXI_AW_T> m_mem1_aw("m_mem1_aw", 2);
	sc_fifo<AXI_W_T > m_mem1_w ("m_mem1_w" , 2);
	sc_fifo<AXI_B_T > m_mem1_b ("m_mem1_b" , 2);
	sc_fifo<AXI_AR_T> m_mem1_ar("m_mem1_ar", 2);
	sc_fifo<AXI_R_T > m_mem1_r ("m_mem1_r" , 2);

	sc_fifo<AXI_AW_T> m_mem2_aw("m_mem2_aw", 2);
	sc_fifo<AXI_W_T > m_mem2_w ("m_mem2_w" , 2);
	sc_fifo<AXI_B_T > m_mem2_b ("m_mem2_b" , 2);
	sc_fifo<AXI_AR_T> m_mem2_ar("m_mem2_ar", 2);
	sc_fifo<AXI_R_T > m_mem2_r ("m_mem2_r" , 2);

	pulse<RLEVEL> u_pulse("u_pulse");
	cpu u_cpu("u_cpu");
	kvs u_acc("u_acc");

	// connect pulse
	u_pulse.clk(clk);
	u_pulse.sig(reset);

	// connect CPU
	u_cpu.clk(clk);
	u_cpu.reset(reset);
	u_cpu.s_ctl(m_ctl0);
	u_cpu.m_ctl(s_ctl0);

	// connect accelerator
	u_acc.clk(clk);
	u_acc.reset(reset);
	u_acc.s_ctl0(s_ctl0);
	u_acc.m_ctl0(m_ctl0);
	u_acc.m_mem1_aw(m_mem1_aw);
	u_acc.m_mem1_w (m_mem1_w );
	u_acc.m_mem1_b (m_mem1_b );
	u_acc.m_mem1_ar(m_mem1_ar);
	u_acc.m_mem1_r (m_mem1_r );
	u_acc.m_mem2_aw(m_mem2_aw);
	u_acc.m_mem2_w (m_mem2_w );
	u_acc.m_mem2_b (m_mem2_b );
	u_acc.m_mem2_ar(m_mem2_ar);
	u_acc.m_mem2_r (m_mem2_r );

	cout << "time resolution: " << sc_get_time_resolution() << endl;
	cout << "max time: " << sc_max_time() << endl;
	SC_REPORT_INFO("/OSCI/SystemC", "Simulation begin.");
	sc_start();

	return u_cpu.exitval;
}
