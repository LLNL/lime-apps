/*
 * gdt.c
 *
 *  Created on: May 5, 2020
 *      Author: macaraeg1
 */
#include <stdio.h>
#include "gdt.h"
#include "xparameters.h"

// NOTE: files with nxx suffix contain gaussians; files with const_xx suffix contain constants
int gdt_data_n0[1024] = {
#include "gdt_data_n0.txt"
};

int gdt_data_n5[1024] = {
#include "gdt_data_n5.txt"
};

int gdt_data_n10[1024] = {
#include "gdt_data_n10.txt"
};

int gdt_data_n15[1024] = {
#include "gdt_data_n15.txt"
};

int gdt_data_n20[1024] = {
#include "gdt_data_n20.txt"
};

int gdt_data_n25[1024] = {
#include "gdt_data_n25.txt"
};

int gdt_data_n50[1024] = {
#include "gdt_data_n50.txt"
};

int gdt_data_n100[1024] = {
#include "gdt_data_n100.txt"
};

int gdt_data_n250[1024] = {
#include "gdt_data_n250.txt"
};

int gdt_data_n500[1024] = {
#include "gdt_data_n500.txt"
};

int gdt_data_n1000[1024] = {
#include "gdt_data_n1000.txt"
};

int gdt_data_n1500[1024] = {
#include "gdt_data_n1500.txt"
};

int gdt_data_n2000[1024] = {
#include "gdt_data_n2000.txt"
};

int gdt_data_n2500[1024] = {
#include "gdt_data_n2500.txt"
};

int gdt_data_n3000[1024] = {
#include "gdt_data_n3000.txt"
};

int gdt_data_n5000[1024] = {
#include "gdt_data_n5000.txt"
};

int gdt_data_n10000[1024] = {
#include "gdt_data_n10000.txt"
};

int gdt_data_n20000[1024] = {
#include "gdt_data_n20000.txt"
};

int gdt_data_const_0[1024] = {
#include "gdt_data_const_0.txt"
};

int gdt_data_const_5[1024] = {
#include "gdt_data_const_5.txt"
};

int gdt_data_const_10[1024] = {
#include "gdt_data_const_10.txt"
};

int gdt_data_const_15[1024] = {
#include "gdt_data_const_15.txt"
};

int gdt_data_const_25[1024] = {
#include "gdt_data_const_25.txt"
};

int gdt_data_const_100[1024] = {
#include "gdt_data_const_100.txt"
};

int gdt_data_const_500[1024] = {
#include "gdt_data_const_500.txt"
};

int gdt_data_const_1000[1024] = {
#include "gdt_data_const_1000.txt"
};

int gdt_data_const_1500[1024] = {
#include "gdt_data_const_1500.txt"
};

int gdt_data_const_10000[1024] = {
#include "gdt_data_const_10000.txt"
};

int gdt_data_n25_50pct[1024] = {    //Dmax=25, 50% duty cycle
#include "gdt_data_n25_50pct.txt"
};

int gdt_data_n25_25pct[1024] = {    //Dmax=25, 25% duty cycle (i.e. 25% Dmax, 75% 0)
#include "gdt_data_n25_25pct.txt"
};

int gdt_data_n25_924zeros[1024] = {  //100 D=25, 924 D=0 - This one passes
#include "gdt_data_n25_924zeros.txt"
};

int gdt_data_n25_904zeros[1024] = {  //120 D=25, 904 D=0 - This one passes
#include "gdt_data_n25_904zeros.txt"
};

int gdt_data_n25_894zeros[1024] = {  //130 D=25, 894 D=0 - This one passes (multiple attempts)
#include "gdt_data_n25_894zeros.txt"
};

int gdt_data_n25_889zeros[1024] = {  //135 D=25, 889 D=0 - This one hung 3x, passed 3x
#include "gdt_data_n25_889zeros.txt"
};

int gdt_data_n25_884zeros[1024] = {  //140 D=25, 884 D=0 - This one hung 3x, passed 0
#include "gdt_data_n25_884zeros.txt"
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

        *avd_0_0_b = gdt_data_n0[iii];  // CPU SRAM write response
        *avd_0_0_r = gdt_data_n0[iii];  // CPU SRAM read response
        *avd_0_1_b = gdt_data_n0[iii];  // CPU DRAM write response
        *avd_0_1_r = gdt_data_n0[iii];  // CPU DRAM read response
        *avd_1_0_b = gdt_data_n0[iii];  // Accererator SRAM write response
        *avd_1_0_r = gdt_data_n0[iii];  // Accererator SRAM read response
        *avd_1_1_b = gdt_data_n0[iii];  // Accererator DRAM write response
        *avd_1_1_r = gdt_data_n0[iii];  // Accererator DRAM read response

/* Constant delays
        *avd_0_0_b = gdt_data_const_0[iii];
        *avd_0_0_r = gdt_data_const_0[iii];
        *avd_0_1_b = gdt_data_const_0[iii];
        *avd_0_1_r = gdt_data_const_0[iii];
        *avd_1_0_b = gdt_data_const_0[iii];
        *avd_1_0_r = gdt_data_const_0[iii];
        *avd_1_1_b = gdt_data_const_0[iii];
        *avd_1_1_r = gdt_data_const_0[iii];
*/

/* For randa
        *avd_0_0_b = gdt_data_n100[iii]; //using non-zero table: FAIL at 250 clocks
        *avd_0_0_r = gdt_data_n0[iii];   //using non-zero table: FAIL at 25 clocks
        *avd_0_1_b = gdt_data_n250[iii]; //using non-zero table: FAIL at 500 clocks
        *avd_0_1_r = gdt_data_n250[iii]; //using non-zero table: FAIL at 500 clocks
        *avd_1_0_b = gdt_data_n100[iii]; //using non-zero table: FAIL at 250 clocks
        *avd_1_0_r = gdt_data_n0[iii];   //using non-zero table: FAIL at 25 clocks
        *avd_1_1_b = gdt_data_n500[iii]; //using non-zero table: FAIL at 1000 clocks
        *avd_1_1_r = gdt_data_n250[iii]; //using non-zero table: FAIL at 500 clocks
*/

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
