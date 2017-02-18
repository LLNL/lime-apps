/*
 * lsu_cmd.c
 *
 *  Created on: May 1, 2014
 *      Author: lloyd23
 */

//      aport header
//       31 bits  0
//     |------------|
//   0 | header:24  | tuser:(go:1 write:1 select:3 length:3) tid:8 tdest:8
//     |------------|

//      hash unit
//      register map
// reg   31 bits  0
//     |------------|
//   0 | status:32  |
//   1 | tlen_lo:32 | currently used as a mask for hash instead of mod
//   2 | tlen_hi:32 |
//   3 | command:30 | seldi:1 seldo:1 seed:4 tdest:4 tid:4 hlen:8 klen:8
//   4 | data_lo:32 |
//   5 | data_hi:32 |
//   6 | hash_lo:32 |
//   7 | hash_hi:32 |
//     |------------|

//      load-store unit
//      register map
// reg   31  bits   0
//     |--------------|
//   0 | status:32    | zero:24 okay:1 slverr:1 decerr:1 interr:1 treqstat:1 tdest:3
//   1 | command:8    | reqstat:1 ignore:4 command:3
//   2 | address:32   |
//   3 | size:30      |
//   4 | inc/index:30 |
//   5 | rep:30       |
//   6 | ignore:32    |
//   7 | ignore:32    |
//     |--------------|

#include "lsu_cmd.h"

typedef struct {
	unsigned int tdest :  8; /* forward route id */
	unsigned int tid   :  8; /* return route id */
	unsigned int tuser : 16; /* command */

//	unsigned int stat  : 32; /* status */
	unsigned int cmd   : 32; /* command */
	unsigned int addr  : 32; /* address */
	unsigned int size  : 32; /* size */
} lsu_move;

typedef struct {
	unsigned int tdest :  8; /* forward route id */
	unsigned int tid   :  8; /* return route id */
	unsigned int tuser : 16; /* command */

//	unsigned int stat  : 32; /* status */
	unsigned int cmd   : 32; /* command */
	unsigned int addr  : 32; /* address */
	unsigned int size  : 32; /* size */
	unsigned int inc   : 32; /* increment */
	unsigned int rep   : 32; /* repetitions */
} lsu_smove;

typedef struct {
	unsigned int tdest :  8; /* forward route id */
	unsigned int tid   :  8; /* return route id */
	unsigned int tuser : 16; /* command */

	unsigned int tag    :  4; /* echos the tag from the command */
	unsigned int interr :  1; /* 1 = internal error */
	unsigned int decerr :  1; /* 1 = address decode error */
	unsigned int slverr :  1; /* 1 = slave asserted error */
	unsigned int okay   :  1; /* 1 = okay response */
	unsigned int        : 24;
} lsu_status;

stream_t *gport;
unsigned gfwd_id;
unsigned gret_id;


void aport_set(stream_t *port)
{
	gport = port;
}

unsigned aport_read(unsigned fwd_id, unsigned ret_id, unsigned sel)
{
	unsigned command[2];
	unsigned response[2];

	/* go=0, write=0, select=sel, length=1, tid=ret_id, tdest=fwd_id */
	command[0] = sel << 19 | 1 << 16 | ret_id << 8 | fwd_id;
	command[1] = 0;
	stream_send(gport, command, sizeof(command), F_BEGP|F_ENDP);
	stream_recv(gport, response, sizeof(response), F_BEGP|F_ENDP);
	return response[1];
}

void aport_write(unsigned fwd_id, unsigned ret_id, unsigned go, unsigned sel, unsigned val)
{
	unsigned command[2];

	/* go=go, write=1, select=sel, length=1, tid=ret_id, tdest=fwd_id */
	command[0] = go << 23 | 1 << 22 | sel << 19 | 1 << 16 | ret_id << 8 | fwd_id;
	command[1] = val;
	stream_send(gport, command, sizeof(command), F_BEGP|F_ENDP);
}

/* The first location of value is reserved for the header. When a new host interface
 * is implemented, then the header can be implemented as a separate variable.
 */

void aport_nread(unsigned fwd_id, unsigned ret_id, unsigned sel, unsigned *val, unsigned n)
{
	/* go=0, write=0, select=sel, length=n, tid=ret_id, tdest=fwd_id */
	val[0] = sel << 19 | n << 16 | ret_id << 8 | fwd_id;
	stream_send(gport, val, 2*sizeof(unsigned), F_BEGP|F_ENDP);
	stream_recv(gport, val, (n+1)*sizeof(unsigned), F_BEGP|F_ENDP);
}

void aport_nwrite(unsigned fwd_id, unsigned ret_id, unsigned go, unsigned sel, unsigned *val, unsigned n)
{
	/* go=go, write=1, select=sel, length=n, tid=ret_id, tdest=fwd_id */
	val[0] = go << 23 | 1 << 22 | sel << 19 | n << 16 | ret_id << 8 | fwd_id;
	stream_send(gport, val, (n+1)*sizeof(unsigned), F_BEGP|F_ENDP);
}

void lsu_setport(stream_t *port, unsigned fwd_pn, unsigned ret_pn)
{
	gport = port;
	gfwd_id = fwd_pn<<1;
	gret_id = ret_pn<<1;
}

void *lsu_memcpy(void *dst, const void *src, size_t n)
{
	lsu_move command;
	lsu_status status;

	command.tid = gret_id;
	command.tuser = 0313; /* go=1, write=1, select=1, length=3 */
//	command.stat = 0;
	command.size = n;

	/* send load command */
	command.cmd = CMD_move; /* reqstat=0, command=move */
	command.tdest = gfwd_id+READ_CH;
	command.addr = (unsigned)src;
	stream_send(gport, &command, sizeof(command), F_BEGP|F_ENDP);

	/* send store command */
	command.cmd = 0x80 | CMD_move; /* reqstat=1, command=move */
	command.tdest = gfwd_id+WRITE_CH;
	command.addr = (unsigned)dst;
	stream_send(gport, &command, sizeof(command), F_BEGP|F_ENDP);

	/* receive store status */
	stream_recv(gport, &status, sizeof(status), F_BEGP|F_ENDP);

	return dst;
}

void *lsu_smemcpy(void *dst, const void *src, size_t block_sz, size_t dst_inc, size_t src_inc, size_t n)
{
	lsu_smove command;
	lsu_status status;

	command.tid = gret_id;
	command.tuser = 0315; /* go=1, write=1, select=1, length=5 */
//	command.stat = 0;
	command.size = block_sz;
	command.rep = n;

	/* send load command */
	command.cmd = CMD_smove; /* reqstat=0, command=smove */
	command.tdest = gfwd_id+READ_CH;
	command.addr = (unsigned)src;
	command.inc = src_inc;
	stream_send(gport, &command, sizeof(command), F_BEGP|F_ENDP);

	/* send store command */
	command.cmd = 0x80 | CMD_smove; /* reqstat=1, command=smove */
	command.tdest = gfwd_id+WRITE_CH;
	command.addr = (unsigned)dst;
	command.inc = dst_inc;
	stream_send(gport, &command, sizeof(command), F_BEGP|F_ENDP);

	/* receive store status */
	stream_recv(gport, &status, sizeof(status), F_BEGP|F_ENDP);

	return dst;
}
