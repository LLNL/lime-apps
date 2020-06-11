/*
 * gdt.c
 *
 *  Created on: May 5, 2020
 *      Author: macaraeg1
 */
#include <stdio.h>
#include "gdt.h"
#include "xparameters.h"

int gdt_data[1024] = {
#include "gdt_data.txt"
};

void config_gdt()
{
	int num_elements;
	int iii;

    int *avd_0_0_b = (int *) (XPAR_DELAY_0_AXI_DELAY_0_BASEADDR + B_OFFSET);
    int *avd_0_0_r = (int *) (XPAR_DELAY_0_AXI_DELAY_0_BASEADDR + R_OFFSET);
    int *avd_0_1_b = (int *) (XPAR_DELAY_0_AXI_DELAY_1_BASEADDR + B_OFFSET);
    int *avd_0_1_r = (int *) (XPAR_DELAY_0_AXI_DELAY_1_BASEADDR + R_OFFSET);
    int *avd_1_0_b = (int *) (XPAR_DELAY_1_AXI_DELAY_0_BASEADDR + B_OFFSET);
    int *avd_1_0_r = (int *) (XPAR_DELAY_1_AXI_DELAY_0_BASEADDR + R_OFFSET);
    int *avd_1_1_b = (int *) (XPAR_DELAY_1_AXI_DELAY_1_BASEADDR + B_OFFSET);
    int *avd_1_1_r = (int *) (XPAR_DELAY_1_AXI_DELAY_1_BASEADDR + R_OFFSET);

    num_elements = sizeof(gdt_data)/sizeof(gdt_data[0]);
    printf("The size of Gaussian Delay Table in bytes is %lu\n", sizeof(gdt_data));
    printf("The number of entries in the Gaussian Delay Table is %d\n", num_elements);

    for (iii = 0; iii < num_elements; ++iii){
        *avd_0_0_b = gdt_data[iii];
        *avd_0_0_r = gdt_data[iii];
        *avd_0_1_b = gdt_data[iii];
        *avd_0_1_r = gdt_data[iii];
        *avd_1_0_b = gdt_data[iii];
        *avd_1_0_r = gdt_data[iii];
        *avd_1_1_b = gdt_data[iii];
        *avd_1_1_r = gdt_data[iii];

        avd_0_0_b++;
        avd_0_0_r++;
        avd_0_1_b++;
        avd_0_1_r++;
        avd_1_0_b++;
        avd_1_0_r++;
        avd_1_1_b++;
        avd_1_1_r++;
	}
}
