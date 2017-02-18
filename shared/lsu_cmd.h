/*
 * lsu_cmd.h
 *
 *  Created on: May 1, 2014
 *      Author: lloyd23
 */

#ifndef LSU_CMD_H_
#define LSU_CMD_H_

#include <stddef.h> /* size_t */
#include "stream.h"

#define AP_HEAD(go,wr,sel,len,tid,tdest) \
	((go) << 23 | (wr) << 22 | (sel) << 19 | (len) << 16 | (tid) << 8 | (tdest))

#define READ_CH 0
#define WRITE_CH 1

enum {
	CMD_nop,
	CMD_move,
	CMD_smove,
	CMD_index,
	CMD_flush = 7,
};

#ifdef __cplusplus
extern "C" {
#endif

extern stream_t *gport;
extern unsigned gfwd_id;
extern unsigned gret_id;

extern void aport_set(stream_t *port);
extern unsigned aport_read(unsigned fwd_id, unsigned ret_id, unsigned sel);
extern void aport_write(unsigned fwd_id, unsigned ret_id, unsigned go, unsigned sel, unsigned val);
extern void aport_nread(unsigned fwd_id, unsigned ret_id, unsigned sel, unsigned *val, unsigned n);
extern void aport_nwrite(unsigned fwd_id, unsigned ret_id, unsigned go, unsigned sel, unsigned *val, unsigned n);

extern void lsu_setport(stream_t *port, unsigned fwd_pn, unsigned ret_pn);
extern void *lsu_memcpy(void *dst, const void *src, size_t n);
extern void *lsu_smemcpy(void *dst, const void *src, size_t block_sz, size_t dst_inc, size_t src_inc, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* LSU_CMD_H_ */
