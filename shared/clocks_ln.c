/*
 * clocks_ln.c
 *
 *  Created on: Jan 31, 2020
 *      Author: lloyd23
 */

#if defined(CLOCKS)

#include <stdio.h> /* printf, fprintf, perror, fopen, fwrite */
#include <stdlib.h> /* exit */
#include <stdint.h> /* uint64_t, uintptr_t */

#include "clocks.h"
#include "devtree.h"

#define DEV_TREE "/sys/firmware/devicetree/base/amba_pl@0"

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

/* TODO: make clocks_init() and clocks_finish() functions (like monitor_ln.c) */


static void *dev_smmap(const char *name, int inst)
{
	struct {uint64_t len, addr;} reg;
	int found = dev_search(DEV_TREE, name, inst, "reg", &reg, sizeof(reg));
	return (found == inst) ? dev_mmap(reg.addr) : MAP_FAILED;
}

void clocks_emulate(void)
{
	char *fpd_slcr = (char *)dev_mmap(FPD_SLCR);
	char *crf_apb  = (char *)dev_mmap(CRF_APB);
	char *crl_apb  = (char *)dev_mmap(CRL_APB);
	if (fpd_slcr == MAP_FAILED || crf_apb == MAP_FAILED || crl_apb == MAP_FAILED) goto ce_return;

	volatile unsigned int *unlock   = (unsigned int *)(fpd_slcr+wprot0);
	volatile unsigned int *apll_c   = (unsigned int *)(crf_apb+APLL_CTRL);
	volatile unsigned int *dpll_c   = (unsigned int *)(crf_apb+DPLL_CTRL);
	volatile unsigned int *arm_cc   = (unsigned int *)(crf_apb+ACPU_CTRL);
	volatile unsigned int *ddr_cc   = (unsigned int *)(crf_apb+DDR_CTRL);
	volatile unsigned int *iopll_c  = (unsigned int *)(crl_apb+IOPLL_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(crl_apb+PL0_REF_CTRL); /* Accelerator & Peripheral Clock */
	volatile unsigned int *fpga1_cc = (unsigned int *)(crl_apb+PL1_REF_CTRL); /* Interconnect & APM */

	printf("SRAM_W:%d SRAM_R:%d DRAM_W:%d DRAM_R:%d\nQUEUE_W:%d QUEUE_R:%d TRANS:%d W:%d R:%d\n",
			T_SRAM_W, T_SRAM_R, T_DRAM_W, T_DRAM_R, T_QUEUE_W, T_QUEUE_R, T_TRANS,
			T_DRAM_W+T_QUEUE_W+T_TRANS, T_DRAM_R+T_QUEUE_R+T_TRANS);
	printf("ARM_PLL_CTRL:%08X DDR_PLL_CTRL:%08X IO_PLL_CTRL:%08X\n", *apll_c, *dpll_c, *iopll_c);
	if (*apll_c != APLL_CHK || *iopll_c != IOPLL_CHK) {
		printf(" -- error: clocks_emulate: incompatible clock configuration\n");
		goto ce_return;
	}

	*unlock   = wprot0_off;
	// *arm_cc   = ACPU_EMUL; /* ARM at 2.75 GHz */
	*fpga0_cc = PL0_EMUL; /* DRE at 1.25 GHz */
	FILE *fp = fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed", "w+b");
	if (fp != NULL) {char *str = (char *)"137500"; fwrite(str, sizeof(char), sizeof(str), fp); fclose(fp);}
	printf("ARM_CLK_CTRL:%08X DDR_CLK_CTRL:%08X\n", *arm_cc, *ddr_cc);
	printf("FPGA0_CLK_CTRL:%08X FPGA1_CLK_CTRL:%08X\n", *fpga0_cc, *fpga1_cc);
	if (*arm_cc != ACPU_EMUL || *fpga0_cc != PL0_EMUL) {
		printf(" -- error: clocks_emulate: clock configuration not set\n");
		goto ce_return;
	}

	/* TODO: Make two sets of delay calibration values, */
	/* one for the zcu102 and the other for the sidewinder */
	/* The values here likely apply to only one of the boards, */
	/* since the DDR memories run at different frequencies. */
	volatile unsigned int *delay0 = (unsigned int *)dev_smmap("axi_delay",1); /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)dev_smmap("axi_delay",2); /* slot 0, CPU DRAM W, R */
	if (delay0 != MAP_FAILED && delay1 != MAP_FAILED) {
		delay0[2] = 6*(T_SRAM_W+T_TRANS)           - 52; delay0[4] = 6*(T_SRAM_R+T_TRANS)           - 69; /* .16 ns per count */
		delay1[2] = 6*(T_DRAM_W+T_QUEUE_W+T_TRANS) - 52; delay1[4] = 6*(T_DRAM_R+T_QUEUE_R+T_TRANS) - 69;
		printf("Slot 0 - CPU_SRAM_B:%u CPU_SRAM_R:%u CPU_DRAM_B:%u CPU_DRAM_R:%u\n", delay0[2], delay0[4], delay1[2], delay1[4]);
	}
	if (delay0 != MAP_FAILED) dev_munmap((void *)delay0);
	if (delay1 != MAP_FAILED) dev_munmap((void *)delay1);

	volatile unsigned int *delay2 = (unsigned int *)dev_smmap("axi_delay",3); /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)dev_smmap("axi_delay",4); /* slot 1, ACC DRAM W, R */
	if (delay2 != MAP_FAILED && delay3 != MAP_FAILED) {
		delay2[2] = 6*(T_SRAM_W)                   - 48; delay2[4] = 6*(T_SRAM_R)                   - 66;
		delay3[2] = 6*(T_DRAM_W+T_QUEUE_W)         - 48; delay3[4] = 6*(T_DRAM_R+T_QUEUE_R)         - 66;
		printf("Slot 1 - ACC_SRAM_B:%u ACC_SRAM_R:%u ACC_DRAM_B:%u ACC_DRAM_R:%u\n", delay2[2], delay2[4], delay3[2], delay3[4]);
	}
	if (delay2 != MAP_FAILED) dev_munmap((void *)delay2);
	if (delay3 != MAP_FAILED) dev_munmap((void *)delay3);

