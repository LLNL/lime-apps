/*
 * stream.h
 *
 *  Created on: Jan 13, 2014
 *      Author: lloyd23
 */

#ifndef STREAM_H_
#define STREAM_H_

#include <stddef.h> /* size_t */

#define F_BEGP 0x01
#define F_ENDP 0x02
#define F_NOWA 0x04

#ifdef __microblaze__

typedef struct {
} stream_t;

#else /* __microblaze__ */

#include "xllfifo.h"

typedef struct {
	XLlFifo Instance;
	u32 TransmitLength;
	u32 ReceiveLength;
} stream_t;

#ifdef __cplusplus
extern "C" {
#endif

/* These were left out of xllfifo.h */
extern int XLlFifo_iWrite_Aligned(XLlFifo *InstancePtr, void *BufPtr, unsigned WordCount);
extern int XLlFifo_iRead_Aligned(XLlFifo *InstancePtr, void *BufPtr, unsigned WordCount);

#ifdef __cplusplus
}
#endif

#endif /* __microblaze__ */


#ifdef __cplusplus
extern "C" {
#endif

extern int stream_init(stream_t *port, unsigned id);
extern int stream_send(stream_t *port, void *buf, size_t size, unsigned flags);
extern int stream_recv(stream_t *port, void *buf, size_t size, unsigned flags);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_H_ */
