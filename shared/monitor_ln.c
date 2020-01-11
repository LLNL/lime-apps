/*
 * monitor_ln.c
 *
 *  Created on: Sep 7, 2018
 *      Author: Maya Gokhale
 */

#if defined(STATS) || defined(TRACE)

#include <stdio.h> /* printf, fprintf, perror, fopen, fwrite, fflush */
#include <stdlib.h> /* exit */
#include <string.h> /* strerror */
#include <sys/time.h> /* gettimeofday, struct timeval */
#include <errno.h> /* errno */
#include <stdint.h> /* uint16_t, uint32_t, uint64_t */
#if defined(PAPI)
#include <papi.h>
#endif

#include "monitor.h"
#include "devtree.h"

/*
  Renamed Linux version of xaxipmon to xapm to avoid a name collision
  with the standalone version. Limited baseaddr to file scope (made static).
  Added volatile qualifier to readreg() and writereg().
  See https://github.com/Xilinx/linux-xlnx/tree/<tag>/samples/xilinx_apm
*/
#include "xapm.c"

#define DEV_TREE "/sys/firmware/devicetree/base/amba_pl@0"

#if defined(USE_UIO)
#include <sys/types.h> /* open */
#include <sys/stat.h> /* open */
#include <fcntl.h> /* open */
#include <sys/mman.h> /* mmap */
#include <unistd.h> /* getpagesize */
#define MAP_SIZE getpagesize()
static int uiofd;
#endif

#if defined(PAPI)
int PAPI_events[] = {
	PAPI_L1_DCA, PAPI_L1_DCM, PAPI_L2_DCA, PAPI_L2_DCM,
	PAPI_LD_INS, PAPI_SR_INS, PAPI_TOT_CYC
};
static long long counters[7];
#endif

/* global baseaddr and params are defined in xapm.h */


void monitor_init(void) {
#if defined(USE_UIO)
	char* uiod = "/dev/uio0";

	uiofd = open(uiod, O_RDWR);
	if (uiofd < 1) {
		fprintf(stderr, "Invalid UIO device file:%s.\n", uiod);
		exit(1);
	}
#else
	int found;
#endif

#if defined(USE_UIO)
	baseaddr = (ulong)mmap(0, MAP_SIZE, PROT_READ|PROT_WRITE,
	                       MAP_SHARED, uiofd, 0);
#else
	struct {uint64_t len, addr;} reg;
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "reg", &reg, sizeof(reg));
	if (found != 1) {
		fprintf(stderr, "Can't find APM reg space in device tree.\n");
		exit(1);
	}
	baseaddr = (ulong)dev_mmap(reg.addr);
#endif
	if ((void *)baseaddr == MAP_FAILED) {
		perror("APM reg mmap failed.\n");
		exit(1);
	}

	/* mmap the UIO device */
#if defined(USE_UIO)
	params = (struct xapm_param *)mmap(0, MAP_SIZE, PROT_READ|PROT_WRITE,
	                                   MAP_SHARED, uiofd, MAP_SIZE);
	if (params == MAP_FAILED) {
		perror("APM params mmap failed.\n");
		exit(1);
	}
#else
	params = (struct xapm_param *)malloc(sizeof(struct xapm_param));
	if (params == NULL) {
		fprintf(stderr, "APM params allocation failed.\n");
		exit(1);
	}
	params->mode = XAPM_MODE_ADVANCED;
	params->isr = 0;
	params->is_32bit_filter = FALSE;
	u32 mode;
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,enable-profile", &mode, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM enable-profile not found in device tree.\n"); exit(1);} else if (mode) {params->mode = XAPM_MODE_PROFILE;}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,enable-trace", &mode, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM enable-trace not found in device tree.\n"); exit(1);} else if (mode) {params->mode = XAPM_MODE_TRACE;}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,num-monitor-slots", &params->maxslots, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM num-monitor-slots not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,enable-event-count", &params->eventcnt, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM enable-event-count not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,enable-event-log", &params->eventlog, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM enable-event-log not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,have-sampled-metric-cnt", &params->sampledcnt, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM have-sampled-metric-cnt not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,num-of-counters", &params->numcounters, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM num-of-counters not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,metric-count-width", &params->metricwidth, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM metric-count-width not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,metrics-sample-count-width", &params->sampledwidth, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM metrics-sample-count-width not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,global-count-width", &params->globalcntwidth, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM global-count-width not found in device tree.\n"); exit(1);}
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,metric-count-scale", &params->scalefactor, sizeof(u32));
	if (found != 1) {fprintf(stderr, "APM metric-count-scale not found in device tree.\n"); exit(1);}
#endif

