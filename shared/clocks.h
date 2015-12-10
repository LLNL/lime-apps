/*
 * clocks.h
 *
 *  Created on: Dec 10, 2014
 *      Author: lloyd23
 */

#ifndef CLOCKS_H_
#define CLOCKS_H_

#define tesec(f,s) ((us long long)tdiff(f,s)/(double)TICKS_ESEC)

#if defined(CLOCKS) && defined(ZYNQ)

#include <stdio.h> /* printf */
#include "xparameters.h" /* XPAR_* */

#define CLOCKS_EMULATE clocks_emulate();
#define CLOCKS_NORMAL  clocks_normal();
#define TICKS_ESEC (2571428545UL/2)

#define T_V_W 103 // DRAM Vault Write, off-chip
#define T_V_R 84  // DRAM Vault Read, off-chip
#define T_SRAM_W 10
#define T_SRAM_R 10
#define T_DRAM_W 45
#define T_DRAM_R 45
#define T_QUEUE_W (T_V_W-T_DRAM_W-T_TRANS) // 00 20 40
#define T_QUEUE_R (T_V_R-T_DRAM_R-T_TRANS) // 00 20 40
#define T_TRANS  24  // 24 32 40

inline void clocks_emulate(void)
{
	volatile unsigned int *unlock   = (unsigned int *)0xF8000008;
	volatile unsigned int *pll_c    = (unsigned int *)0xF8000100;
	volatile unsigned int *arm_cc   = (unsigned int *)0xF8000120;
	volatile unsigned int *ddr_cc   = (unsigned int *)0xF8000124;
	volatile unsigned int *fpga0_cc = (unsigned int *)0xF8000170;
	volatile unsigned int *fpga1_cc = (unsigned int *)0xF8000180;
	volatile unsigned int *clk_621  = (unsigned int *)0xF80001C4;
	volatile unsigned int *delay0   = (unsigned int *)XPAR_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1   = (unsigned int *)XPAR_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	volatile unsigned int *delay2   = (unsigned int *)XPAR_AXI_DELAY_2_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3   = (unsigned int *)XPAR_AXI_DELAY_3_BASEADDR; /* slot 1, ACC DRAM W, R */
	volatile unsigned int *delay4   = (unsigned int *)XPAR_AXI_DELAY_4_BASEADDR; /* slot 2, CPU ACC  W, R */
	*unlock   =     0xDF0D;
	*arm_cc   = 0x1F000E00; /* 13:8 DIVISOR = 14 */
	*clk_621  = 0x00000000; /* bit 0 CLK_621_TRUE = 0 (4:2:1) */
	*fpga0_cc = 0x00101000; /* 25:20 DIVISOR1 = 1, 13:8 DIVISOR0 = 16, 5:4 SRCSEL = 0 (IO PLL) */
	*fpga1_cc = 0x00101000; /* 25:20 DIVISOR1 = 1, 13:8 DIVISOR0 = 16, 5:4 SRCSEL = 0 (IO PLL) */

	/* when using DRAM (0x00080000) for SRAM */
	// CPU_ACC target: min 4 accesses (42 ns) + CPU compare for ready (10 ns) + ACC command interpreter (80) = 258 ns
	// CPU_ACC: 0 -> 0.339 usec, 20 -> 0.627, 32 -> 0.819 usec, 64 -> 1.331 usec emulated time

	delay0[2] = 4*(T_SRAM_W+T_TRANS)           - 44; delay0[4] = 4*(T_SRAM_R+T_TRANS)           - 39; /* .25 ns per count */
	delay1[2] = 4*(T_DRAM_W+T_QUEUE_W+T_TRANS) - 45; delay1[4] = 4*(T_DRAM_R+T_QUEUE_R+T_TRANS) - 44;
	delay2[2] = 4*(T_SRAM_W)                   - 21; delay2[4] = 4*(T_SRAM_R)                   - 36;
	delay3[2] = 4*(T_DRAM_W+T_QUEUE_W)         - 21; delay3[4] = 4*(T_DRAM_R+T_QUEUE_R)         - 37;
	delay4[0] = delay4[1] = delay4[2] = delay4[3] = delay4[4] = 0; /* .80 ns per count */
	printf("SRAM_W:%u SRAM_R:%u DRAM_W:%u DRAM_R:%u QUEUE_W:%u QUEUE_R:%u TRANS:%u V_W:%u V_R:%u\n",
			T_SRAM_W, T_SRAM_R, T_DRAM_W, T_DRAM_R, T_QUEUE_W, T_QUEUE_R, T_TRANS,
			T_DRAM_W+T_QUEUE_W+T_TRANS, T_DRAM_R+T_QUEUE_R+T_TRANS);
	printf("ARM_PLL_CTRL:%08X DDR_PLL_CTRL:%08X IO_PLL_CTRL:%08X\n", pll_c[0], pll_c[1], pll_c[2]);
	printf("ARM_CLK_CTRL:%08X CLK_621_TRUE:%08X DDR_CLK_CTRL:%08X\n", *arm_cc, *clk_621, *ddr_cc);
	printf("FPGA0_CLK_CTRL:%08X FPGA1_CLK_CTRL:%08X\n", *fpga0_cc, *fpga1_cc);
	printf("Slot 0 - CPU_SRAM_B:%u CPU_SRAM_R:%u CPU_DRAM_B:%u CPU_DRAM_R:%u\n", delay0[2], delay0[4], delay1[2], delay1[4]);
	printf("Slot 1 - ACC_SRAM_B:%u ACC_SRAM_R:%u ACC_DRAM_B:%u ACC_DRAM_R:%u\n", delay2[2], delay2[4], delay3[2], delay3[4]);
	printf("Slot 2 - CPU_ACC(AW,W,B,AR,R):%u,%u,%u,%u,%u\n", delay4[0], delay4[1], delay4[2], delay4[3], delay4[4]);
}

