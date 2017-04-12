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
// defined(USE_LSU)  : use the load-store unit, implies using LSU0 for data movement
// defined(USE_DMAC) : use the ARM DMA controller
// defined(USE_INDEX) : use index command in LSU
// defined(USE_HASH)  : use the hash unit, implies using full pipeline ({MCU0,} LSU1, HSU0, LSU2, flow, & probe)

#if defined(DIRECT) || defined(CLIENT) || defined(SERVER) || defined(USE_LSU) || defined(USE_DMAC) || defined(USE_HASH)
#define USE_ACC 1 // use accelerator or engine code
#endif
#if defined(CLIENT) || defined(SERVER) || defined(USE_LSU) || defined(USE_HASH)
#define USE_STREAM 1 // use stream communication and aport protocols
#endif
#endif /* CONFIG_H_ */
