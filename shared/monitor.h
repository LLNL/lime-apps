/*
 * monitor.h
 *
 *  Created on: Sep 16, 2014
 *      Author: lloyd23
 */

#ifndef MONITOR_H_
#define MONITOR_H_

//------------------ Initialization ------------------//

#if (defined(STATS) || defined(TRACE)) && defined(ZYNQ)

#include "xparameters.h" /* XPAR_* */
#include "xaxipmon.h" /* XAxiPmon_* */
#include "xil_printf.h" /* print, xil_printf */

#define MONITOR_INIT monitor_init();

extern XAxiPmon apm;

inline void monitor_init(void)
{
	int apm_status = XST_FAILURE;
	XAxiPmon_Config *apm_config = XAxiPmon_LookupConfig(XPAR_AXIPMON_0_DEVICE_ID);
	if (apm_config != NULL) apm_status = XAxiPmon_CfgInitialize(&apm, apm_config, apm_config->BaseAddress);
	if (apm_status != XST_SUCCESS) print(" -- error: failed to init AXI Performance Monitor\r\n");
}

#else

#define MONITOR_INIT

#endif // (STATS || TRACE) && ZYNQ

//------------------ Statistics ------------------//

// TODO: Want to push, pop, or peek (get) stats for a section without printing
// Wait to print until after benchmark section since printing can modify the stats
// Consider making stat stack (class) external to memory stack, use STL for stack
// Where to locate stat stack? make global, put in DRE class, or put in namespace host?
// Make stat methods in memory stack closer to APM functionality (get counters, clear)
// Incorporate APM into stats macros
// Macros: STATS_PUSH, STATS_POP, STATS_PEEK, STATS_PRINT
// Example Usage:
//   ...setup...
//   STATS_PUSH
//   ...benchmark...
//   STATS_PUSH
//   ...other...
//   STATS_POP   # toss ...other...
//   STATS_PRINT # print ...benchmark... section

#if defined(STATS) && defined(ZYNQ)

#define STATS_START stats_start();
#define STATS_STOP  stats_stop();
#define STATS_PRINT stats_print();

inline void stats_start(void)
{
	XAxiPmon_SetMetrics(&apm, 0, XAPM_METRIC_SET_0, XAPM_METRIC_COUNTER_0); /* Slot 0 Write Transaction Count */
	XAxiPmon_SetMetrics(&apm, 0, XAPM_METRIC_SET_1, XAPM_METRIC_COUNTER_1); /* Slot 0 Read Transaction Count */
	XAxiPmon_SetMetrics(&apm, 0, XAPM_METRIC_SET_2, XAPM_METRIC_COUNTER_2); /* Slot 0 Write Byte Count */
	XAxiPmon_SetMetrics(&apm, 0, XAPM_METRIC_SET_3, XAPM_METRIC_COUNTER_3); /* Slot 0 Read Byte Count */
	XAxiPmon_SetIncrementerRange(&apm, XAPM_INCREMENTER_2, 3, 0); /* counts data beats with low strobes on the 32-bit data channel */
#if defined(USE_ACC)
	XAxiPmon_SetMetrics(&apm, 1, XAPM_METRIC_SET_0, XAPM_METRIC_COUNTER_4); /* Slot 1 Write Transaction Count */
	XAxiPmon_SetMetrics(&apm, 1, XAPM_METRIC_SET_1, XAPM_METRIC_COUNTER_5); /* Slot 1 Read Transaction Count */
	XAxiPmon_SetMetrics(&apm, 1, XAPM_METRIC_SET_2, XAPM_METRIC_COUNTER_6); /* Slot 1 Write Byte Count */
	XAxiPmon_SetMetrics(&apm, 1, XAPM_METRIC_SET_3, XAPM_METRIC_COUNTER_7); /* Slot 1 Read Byte Count */
	XAxiPmon_SetIncrementerRange(&apm, XAPM_INCREMENTER_6, 7, 0); /* counts data beats with low strobes on the 64-bit data channel */
#endif
	XAxiPmon_IntrClear(&apm, XAPM_IXR_ALL_MASK);
	XAxiPmon_ResetMetricCounter(&apm);
	XAxiPmon_EnableMetricsCounter(&apm);
}

inline void stats_stop(void)
{
	XAxiPmon_DisableMetricsCounter(&apm);
#if defined(USE_ACC)
#endif
}