inline void clocks_normal(void)
{
	volatile unsigned int *unlock   = (unsigned int *)0xF8000008;
	volatile unsigned int *arm_cc   = (unsigned int *)0xF8000120;
	//volatile unsigned int *ddr_cc   = (unsigned int *)0xF8000124;
	volatile unsigned int *fpga0_cc = (unsigned int *)0xF8000170;
	volatile unsigned int *fpga1_cc = (unsigned int *)0xF8000180;
	volatile unsigned int *clk_621  = (unsigned int *)0xF80001C4;
	volatile unsigned int *delay0   = (unsigned int *)XPAR_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1   = (unsigned int *)XPAR_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	volatile unsigned int *delay2   = (unsigned int *)XPAR_AXI_DELAY_2_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3   = (unsigned int *)XPAR_AXI_DELAY_3_BASEADDR; /* slot 1, ACC DRAM W, R */
	volatile unsigned int *delay4   = (unsigned int *)XPAR_AXI_DELAY_4_BASEADDR; /* slot 2, CPU Device W, R */
	*unlock   =     0xDF0D;
	*arm_cc   = 0x1F000300; /* 13:8 DIVISOR = 3 */
	*clk_621  = 0x00000000; /* bit 0 CLK_621_TRUE = 0 (4:2:1) */
	*fpga0_cc = 0x00100600; /* 25:20 DIVISOR1 = 1, 13:8 DIVISOR0 = 6, 5:4 SRCSEL = 0 (IO PLL) */
	*fpga1_cc = 0x00100600; /* 25:20 DIVISOR1 = 1, 13:8 DIVISOR0 = 6, 5:4 SRCSEL = 0 (IO PLL) */
	delay0[2] = 0; delay0[4] = 0; delay1[2] = 0; delay1[4] = 0;
	delay2[2] = 0; delay2[4] = 0; delay3[2] = 0; delay3[4] = 0;
	delay4[0] = delay4[1] = delay4[2] = delay4[3] = delay4[4] = 0;
}

#else /* CLOCKS */

#define CLOCKS_EMULATE
#define CLOCKS_NORMAL
#define TICKS_ESEC TICKS_SEC

#endif /* CLOCKS */

#endif /* CLOCKS_H_ */
