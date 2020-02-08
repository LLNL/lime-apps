/*
 * clocks_sa.c
 *
 *  Created on: Dec 10, 2014
 *      Author: lloyd23
 */

#if defined(CLOCKS)

#include <stdio.h> /* printf */
#include "xparameters.h" /* XPAR_* */
#include "clocks.h"


#if defined(ZYNQ) && ZYNQ == _Z7_
/* Zynq-7000 Device */

#define SLCR           0xF8000000 /* System Level Control Registers */
#define SLCR_UNLOCK         0x008 /* 15:0 UNLOCK_KEY */
#define SLCR_UNLOCK_KEY    0xDF0D /*          0xDF0D */
#define ARM_PLL_CTRL        0x100 /* 18:12 PLL_FDIV, 4 PLL_BYPASS_FORCE, 3 PLL_BYPASS_QUAL, 1 PLL_PWRDWN, 0 PLL_RESET */
#define APLL_CHK       0x00036008 /*             54,                  0,                 1,            0,           0 */
#define DDR_PLL_CTRL        0x104 /* 18:12 PLL_FDIV, 4 PLL_BYPASS_FORCE, 3 PLL_BYPASS_QUAL, 1 PLL_PWRDWN, 0 PLL_RESET */
#define IO_PLL_CTRL         0x108 /* 18:12 PLL_FDIV, 4 PLL_BYPASS_FORCE, 3 PLL_BYPASS_QUAL, 1 PLL_PWRDWN, 0 PLL_RESET */
#define IOPLL_CHK      0x0001E008 /*             30,                  0,                 1,            0,           0 */
#define ARM_CLK_CTRL        0x120 /* 28:24 CLKACT, 13:8 DIVISOR, 5:4 SRCSEL */
#define ACPU_EMUL      0x1F000E00 /*          all,           14,       APLL */
#define ACPU_NORM      0x1F000300 /*          all,            3,       APLL */
#define DDR_CLK_CTRL        0x124 /* 31:26 DDR_2XCLK_DIVISOR, 25:20 DDR_3XCLK_DIVISOR, 1 DDR_2XCLKACT, 0 DDR_3XCLKACT */
#define FPGA0_CLK_CTRL      0x170 /* 25:20 DIVISOR1, 13:8 DIVISOR0, 5:4 SRCSEL */
#define PL0_EMUL       0x00101000 /*              1,            16,      IOPLL */
#define PL0_NORM       0x00100600 /*              1,             6,      IOPLL */
#define FPGA1_CLK_CTRL      0x180 /* 25:20 DIVISOR1, 13:8 DIVISOR0, 5:4 SRCSEL */
#define CLK_621_TRUE        0x1C4 /* 0 CLK_621_TRUE */
#define CLK_621_CHK             0 /*        (4:2:1) */