/* Xilinx header has ';' on end of macro */
#undef XAxiPmon_IntrGetStatus
#define XAxiPmon_IntrGetStatus(InstancePtr) XAxiPmon_ReadReg((InstancePtr)->Config.BaseAddress, XAPM_IS_OFFSET)

inline void stats_print(void)
{
	xil_printf("Slot TranW TranR ByteW ByteR StrobeLW\r\n");
	xil_printf("CPU %s%d %s%d %s%d %s%d %d\r\n",
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC0_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_0),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC1_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_1),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC2_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_2),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC3_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_3),
		XAxiPmon_GetIncrementer(&apm, XAPM_INCREMENTER_2)
		);
#if defined(USE_ACC)
	xil_printf("ACC %s%d %s%d %s%d %s%d %d\r\n",
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC4_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_4),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC5_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_5),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC6_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_6),
		(XAxiPmon_IntrGetStatus(&apm) & XAPM_IXR_MC7_OVERFLOW_MASK) ? "*" : "", XAxiPmon_GetMetricCounter(&apm, XAPM_METRIC_COUNTER_7),
		XAxiPmon_GetIncrementer(&apm, XAPM_INCREMENTER_6)
		);
#endif
}

#else

#define STATS_START
#define STATS_STOP
#define STATS_PRINT

#endif // STATS && ZYNQ

//------------------ Trace ------------------//

#if defined(TRACE) && defined(ZYNQ)

#include <unistd.h> /* _exit */
#include "xil_cache.h" /* Xil_DCacheFlush */
#include <xtime_l.h> /* XTime, XTime_GetTime, COUNTS_PER_SECOND */

#define TRACE_START    trace_start();
#define TRACE_STOP     trace_stop();
#define TRACE_CAP      trace_capture();
#define TRACE_EVENT(e) XAxiPmon_SetSwDataReg(&apm, e);

inline void trace_start(void)
{
#if 0
	XAxiPmon_StartEventLog(&apm,
		XAPM_FLAG_WRADDR|
		XAPM_FLAG_FIRSTWR|
		XAPM_FLAG_LASTWR|
		XAPM_FLAG_RESPONSE|
		XAPM_FLAG_RDADDR|
		XAPM_FLAG_FIRSTRD|
		XAPM_FLAG_LASTRD|
		XAPM_FLAG_SWDATA
	);
#else
	XAxiPmon_StartEventLog(&apm,
		XAPM_FLAG_WRADDR|
		XAPM_FLAG_RDADDR|
		XAPM_FLAG_SWDATA
	);
#endif
	XAxiPmon_SetSwDataReg(&apm, 0xAAAAAAAA);
}

inline void trace_stop(void)
{
	int i;
	/* flush FIFO in trace capture device with 16 events */
	for (i = 0; i < 16; i++) XAxiPmon_SetSwDataReg(&apm, 0xDEADBEEF);
	XAxiPmon_StopEventLog(&apm);
}

#define ENTRY_SZ 32   /* 32 bytes, 256 bits */
#define BLK_SZ (1<<17) /* 128 Kbytes */

