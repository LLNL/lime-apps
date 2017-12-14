/*
 * dmac_cmd.c
 *
 *  Created on: Feb 26, 2014
 *      Author: lloyd23
 */

#include <stdlib.h> /* exit */
#include <stdio.h> /* printf */
#include <string.h> /* memcpy, memset */
#include "xdmaps.h"
#include "xil_cache.h"
#include "dmac_cmd.h"

//#include "xdmaps_emit.h"
/*
 * Register number for the DMAADDH instruction
 */
#define XDMAPS_ADD_SAR 0x0
#define XDMAPS_ADD_DAR 0x1

/*
 * Register number for the DMAMOV instruction
 */
#define XDMAPS_MOV_SAR 0x0
#define XDMAPS_MOV_CCR 0x1
#define XDMAPS_MOV_DAR 0x2

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Emit DMAC instruction
 */
extern void XDmaPs_Memcpy4(char *Dst, char *Src);
extern int XDmaPs_Instr_DMAADDH(char *DmaProg, unsigned Ra, u32 Imm);
extern int XDmaPs_Instr_DMAEND(char *DmaProg);
extern int XDmaPs_Instr_DMAGO(char *DmaProg, unsigned int Cn, u32 Imm, unsigned int Ns);
extern int XDmaPs_Instr_DMALD(char *DmaProg);
extern int XDmaPs_Instr_DMALP(char *DmaProg, unsigned Lc, unsigned LoopIterations);
extern int XDmaPs_Instr_DMALPEND(char *DmaProg, char *BodyStart, unsigned Lc);
extern int XDmaPs_Instr_DMAMOV(char *DmaProg, unsigned Rd, u32 Imm);
extern int XDmaPs_Instr_DMANOP(char *DmaProg);
extern int XDmaPs_Instr_DMARMB(char *DmaProg);
extern int XDmaPs_Instr_DMASEV(char *DmaProg, unsigned int EventNumber);
extern int XDmaPs_Instr_DMAST(char *DmaProg);
extern int XDmaPs_Instr_DMAWMB(char *DmaProg);

#ifdef __cplusplus
}
#endif


XDmaPs DmaInstance;
unsigned gdma_chan;

void dmac_info(void)
{
	printf("DMA CR0:%lX CR1:%lX CR2:%lX CR3:%lX CR4:%lX CRD:%lX\n",
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CR0_OFFSET),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CR1_OFFSET),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CR2_OFFSET),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CR3_OFFSET),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CR4_OFFSET),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDMAPS_CRDN_OFFSET)
	);
	printf("DMA CS:%lX FT:%lX CP:%lX SA:%lX DA:%lX CC:%lX\n",
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_CSn_OFFSET(gdma_chan)),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_FTCn_OFFSET(gdma_chan)),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_CPCn_OFFSET(gdma_chan)),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_SA_n_OFFSET(gdma_chan)),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_DA_n_OFFSET(gdma_chan)),
		XDmaPs_ReadReg(DmaInstance.Config.BaseAddress, XDmaPs_CC_n_OFFSET(gdma_chan))
	);
	if (DmaInstance.Chans[gdma_chan].DmaCmdToHw != NULL)
		XDmaPs_Print_DmaProg(DmaInstance.Chans[gdma_chan].DmaCmdToHw);
}

