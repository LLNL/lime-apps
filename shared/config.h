/*
 * config.h
 *
 *  Created on: Sep 15, 2014
 *      Author: lloyd23
 */

#ifndef CONFIG_H_
#define CONFIG_H_

//------------------ Configurations ------------------//

// defined(STATS)  : print memory access statistics
// defined(TRACE)  : enable trace capture
// defined(CLOCKS) : enable clock scaling for emulation
// defined(ENTIRE) : flush/invalidate entire cache
// defined(USE_SP) : use scratch pad memory
// defined(USE_OCM) : use on-chip memory (SRAM) for scratch pad, otherwise use special DRAM section
// defined(USE_SD)  : use SD card for trace capture

//----- Accelerator Options -----//
// no defines      : host executes stock algorithm with no accelerator
// defined(DIRECT) : host executes accelerator algorithm with direct calls
// defined(CLIENT) : enable protocol & methods for sending commands to accelerator
// defined(SERVER) : enable protocol & command server for the accelerator
// defined(USE_LSU)  : use the load-store unit
// defined(USE_DMAC) : use the ARM DMA controller
// defined(USE_INDEX) : use index command in LSU

#if defined(DIRECT) || defined(CLIENT) || defined(SERVER) || defined(USE_LSU) || defined(USE_DMAC)
#define USE_ACC 1 // use accelerator or engine code
#endif
#endif /* CONFIG_H_ */
