/*
 * dmac_cmd.h
 *
 *  Created on: Feb 26, 2014
 *      Author: lloyd23
 */

#ifndef DMAC_CMD_H_
#define DMAC_CMD_H_

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

extern void dmac_info(void);
extern int dmac_init(unsigned device);
extern void dmac_setunit(unsigned unit);
extern void *dmac_memcpy(void *dst, const void *src, size_t n);
extern void *dmac_smemcpy(void *dst, const void *src, size_t block_sz, size_t dst_inc, size_t src_inc, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* DMAC_CMD_H_ */