int dmac_init(unsigned device)
{
	int Status;
	XDmaPs_Config *DmaCfg;

	DmaCfg = XDmaPs_LookupConfig(device);
	if (DmaCfg == NULL) {
		return XST_FAILURE;
	}

	Status = XDmaPs_CfgInitialize(&DmaInstance, DmaCfg, DmaCfg->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

void dmac_setunit(unsigned unit)
{
	gdma_chan = unit;
}

void *dmac_memcpy(void *dst, const void *src, size_t n)
{
	int Status;
	u32 tmp, burst_sz, burst_len;
	XDmaPs *DmaInst = &DmaInstance;
	XDmaPs_Cmd DmaCmd;

	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));
	if (n < 8) {
		if (n < 4) {
			if (n < 2) {
				burst_sz = 1;
			} else burst_sz = 2;
		} else burst_sz = 4;
	} else burst_sz = 8;
	burst_len = n / burst_sz;
	if (burst_len > 16) burst_len = 16;
	DmaCmd.ChanCtrl.SrcBurstSize = burst_sz;
	DmaCmd.ChanCtrl.SrcBurstLen = burst_len;
	DmaCmd.ChanCtrl.SrcInc = 1;
	DmaCmd.ChanCtrl.DstBurstSize = burst_sz;
	DmaCmd.ChanCtrl.DstBurstLen = burst_len;
	DmaCmd.ChanCtrl.DstInc = 1;
	DmaCmd.BD.SrcAddr = (u32)src;
	DmaCmd.BD.DstAddr = (u32)dst;
	DmaCmd.BD.Length = n;

	Status = XDmaPs_GenDmaProg(DmaInst, gdma_chan, &DmaCmd);
	if (Status != XST_SUCCESS) {
		return NULL;
	}
	DmaInst->Chans[gdma_chan].DmaCmdToHw = &DmaCmd;

	Status = XDmaPs_Exec_DMAGO(DmaInst->Config.BaseAddress, gdma_chan, (u32)DmaCmd.GeneratedDmaProg);
	if (Status != XST_SUCCESS) {
		return NULL;
	}

	while (((tmp = XDmaPs_ReadReg(DmaInst->Config.BaseAddress, XDmaPs_CSn_OFFSET(gdma_chan))) & 0xF) != 0x0) {
		if ((tmp & 0xC) == 0xC) {
			dmac_info();
exit(1);
			dst = NULL;
			break;
		}
	}

	DmaInst->Chans[gdma_chan].DmaCmdToHw = NULL;
	Status = XDmaPs_FreeDmaProg(DmaInst, gdma_chan, &DmaCmd);
	if (Status != XST_SUCCESS) {
		return NULL;
	}

	return dst;
}

void *dmac_smemcpy(void *dst, const void *src, size_t block_sz, size_t dst_inc, size_t src_inc, size_t n)
{
	int Status;
	u32 tmp, burst_sz, burst_len;
	XDmaPs *DmaInst = &DmaInstance;
	XDmaPs_Cmd DmaCmd;

	if (block_sz < 8) {
		if (block_sz < 4) {
			if (block_sz < 2) {
				burst_sz = 1;
			} else burst_sz = 2;
		} else burst_sz = 4;
	} else burst_sz = 8;
	burst_len = block_sz / burst_sz;
	if (burst_len > 16) burst_len = 16;

	memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));
	DmaCmd.ChanCtrl.SrcBurstSize = burst_sz;
	DmaCmd.ChanCtrl.SrcBurstLen = burst_len;
//	DmaCmd.ChanCtrl.SrcInc = 1;
	DmaCmd.ChanCtrl.DstBurstSize = burst_sz;
	DmaCmd.ChanCtrl.DstBurstLen = burst_len;
	DmaCmd.ChanCtrl.DstInc = 1;
//	DmaCmd.BD.SrcAddr = (u32)src;
//	DmaCmd.BD.DstAddr = (u32)dst;
//	DmaCmd.BD.Length = block_sz;
	DmaInst->Chans[gdma_chan].DmaCmdToHw = &DmaCmd;

	{ /* Generate DMAC code: begin */

	char *DmaProgBuf, *DmaProgStart;
	int DmaProgLen;
	u32 CCRValue, loop0, loop1;

	DmaProgBuf = (char *)XDmaPs_BufPool_Allocate(DmaInst->Chans[gdma_chan].ProgBufPool);
	if (DmaProgBuf == NULL) {
		printf("failed to allocate program buffer\n");
		return NULL;
	}

	DmaCmd.GeneratedDmaProg = DmaProgBuf;
	DmaProgStart = DmaProgBuf;

	DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_SAR, (u32)src);
	DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_DAR, (u32)dst);