#if defined(USE_SD)
#include "ff.h"
#if 0
/*
   Save a memory trace to SD card. A file name is prompted for and entered
   from the terminal. Data is copied from a port on the Trace Capture Device
   (tcd) to Zynq on-chip memory in blocks of size BLK_SZ and then written to
   the SD card.
*/
inline void trace_capture(void)
{
	typedef int elem_t;
	volatile elem_t *tcd = (elem_t*)XPAR_AXI_TCD_0_BASEADDR;
	register elem_t tmp;
	unsigned long tot = 0, time;
	unsigned int fn_len;
	XTime start, finish;
	FATFS fs; /* Work area (file system object) for logical drives */
	FIL fdst; /* File object */
	TCHAR fn[80]; /* File name */
	FRESULT fr; /* FatFs function common result code */
	UINT bw; /* File write count */
	//BYTE buffer[BLK_SZ] __attribute__ ((aligned (512))); /* File copy buffer */
	BYTE *buffer = 0x00000000; /* File copy buffer */

	/* get file name */
	print("enter trace file name: ");
	fn_len = 0;
	for (;;) {
		char ch = inbyte();
		if (ch == '\r') {outbyte('\r'); outbyte('\n'); break;}
		if (ch == '\b') {if (fn_len > 0) {fn_len--; outbyte('\b'); outbyte(' '); outbyte('\b');} continue;}
		if (fn_len < sizeof(fn)-1) {fn[fn_len++] = ch; outbyte(ch);}
	}
	fn[fn_len] = '\0';
	if (fn_len == 0) return;
	XTime_GetTime(&start);
	Xil_DCacheDisable();
	/* register work area for each logical drive */
	fr = f_mount(0, &fs);
	if (fr != FR_OK) {
		xil_printf("fr:%d = mount(0, fs)\r\n", fr);
		goto tc;
	}
	/* create destination file */
	fr = f_open(&fdst, fn, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		xil_printf("fr:%d = open(fd, fn:%s, FA_CREATE_ALWAYS | FA_WRITE)\r\n", fr, fn);
		goto tc;
	}
//	Xil_DCacheEnable();
	do { /* grab trace */
		register elem_t *bptr = (elem_t*)buffer;
		unsigned int blen = 0;
		do { /* fill buffer */
			unsigned int i;
			tmp = 0;
			for (i = 0; i < (ENTRY_SZ/sizeof(elem_t)); i++) /* fill entry */
				tmp |= *bptr++ = *tcd;
			blen += ENTRY_SZ;
		} while (tmp && blen < BLK_SZ);
		/* write buffer */
		fr = f_write(&fdst, buffer, blen, &bw);
		if (fr != FR_OK /*|| bw != blen*/) {
			xil_printf("fr:%d = write(fd, buf, br:%d, bw:%d)\r\n", fr, blen, bw);
			goto tc;
		}
		tot += blen;
		if ((tot & ((1U<<20)-1)) == 0U) xil_printf("\rtrace length:0x%x\r", tot);
		if (tot >= (1U<<30)) {
			print(" -- error: trace capture memory exceeded\r\n");
			goto tc;
		}
	} while (tmp);
tc:
//	Xil_DCacheDisable();
	/* close file */
	fr = f_close(&fdst);
	if (fr != FR_OK) {
		xil_printf("fr:%d = close(fd)\r\n", fr);
	}
	/* unregister work area prior to discarding it */
	fr = f_mount(0, NULL);
	if (fr != FR_OK) {
		xil_printf("fr:%d = unmount(0, NULL)\r\n", fr);
	}
	Xil_DCacheEnable();
	XTime_GetTime(&finish);
	time = (finish-start)/COUNTS_PER_SECOND;
	xil_printf("trace length:0x%lx\r\n", tot);
	xil_printf("capture time:%ld sec\r\n", time);
	xil_printf("capture bandwidth:%ld bytes/sec\r\n", tot/time);
}
#else
/*
   Save a memory trace to SD card. A file name is prompted for and entered
   from the terminal. Data is copied from a port on the Trace Capture Device
   (tcd) to the heap in one contiguous block and then written to the SD card.
*/
inline void trace_capture(void)
{
	typedef int elem_t;
	extern char _heap_start[];
	extern char _heap_end[];
	/* use direct DRAM address not alias */
	elem_t *mem_beg = (elem_t*)(((size_t)&_heap_start) & 0x3FFFFFFF);
	elem_t *mem_end = (elem_t*)(((size_t)&_heap_end) & 0x3FFFFFFF);
	volatile elem_t *tcd = (elem_t*)XPAR_AXI_TCD_0_BASEADDR;
	register elem_t *dst = mem_beg;
	register elem_t tmp;
	unsigned int fn_len;
	unsigned long tot = 0, time;
	XTime start, finish;
	FATFS fs; /* Work area (file system object) for logical drives */
	FIL fdst; /* File object */
	TCHAR fn[80]; /* File name */
	FRESULT fr; /* FatFs function common result code */
	UINT bw; /* File write count */

	/* get file name */
	print("enter trace file name: ");
	fn_len = 0;
	for (;;) {
		char ch = inbyte();
		if (ch == '\r') {outbyte('\r'); outbyte('\n'); break;}
		if (ch == '\b') {if (fn_len > 0) {fn_len--; outbyte('\b'); outbyte(' '); outbyte('\b');} continue;}
		if (fn_len < sizeof(fn)-1) {fn[fn_len++] = ch; outbyte(ch);}
	}
	fn[fn_len] = '\0';
	if (fn_len == 0) return;

	/* copy trace to heap */
	XTime_GetTime(&start);
	Xil_DCacheFlush();
	do {
		unsigned int i;
		tmp = 0;
		for (i = 0; i < (ENTRY_SZ/sizeof(elem_t)); i++)
			tmp |= *dst++ = *tcd;
		tot += ENTRY_SZ;
	} while (tmp && dst < mem_end);
	Xil_DCacheFlush();

	Xil_DCacheDisable();
	/* register work area for each logical drive */
	fr = f_mount(0, &fs);
	if (fr != FR_OK) {
		xil_printf("fr:%d = mount(0, fs)\r\n", fr);
		goto tc;
	}
	/* create destination file */
	fr = f_open(&fdst, fn, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		xil_printf("fr:%d = open(fd, fn:%s, FA_CREATE_ALWAYS | FA_WRITE)\r\n", fr, fn);
		goto tc;
	}
	/* write buffer */
	fr = f_write(&fdst, mem_beg, tot, &bw);
	if (fr != FR_OK /*|| bw != blen*/) {
		xil_printf("fr:%d = write(fd, buf, br:%d, bw:%d)\r\n", fr, tot, bw);
		goto tc;
	}
tc:
	/* close file */
	fr = f_close(&fdst);
	if (fr != FR_OK) {
		xil_printf("fr:%d = close(fd)\r\n", fr);
	}
	/* unregister work area prior to discarding it */
	fr = f_mount(0, NULL);
	if (fr != FR_OK) {
		xil_printf("fr:%d = unmount(0, NULL)\r\n", fr);
	}
	Xil_DCacheEnable();
	XTime_GetTime(&finish);
	time = (finish-start)/COUNTS_PER_SECOND;
	if (tmp) print(" -- error: ran out of heap for trace\r\n");
	xil_printf("trace address:0x%x length:0x%x\r\n", (int)mem_beg, tot);
	xil_printf("capture time:%ld sec\r\n", time);
	xil_printf("capture bandwidth:%ld bytes/sec\r\n", tot/time);

	_exit(0); /* avoid destructors, type XMD% stop to terminate */
}
#endif
#else /* USE_SD */
/*
   Save a memory trace to the heap for later download through JTAG with the
   "Dump/Restore Data File" tool. Data is first copied from a port on the
   Trace Capture Device (tcd) to the heap in one contiguous block.
*/
inline void trace_capture(void)
{
	typedef int elem_t;
	extern char _heap_start[];
	extern char _heap_end[];
	/* use direct DRAM address not alias */
	elem_t *mem_beg = (elem_t*)(((size_t)&_heap_start) & 0x3FFFFFFF);
	elem_t *mem_end = (elem_t*)(((size_t)&_heap_end) & 0x3FFFFFFF);
	volatile elem_t *tcd = (elem_t*)XPAR_AXI_TCD_0_BASEADDR;
	elem_t *dst = mem_beg;
	register elem_t tmp;
	unsigned long tot = 0;
	Xil_DCacheFlush();
	do {
		unsigned int i;
		tmp = 0;
		for (i = 0; i < (ENTRY_SZ/sizeof(elem_t)); i++)
			tmp |= *dst++ = *tcd;
		tot += ENTRY_SZ;
	} while (tmp && dst < mem_end);
	Xil_DCacheFlush();
	if (tmp) print(" -- error: ran out of heap for trace\r\n");
	xil_printf("trace address:0x%x length:0x%x\r\n", (int)mem_beg, tot);
	_exit(0); /* avoid destructors, type XMD% stop to terminate */
	/* if needed run "hw_server -s tcp::3121" before "Dump/Restore Data File" tool */
}
#endif /* USE_SD */

#else

#define TRACE_START
#define TRACE_STOP
#define TRACE_CAP
#define TRACE_EVENT(e)

#endif // TRACE && ZYNQ

//------------------ gem5 Work Region ------------------//
// Tie into STATS macro instead of creating yet another one.

#if defined(M5)

#undef STATS_START
#undef STATS_STOP
#undef STATS_PRINT

#include "m5op.h"

#define STATS_START m5_work_begin(0,0);
#define STATS_STOP  m5_work_end(0,0);
#define STATS_PRINT

#endif // M5

#endif /* MONITOR_H_ */