void clocks_emulate(void)
{
	volatile unsigned int *unlock   = (unsigned int *)(SLCR+SLCR_UNLOCK);
	volatile unsigned int *apll_c   = (unsigned int *)(SLCR+ARM_PLL_CTRL);
	volatile unsigned int *dpll_c   = (unsigned int *)(SLCR+DDR_PLL_CTRL);
	volatile unsigned int *iopll_c  = (unsigned int *)(SLCR+IO_PLL_CTRL);
	volatile unsigned int *arm_cc   = (unsigned int *)(SLCR+ARM_CLK_CTRL);
	volatile unsigned int *ddr_cc   = (unsigned int *)(SLCR+DDR_CLK_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(SLCR+FPGA0_CLK_CTRL); /* Accelerator & Peripheral Clock */
	volatile unsigned int *fpga1_cc = (unsigned int *)(SLCR+FPGA1_CLK_CTRL); /* Interconnect & APM */
	volatile unsigned int *clk_621  = (unsigned int *)(SLCR+CLK_621_TRUE);

	printf("SRAM_W:%d SRAM_R:%d DRAM_W:%d DRAM_R:%d\nQUEUE_W:%d QUEUE_R:%d TRANS:%d W:%d R:%d\n",
		T_SRAM_W, T_SRAM_R, T_DRAM_W, T_DRAM_R, T_QUEUE_W, T_QUEUE_R, T_TRANS,
		T_DRAM_W+T_QUEUE_W+T_TRANS, T_DRAM_R+T_QUEUE_R+T_TRANS);
	printf("ARM_PLL_CTRL:%08X DDR_PLL_CTRL:%08X IO_PLL_CTRL:%08X CLK_621:%X\n", *apll_c, *dpll_c, *iopll_c, *clk_621);
	if (*apll_c != APLL_CHK || *iopll_c != IOPLL_CHK || *clk_621 != CLK_621_CHK) {
		printf(" -- error: clocks_emulate: incompatible clock configuration\n");
		return;
	}

	*unlock   = SLCR_UNLOCK_KEY;
	*arm_cc   = ACPU_EMUL; /* ARM at 2.57 GHz */
	*fpga0_cc = PL0_EMUL; /* DRE at 1.25 GHz */
	printf("ARM_CLK_CTRL:%08X DDR_CLK_CTRL:%08X\n", *arm_cc, *ddr_cc);
	printf("FPGA0_CLK_CTRL:%08X FPGA1_CLK_CTRL:%08X\n", *fpga0_cc, *fpga1_cc);
	if (*arm_cc != ACPU_EMUL || *fpga0_cc != PL0_EMUL) {
		printf(" -- error: clocks_emulate: clock configuration not set\n");
		return;
	}

#if defined(XPAR_DELAY_0_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay0 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	delay0[2] = 4*(T_SRAM_W+T_TRANS)           - 44; delay0[4] = 4*(T_SRAM_R+T_TRANS)           - 39; /* .25 ns per count */
	delay1[2] = 4*(T_DRAM_W+T_QUEUE_W+T_TRANS) - 45; delay1[4] = 4*(T_DRAM_R+T_QUEUE_R+T_TRANS) - 44;
	printf("Slot 0 - CPU_SRAM_B:%u CPU_SRAM_R:%u CPU_DRAM_B:%u CPU_DRAM_R:%u\n", delay0[2], delay0[4], delay1[2], delay1[4]);
#endif

#if defined(XPAR_DELAY_1_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay2 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_0_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_1_BASEADDR; /* slot 1, ACC DRAM W, R */
	delay2[2] = 4*(T_SRAM_W)                   - 21; delay2[4] = 4*(T_SRAM_R)                   - 36;
	delay3[2] = 4*(T_DRAM_W+T_QUEUE_W)         - 21; delay3[4] = 4*(T_DRAM_R+T_QUEUE_R)         - 37;
	printf("Slot 1 - ACC_SRAM_B:%u ACC_SRAM_R:%u ACC_DRAM_B:%u ACC_DRAM_R:%u\n", delay2[2], delay2[4], delay3[2], delay3[4]);
#endif
}

void clocks_normal(void)
{
	volatile unsigned int *unlock   = (unsigned int *)(SLCR+SLCR_UNLOCK);
	volatile unsigned int *arm_cc   = (unsigned int *)(SLCR+ARM_CLK_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(SLCR+FPGA0_CLK_CTRL);
	*unlock   = SLCR_UNLOCK_KEY;
	*arm_cc   = ACPU_NORM;
	*fpga0_cc = PL0_NORM;
	if (*arm_cc != ACPU_NORM || *fpga0_cc != PL0_NORM) {
		printf(" -- error: clocks_normal: clock configuration not set\n");
		return;
	}
#if defined(XPAR_DELAY_0_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay0 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	delay0[2] = 0; delay0[4] = 0; delay1[2] = 0; delay1[4] = 0;
#endif
#if defined(XPAR_DELAY_1_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay2 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_0_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_1_BASEADDR; /* slot 1, ACC DRAM W, R */
	delay2[2] = 0; delay2[4] = 0; delay3[2] = 0; delay3[4] = 0;
#endif
}

#elif defined(ZYNQ) && ZYNQ == _ZU_
/* Zynq UltraScale+ Device */

#define FPD_SLCR   0xFD610000 /* FPD System-level Control */
#define wprot0          0x000 /* 0 active */
#define wprot0_off 0x00000000 /*        0 */

#define CRF_APB    0xFD1A0000 /* Clock and Reset control registers for FPD */
#define APLL_CTRL       0x020 /* 26:24 POST_SRC, 22:20 PRE_SRC, 16 DIV2, 14:8 FBDIV, 3 BYPASS, 0 RESET */
#define APLL_CHK   0x00014200 /*             NA,    PS_REF_CLK,       1,         66,        0,       0 */
#define DPLL_CTRL       0x02C /* 26:24 POST_SRC, 22:20 PRE_SRC, 16 DIV2, 14:8 FBDIV, 3 BYPASS, 0 RESET */
#define ACPU_CTRL       0x060 /* 25 CLKACT_HALF, 24 CLKACT_FULL, 13:8 DIVISOR0, 2:0 SRCSEL */
#define ACPU_EMUL  0x03000800 /*              1,              1,             8,       APLL */
#define ACPU_NORM  0x03000100 /*              1,              1,             1,       APLL */
#define DDR_CTRL        0x080 /* 13:8 DIVISOR0, 2:0 SRCSEL */

#define CRL_APB    0xFF5E0000 /* Clock and Reset control registers for LPD */
#define IOPLL_CTRL      0x020 /* 26:24 POST_SRC, 22:20 PRE_SRC, 16 DIV2, 14:8 FBDIV, 3 BYPASS, 0 RESET */
#define IOPLL_CHK  0x00015A00 /*             NA,    PS_REF_CLK,       1,         90,        0,       0 */
#define PL0_REF_CTRL    0x0C0 /* 24 CLKACT, 21:16 DIVISOR1, 13:8 DIVISOR0, 2:0 SRCSEL */
#define PL0_EMUL   0x01011800 /*         1,              1,            24,      IOPLL */
#define PL0_NORM   0x01010600 /*         1,              1,             6,      IOPLL */
#define PL1_REF_CTRL    0x0C4 /* 24 CLKACT, 21:16 DIVISOR1, 13:8 DIVISOR0, 2:0 SRCSEL */

void clocks_emulate(void)
{
	volatile unsigned int *unlock   = (unsigned int *)(FPD_SLCR+wprot0);
	volatile unsigned int *apll_c   = (unsigned int *)(CRF_APB+APLL_CTRL);
	volatile unsigned int *dpll_c   = (unsigned int *)(CRF_APB+DPLL_CTRL);
	volatile unsigned int *arm_cc   = (unsigned int *)(CRF_APB+ACPU_CTRL);
	volatile unsigned int *ddr_cc   = (unsigned int *)(CRF_APB+DDR_CTRL);
	volatile unsigned int *iopll_c  = (unsigned int *)(CRL_APB+IOPLL_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(CRL_APB+PL0_REF_CTRL); /* Accelerator & Peripheral Clock */
	volatile unsigned int *fpga1_cc = (unsigned int *)(CRL_APB+PL1_REF_CTRL); /* Interconnect & APM */

	printf("SRAM_W:%d SRAM_R:%d DRAM_W:%d DRAM_R:%d\nQUEUE_W:%d QUEUE_R:%d TRANS:%d W:%d R:%d\n",
			T_SRAM_W, T_SRAM_R, T_DRAM_W, T_DRAM_R, T_QUEUE_W, T_QUEUE_R, T_TRANS,
			T_DRAM_W+T_QUEUE_W+T_TRANS, T_DRAM_R+T_QUEUE_R+T_TRANS);
	printf("ARM_PLL_CTRL:%08X DDR_PLL_CTRL:%08X IO_PLL_CTRL:%08X\n", *apll_c, *dpll_c, *iopll_c);
	if (*apll_c != APLL_CHK || *iopll_c != IOPLL_CHK) {
		printf(" -- error: clocks_emulate: incompatible clock configuration\n");
		return;
	}

	*unlock   = wprot0_off;
	*arm_cc   = ACPU_EMUL; /* ARM at 2.75 GHz */
	*fpga0_cc = PL0_EMUL; /* DRE at 1.25 GHz */
	printf("ARM_CLK_CTRL:%08X DDR_CLK_CTRL:%08X\n", *arm_cc, *ddr_cc);
	printf("FPGA0_CLK_CTRL:%08X FPGA1_CLK_CTRL:%08X\n", *fpga0_cc, *fpga1_cc);
	if (*arm_cc != ACPU_EMUL || *fpga0_cc != PL0_EMUL) {
		printf(" -- error: clocks_emulate: clock configuration not set\n");
		return;
	}

	/* TODO: Make two sets of delay calibration values, */
	/* one for the zcu102 and the other for the sidewinder */
	/* The values here likely apply to only one of the boards, */
	/* since the DDR memories run at different frequencies. */
#if defined(XPAR_DELAY_0_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay0 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	delay0[2] = 6*(T_SRAM_W+T_TRANS)           - 52; delay0[4] = 6*(T_SRAM_R+T_TRANS)           - 69; /* .16 ns per count */
	delay1[2] = 6*(T_DRAM_W+T_QUEUE_W+T_TRANS) - 52; delay1[4] = 6*(T_DRAM_R+T_QUEUE_R+T_TRANS) - 69;
	printf("Slot 0 - CPU_SRAM_B:%u CPU_SRAM_R:%u CPU_DRAM_B:%u CPU_DRAM_R:%u\n", delay0[2], delay0[4], delay1[2], delay1[4]);
#endif

#if defined(XPAR_DELAY_1_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay2 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_0_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_1_BASEADDR; /* slot 1, ACC DRAM W, R */
	delay2[2] = 6*(T_SRAM_W)                   - 48; delay2[4] = 6*(T_SRAM_R)                   - 66;
	delay3[2] = 6*(T_DRAM_W+T_QUEUE_W)         - 48; delay3[4] = 6*(T_DRAM_R+T_QUEUE_R)         - 66;
	printf("Slot 1 - ACC_SRAM_B:%u ACC_SRAM_R:%u ACC_DRAM_B:%u ACC_DRAM_R:%u\n", delay2[2], delay2[4], delay3[2], delay3[4]);
#endif
}

void clocks_normal(void)
{
	volatile unsigned int *unlock   = (unsigned int *)(FPD_SLCR+wprot0);
	volatile unsigned int *arm_cc   = (unsigned int *)(CRF_APB+ACPU_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(CRL_APB+PL0_REF_CTRL);
	*unlock   = wprot0_off;
	*arm_cc   = ACPU_NORM;
	*fpga0_cc = PL0_NORM;
	if (*arm_cc != ACPU_NORM || *fpga0_cc != PL0_NORM) {
		printf(" -- error: clocks_normal: clock configuration not set\n");
		return;
	}
#if defined(XPAR_DELAY_0_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay0 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_0_BASEADDR; /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)XPAR_DELAY_0_AXI_DELAY_1_BASEADDR; /* slot 0, CPU DRAM W, R */
	delay0[2] = 0; delay0[4] = 0; delay1[2] = 0; delay1[4] = 0;
#endif
#if defined(XPAR_DELAY_1_AXI_DELAY_0_BASEADDR)
	volatile unsigned int *delay2 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_0_BASEADDR; /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)XPAR_DELAY_1_AXI_DELAY_1_BASEADDR; /* slot 1, ACC DRAM W, R */
	delay2[2] = 0; delay2[4] = 0; delay3[2] = 0; delay3[4] = 0;
#endif
}

#endif /* ZYNQ */

#endif /* CLOCKS */