#if 1
	loop1 = n >> 7;
	loop0 = n & 0x7F;
	if (loop1) {
		char *Label0, *Label1;

		DmaCmd.ChanCtrl.DstBurstSize = 8;
		DmaCmd.ChanCtrl.DstBurstLen = 16;
		CCRValue = XDmaPs_ToCCRValue(&DmaCmd.ChanCtrl);
		DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_CCR, CCRValue);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 1, loop1);
		Label1 = DmaProgBuf;

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 32);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 32);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 32);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 32);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);

		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label1, 1);
	}
#endif
#if 0
	loop1 = n >> 5;
	loop0 = n & 0x1F;
	if (loop1) {
		char *Label0, *Label1;

		DmaCmd.ChanCtrl.DstBurstSize = 8;
		DmaCmd.ChanCtrl.DstBurstLen = 16;
		CCRValue = XDmaPs_ToCCRValue(&DmaCmd.ChanCtrl);
		DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_CCR, CCRValue);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 1, loop1);
		Label1 = DmaProgBuf;

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 32);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);

		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label1, 1);
	}
#endif
#if 0
	loop1 = n >> 8;
	loop0 = n & 0xFF;
	if (loop1) {
		char *Label0, *Label1;

		DmaCmd.ChanCtrl.DstBurstSize = burst_sz;
		DmaCmd.ChanCtrl.DstBurstLen = burst_len;
		CCRValue = XDmaPs_ToCCRValue(&DmaCmd.ChanCtrl);
		DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_CCR, CCRValue);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 1, loop1);
		Label1 = DmaProgBuf;

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, 256);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);

		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label1, 1);
	}
#endif
	if (loop0) {
		char *Label0;

		DmaCmd.ChanCtrl.DstBurstSize = burst_sz;
		DmaCmd.ChanCtrl.DstBurstLen = burst_len;
		CCRValue = XDmaPs_ToCCRValue(&DmaCmd.ChanCtrl);
		DmaProgBuf += XDmaPs_Instr_DMAMOV(DmaProgBuf, XDMAPS_MOV_CCR, CCRValue);

		DmaProgBuf += XDmaPs_Instr_DMALP(DmaProgBuf, 0, loop0);
		Label0 = DmaProgBuf;
		DmaProgBuf += XDmaPs_Instr_DMALD(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAST(DmaProgBuf);
		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_SAR, src_inc);
//		DmaProgBuf += XDmaPs_Instr_DMAADDH(DmaProgBuf, XDMAPS_ADD_DAR, dst_inc);
		DmaProgBuf += XDmaPs_Instr_DMALPEND(DmaProgBuf, Label0, 0);
	}

//	DmaProgBuf += XDmaPs_Instr_DMASEV(DmaProgBuf, gdma_chan);
	DmaProgBuf += XDmaPs_Instr_DMAEND(DmaProgBuf);

	DmaProgLen = DmaProgBuf - DmaProgStart;
	Xil_DCacheFlushRange((INTPTR)DmaProgStart, DmaProgLen);
	DmaCmd.GeneratedDmaProgLength = DmaProgLen;

	} /* Generate DMAC code: end */

	Status = XDmaPs_Exec_DMAGO(DmaInst->Config.BaseAddress, gdma_chan, (u32)DmaCmd.GeneratedDmaProg);
	if (Status != XST_SUCCESS) {
		return NULL;
	}

	while (((tmp = XDmaPs_ReadReg(DmaInst->Config.BaseAddress, XDmaPs_CSn_OFFSET(gdma_chan))) & 0xF) != 0x0) {
		if ((tmp & 0xC) == 0xC) {
			dmac_info();
exit(1);
			dst = NULL;
			break;
		}
	}

	DmaInst->Chans[gdma_chan].DmaCmdToHw = NULL;
	Status = XDmaPs_FreeDmaProg(DmaInst, gdma_chan, &DmaCmd);
	if (Status != XST_SUCCESS) {
		return NULL;
	}

	return dst;
}