ce_return:
	if (fpd_slcr != MAP_FAILED) dev_munmap(fpd_slcr);
	if (crf_apb  != MAP_FAILED) dev_munmap(crf_apb);
	if (crl_apb  != MAP_FAILED) dev_munmap(crl_apb);
}

void clocks_normal(void)
{
	char *fpd_slcr = (char *)dev_mmap(FPD_SLCR);
	char *crf_apb  = (char *)dev_mmap(CRF_APB);
	char *crl_apb  = (char *)dev_mmap(CRL_APB);
	if (fpd_slcr == MAP_FAILED || crf_apb == MAP_FAILED || crl_apb == MAP_FAILED) goto cn_return;

	volatile unsigned int *unlock   = (unsigned int *)(fpd_slcr+wprot0);
	volatile unsigned int *arm_cc   = (unsigned int *)(crf_apb+ACPU_CTRL);
	volatile unsigned int *fpga0_cc = (unsigned int *)(crl_apb+PL0_REF_CTRL);

	*unlock   = wprot0_off;
	// *arm_cc   = ACPU_NORM;
	*fpga0_cc = PL0_NORM;
	FILE *fp = fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed", "w+b");
	if (fp != NULL) {char *str = (char *)"1100000"; fwrite(str, sizeof(char), sizeof(str), fp); fclose(fp);}
	if (*arm_cc != ACPU_NORM || *fpga0_cc != PL0_NORM) {
		printf(" -- error: clocks_normal: clock configuration not set\n");
		goto cn_return;
	}

	volatile unsigned int *delay0 = (unsigned int *)dev_smmap("axi_delay",1); /* slot 0, CPU SRAM W, R */
	volatile unsigned int *delay1 = (unsigned int *)dev_smmap("axi_delay",2); /* slot 0, CPU DRAM W, R */
	if (delay0 != MAP_FAILED && delay1 != MAP_FAILED) {
		delay0[2] = 0; delay0[4] = 0; delay1[2] = 0; delay1[4] = 0;
	}
	if (delay0 != MAP_FAILED) dev_munmap((void *)delay0);
	if (delay1 != MAP_FAILED) dev_munmap((void *)delay1);

	volatile unsigned int *delay2 = (unsigned int *)dev_smmap("axi_delay",3); /* slot 1, ACC SRAM W, R */
	volatile unsigned int *delay3 = (unsigned int *)dev_smmap("axi_delay",4); /* slot 1, ACC DRAM W, R */
	if (delay2 != MAP_FAILED && delay3 != MAP_FAILED) {
		delay2[2] = 0; delay2[4] = 0; delay3[2] = 0; delay3[4] = 0;
	}
	if (delay2 != MAP_FAILED) dev_munmap((void *)delay2);
	if (delay3 != MAP_FAILED) dev_munmap((void *)delay3);

cn_return:
	if (fpd_slcr != MAP_FAILED) dev_munmap(fpd_slcr);
	if (crf_apb  != MAP_FAILED) dev_munmap(crf_apb);
	if (crl_apb  != MAP_FAILED) dev_munmap(crl_apb);
}

#endif /* CLOCKS */
