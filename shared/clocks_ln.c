/*
 * clocks_ln.c
 *
 *  Created on: Jun 21, 2018
 *      Author: Abhishek Jain
 */

#if defined(CLOCKS)

#include <stdio.h> /* printf, fprintf, perror, fopen, fwrite */
#include <stdlib.h> /* exit */
#include <stdint.h> /* uint64_t */

#include "clocks.h"
#include "devtree.h"

#define DEV_TREE "/sys/firmware/devicetree/base/amba_pl@0"

/* TODO: make clocks_init() and clocks_finish() functions (like monitor_ln.c) */
/* TODO: check number of slots before mmaping & accessing delay units */


void clocks_emulate(void)
{
	int found;
	struct {uint64_t len, addr;} reg;

	/* delay0 */
	found = dev_search(DEV_TREE, "axi_delay", 1, "reg", &reg, sizeof(reg));
	if (found != 1) {
		fprintf(stderr, "Can't find delay0 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay0 = (unsigned int *)dev_mmap(reg.addr);
	if (delay0 == MAP_FAILED) {
		perror("delay0 mmap failed");
		exit(1);
	}
	/* delay1 */
	found = dev_search(DEV_TREE, "axi_delay", 2, "reg", &reg, sizeof(reg));
	if (found != 2) {
		fprintf(stderr, "Can't find delay1 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay1 = (unsigned int *)dev_mmap(reg.addr);
	if (delay1 == MAP_FAILED) {
		perror("delay1 mmap failed");
		exit(1);
	}
	/* delay2 */
	found = dev_search(DEV_TREE, "axi_delay", 3, "reg", &reg, sizeof(reg));
	if (found != 3) {
		fprintf(stderr, "Can't find delay2 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay2 = (unsigned int *)dev_mmap(reg.addr);
	if (delay2 == MAP_FAILED) {
		perror("delay2 mmap failed");
		exit(1);
	}
	/* delay3 */
	found = dev_search(DEV_TREE, "axi_delay", 4, "reg", &reg, sizeof(reg));
	if (found != 4) {
		fprintf(stderr, "Can't find delay3 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay3 = (unsigned int *)dev_mmap(reg.addr);
	if (delay3 == MAP_FAILED) {
		perror("delay3 mmap failed");
		exit(1);
	}

	FILE *fp = fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed", "w+b");
	if (fp == NULL) {
		perror("Can't open frequency scaling file");
		exit(1);
	}

	{char *str = (char *)"137999"; fwrite(str, sizeof(char), sizeof(str), fp);}
	delay0[2] = 6*(T_SRAM_W+T_TRANS)           - 52; delay0[4] = 6*(T_SRAM_R+T_TRANS)           - 69; /* .16 ns per count */
	delay1[2] = 6*(T_DRAM_W+T_QUEUE_W+T_TRANS) - 52; delay1[4] = 6*(T_DRAM_R+T_QUEUE_R+T_TRANS) - 69;
	delay2[2] = 6*(T_SRAM_W)                   - 48; delay2[4] = 6*(T_SRAM_R)                   - 66;
	delay3[2] = 6*(T_DRAM_W+T_QUEUE_W)         - 48; delay3[4] = 6*(T_DRAM_R+T_QUEUE_R)         - 66;

	printf("SRAM_W:%d SRAM_R:%d DRAM_W:%d DRAM_R:%d\nQUEUE_W:%d QUEUE_R:%d TRANS:%d W:%d R:%d\n",
		T_SRAM_W, T_SRAM_R, T_DRAM_W, T_DRAM_R, T_QUEUE_W, T_QUEUE_R, T_TRANS,
		T_DRAM_W+T_QUEUE_W+T_TRANS, T_DRAM_R+T_QUEUE_R+T_TRANS);
	// printf("ARM_PLL_CTRL:%08X DDR_PLL_CTRL:%08X IO_PLL_CTRL:%08X\n", *apll_c, *dpll_c, *iopll_c);
	// printf("ARM_CLK_CTRL:%08X DDR_CLK_CTRL:%08X\n", *arm_cc, *ddr_cc);
	// printf("FPGA0_CLK_CTRL:%08X FPGA1_CLK_CTRL:%08X\n", *fpga0_cc, *fpga1_cc);
	printf("Slot 0 - CPU_SRAM_B:%u CPU_SRAM_R:%u CPU_DRAM_B:%u CPU_DRAM_R:%u\n", delay0[2], delay0[4], delay1[2], delay1[4]);
	printf("Slot 1 - ACC_SRAM_B:%u ACC_SRAM_R:%u ACC_DRAM_B:%u ACC_DRAM_R:%u\n", delay2[2], delay2[4], delay3[2], delay3[4]);

	fclose(fp);
	dev_munmap((void *)delay0);
	dev_munmap((void *)delay1);
	dev_munmap((void *)delay2);
	dev_munmap((void *)delay3);
}

void clocks_normal(void)
{
	int found;
	struct {uint64_t len, addr;} reg;

	/* delay0 */
	found = dev_search(DEV_TREE, "axi_delay", 1, "reg", &reg, sizeof(reg));
	if (found != 1) {
		fprintf(stderr, "Can't find delay0 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay0 = (unsigned int *)dev_mmap(reg.addr);
	if (delay0 == MAP_FAILED) {
		perror("delay0 mmap failed");
		exit(1);
	}
	/* delay1 */
	found = dev_search(DEV_TREE, "axi_delay", 2, "reg", &reg, sizeof(reg));
	if (found != 2) {
		fprintf(stderr, "Can't find delay1 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay1 = (unsigned int *)dev_mmap(reg.addr);
	if (delay1 == MAP_FAILED) {
		perror("delay1 mmap failed");
		exit(1);
	}
	/* delay2 */
	found = dev_search(DEV_TREE, "axi_delay", 3, "reg", &reg, sizeof(reg));
	if (found != 3) {
		fprintf(stderr, "Can't find delay2 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay2 = (unsigned int *)dev_mmap(reg.addr);
	if (delay2 == MAP_FAILED) {
		perror("delay2 mmap failed");
		exit(1);
	}
	/* delay3 */
	found = dev_search(DEV_TREE, "axi_delay", 4, "reg", &reg, sizeof(reg));
	if (found != 4) {
		fprintf(stderr, "Can't find delay3 reg space in device tree.\n");
		exit(1);
	}
	volatile unsigned int *delay3 = (unsigned int *)dev_mmap(reg.addr);
	if (delay3 == MAP_FAILED) {
		perror("delay3 mmap failed");
		exit(1);
	}

	FILE *fp = fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed", "w+b");
	if (fp == NULL) {
		perror("Can't open frequency scaling file");
		exit(1);
	}

	{char *str = (char *)"1099999"; fwrite(str, sizeof(char), sizeof(str), fp);}
	delay0[2] = 0; delay0[4] = 0;
	delay1[2] = 0; delay1[4] = 0;
	delay2[2] = 0; delay2[4] = 0;
	delay3[2] = 0; delay3[4] = 0;

	fclose(fp);
	dev_munmap((void *)delay0);
	dev_munmap((void *)delay1);
	dev_munmap((void *)delay2);
	dev_munmap((void *)delay3);
}

#endif /* CLOCKS */