#if 0
	printf("APM params          : %p\n", params);
	printf("APM mode            : %s\n", params->mode == XAPM_MODE_ADVANCED ? "Advanced" : (params->mode == XAPM_MODE_PROFILE ? "Profile" : "Trace"));
	printf("APM maxslots        : %u\n", params->maxslots);
	printf("APM eventcnt        : %u\n", params->eventcnt);
	printf("APM eventlog        : %u\n", params->eventlog);
	printf("APM sampledcnt      : %u\n", params->sampledcnt);
	printf("APM numcounters     : %u\n", params->numcounters);
	printf("APM metricwidth     : %u\n", params->metricwidth);
	printf("APM sampledwidth    : %u\n", params->sampledwidth);
	printf("APM globalcntwidth  : %u\n", params->globalcntwidth);
	printf("APM scalefactor     : %u\n", params->scalefactor);
	printf("APM isr             : 0x%x\n", params->isr);
	printf("APM is_32bit_filter : %s\n", params->is_32bit_filter ? "true" : "false");
#endif

#if defined(PAPI)
	PAPI_library_init(PAPI_VER_CURRENT);
#endif
}

void monitor_finish(void) {
#if defined(USE_UIO)
	close(uiofd);
	munmap((u32 *)baseaddr, MAP_SIZE);
	munmap(params, MAP_SIZE);
#else
	if (baseaddr != 0) dev_munmap((void *)baseaddr);
	if (params != NULL) free(params);
#endif
}

#endif // STATS || TRACE

#if defined(STATS)

void stats_start(void) {
	/* CPU */
	setmetrics(0, XAPM_METRIC_SET_0, XAPM_METRIC_COUNTER_0); /* Slot 0 Write Transaction Count */
	setmetrics(0, XAPM_METRIC_SET_1, XAPM_METRIC_COUNTER_1); /* Slot 0 Read Transaction Count */
	setmetrics(0, XAPM_METRIC_SET_2, XAPM_METRIC_COUNTER_2); /* Slot 0 Write Byte Count */
	setmetrics(0, XAPM_METRIC_SET_3, XAPM_METRIC_COUNTER_3); /* Slot 0 Read Byte Count */
	setincrementerrange(XAPM_INCREMENTER_2, 3, 0); /* counts data beats with low strobes on the 32-bit data channel */
#if defined(USE_ACC)
	/* ACC */
	setmetrics(1, XAPM_METRIC_SET_0, XAPM_METRIC_COUNTER_4); /* Slot 1 Write Transaction Count */
	setmetrics(1, XAPM_METRIC_SET_1, XAPM_METRIC_COUNTER_5); /* Slot 1 Read Transaction Count */
	setmetrics(1, XAPM_METRIC_SET_2, XAPM_METRIC_COUNTER_6); /* Slot 1 Write Byte Count */
	setmetrics(1, XAPM_METRIC_SET_3, XAPM_METRIC_COUNTER_6); /* Slot 1 Read Byte Count */
	setincrementerrange(XAPM_INCREMENTER_6, 7, 0); /* counts data beats with low strobes on the 64-bit data channel */
#endif
	intrclear(XAPM_IXR_ALL_MASK);
	resetmetriccounter();
	enablemetricscounter();

#if defined(PAPI)
	PAPI_start_counters(PAPI_events, 7);
#endif
}

void stats_stop(void) {
	disablemetricscounter();

#if defined(PAPI)
	PAPI_read_counters(counters, 7);
#endif
}

void stats_print(void) {
	printf("Slot TranW TranR ByteW ByteR StrobeLW\n");
	printf("CPU %s%u %s%u %s%u %s%u %u\n",
		(intrhwgetstatus() & XAPM_IXR_MC0_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_0),
		(intrhwgetstatus() & XAPM_IXR_MC1_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_1),
		(intrhwgetstatus() & XAPM_IXR_MC2_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_2),
		(intrhwgetstatus() & XAPM_IXR_MC3_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_3),
		getincrementer(XAPM_INCREMENTER_2)
		);
#if defined(USE_ACC)
	printf("ACC %s%u %s%u %s%u %s%u %u\n",
		(intrhwgetstatus() & XAPM_IXR_MC4_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_4),
		(intrhwgetstatus() & XAPM_IXR_MC5_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_5),
		(intrhwgetstatus() & XAPM_IXR_MC6_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_6),
		(intrhwgetstatus() & XAPM_IXR_MC7_OVERFLOW_MASK) ? "*" : "", getmetriccounter( XAPM_METRIC_COUNTER_7),
		getincrementer(XAPM_INCREMENTER_6)
		);
#endif

#if defined(PAPI)
	printf("%lld L1 accesses, %lld L1 misses (%.2lf%%), %lld L2 accesses and %lld L2 misses (%.2lf%%)\n", counters[0], counters[1], (double)counters[1]*100 / (double)counters[0],counters[2], counters[3], (double)counters[3]*100 / (double)counters[2]);
	printf("%lld Loads, %lld Stores, %lld Total and %lld Clock cycle\n", counters[4], counters[5], counters[4]+counters[5], counters[6]);
#endif
}

#endif // STATS

#if defined(TRACE)

typedef int tcd_t;

// TODO: add to xaxipmon.h?
#define XAPM_FLAG_MIDWR    0x00000080 /* Mid Write Flag */
#define XAPM_FLAG_MIDRD    0x00000100 /* Mid Read Flag */

void trace_start(void) {
	starteventlog(
#if TRACE == _TALL_
		XAPM_FLAG_WRADDR|
		XAPM_FLAG_FIRSTWR|
		XAPM_FLAG_LASTWR|
		XAPM_FLAG_RESPONSE|
		XAPM_FLAG_RDADDR|
		XAPM_FLAG_FIRSTRD|
		XAPM_FLAG_LASTRD|
		XAPM_FLAG_MIDWR|
		XAPM_FLAG_MIDRD|
		XAPM_FLAG_SWDATA
#else
		XAPM_FLAG_WRADDR|
		XAPM_FLAG_RDADDR|
		XAPM_FLAG_SWDATA
#endif
		);
	setswdatareg(0xAAAAAAAA);
}

void trace_stop(void)
{
	int i;
	/* flush FIFO in trace capture device with 16 events */
	for (i = 0; i < 16; i++) setswdatareg(0xDEADBEEF);
	stopeventlog();
}

#define BLK_SZ (1U<<17) /* 128 Kbytes */

void trace_capture(void) {

	register tcd_t tmp;

	uint64_t tot = 0;
	uint64_t TCD_MEM_SZ;
	uint32_t TCD_ENTRY_SZ;
	int found;
	struct timeval start, finish;

	char fn[80]; /* File name */
	char buffer[BLK_SZ]; /* File copy buffer */

	char *s;
	printf("enter trace file name: ");
	if (fgets(fn, sizeof(fn), stdin) == NULL || fn[0] == '\n') return;
	if ((s = strrchr(fn, '\n')) != NULL) *s = '\0';

	uint32_t tdata_width;
	found = dev_search(DEV_TREE, "axi_perf_mon", 1, "xlnx,fifo-axis-tdata-width",
		&tdata_width, sizeof(tdata_width));
	if (found != 1) {
		fprintf(stderr, "Can't find APM tdata width in device tree.\n");
		exit(1);
	}
	for (TCD_ENTRY_SZ = 1; TCD_ENTRY_SZ < tdata_width/8; TCD_ENTRY_SZ <<= 1) ;

	uint32_t mem_addr_width;
	found = dev_search(DEV_TREE, "axi_tcd", 1, "xlnx,mem-addr-width",
		&mem_addr_width, sizeof(mem_addr_width));
	if (found != 1) {
		fprintf(stderr, "Can't find TCD memory size in device tree.\n");
		exit(1);
	}
	TCD_MEM_SZ = (1LLU<<mem_addr_width);

	struct {uint64_t len, addr;} reg;
	found = dev_search(DEV_TREE, "axi_tcd", 1, "reg", &reg, sizeof(reg));
	if (found != 1) {
		fprintf(stderr, "Can't find TCD reg space in device tree.\n");
		exit(1);
	}
	volatile tcd_t *tcd = (tcd_t *)dev_mmap(reg.addr);
	if (tcd == MAP_FAILED) {
		perror("TCD mmap failed");
		exit(1);
	}

	gettimeofday(&start, NULL);

	FILE *tfp = fopen(fn, "w+b");
	if (tfp == NULL) {
		fprintf(stderr, "Can't fopen(%s): %s\n", fn, strerror(errno));
		exit(1);
	}

	*tcd = 0;
	do { /* grab trace */
		register tcd_t *bptr = (tcd_t*)buffer;
		unsigned int blen = 0;
		do { /* fill buffer */
			unsigned int i;
			tmp = 0;
			for (i = 0; i < (TCD_ENTRY_SZ/sizeof(tcd_t)); i++) { /* fill entry */
				tmp |= *bptr++ = *tcd;
			}
			blen += TCD_ENTRY_SZ;
		} while (tmp && blen < BLK_SZ);
		/* write buffer */
		size_t fr = fwrite(buffer, 1, blen, tfp);
		if (fr != blen) {
			fprintf(stderr, "fwrite: bytes requested: %u, actual: %u\n", blen, (unsigned)fr);
			goto tc;
		}
		tot += blen;
		if ((tot & ((1U<<20)-1)) == 0U) {
			printf("\rtrace length: 0x%lx\r", tot); fflush(stdout);
		}
		if (tot >= TCD_MEM_SZ) {
			fprintf(stderr, " -- error: trace capture memory exceeded\n");
			goto tc;
		}
	} while (tmp);

	tc:
	fclose(tfp);

	gettimeofday(&finish, NULL);
	double sec =
		(double) (finish.tv_usec - start.tv_usec) / 1000000 +
		(double) (finish.tv_sec - start.tv_sec);
	printf("trace length: 0x%lx\n", tot);
	printf("capture time: %f sec\n", sec);
	printf("capture bandwidth: %f bytes/sec\n", tot/sec);

	dev_munmap((void *)tcd);
}

void trace_event(int e) {
	setswdatareg(e);
}

#endif // TRACE
