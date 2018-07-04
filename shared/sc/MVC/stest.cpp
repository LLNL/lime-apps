
#include <stdlib.h> // exit
#include <stdio.h> // printf

#include "aport.h"
#include "sysinit.h"

MAIN
{
	for (int i = 0; i < argc; i++) {
		printf("arg %d: %s\n", i, argv[i]);
	}
	stream_t port;
	stream_init(&port, 0);

	// ping switch
	flit_t cmd[8];
	flit_t rsp[8];
	cmd[0] = AP_HEAD(0,0,0,2,getID(3),getID(0)); // go,wr,sel,len,tid,tdest
	cmd[1] = 0xAAAAAAAA;
	stream_send(&port, cmd, sizeof(flit_t)*2, F_BEGP|F_ENDP);
	stream_recv(&port, rsp, sizeof(flit_t)*2, F_BEGP|F_ENDP);
	printf("rsp[0]:%08X\n", rsp[0]);
	printf("rsp[1]:%08X\n", rsp[1]);

	// ping lsu
	aport_set(&port);
	aport_write(getID(2)+WRITE_CH, getID(0), 0, 1, 0xBBBBBBBB); // fwd_id, ret_id, go, sel, val
	printf("lsuw[1]:%08X\n", aport_read(getID(2)+WRITE_CH, getID(0), 1));

	cmd[0] = 0; // used by aport_* for header
	cmd[1] = 0xAAAAAAA1;
	cmd[2] = 0xAAAAAAA2;
	cmd[3] = 0xAAAAAAA3;
	cmd[4] = 0xAAAAAAA4;
	cmd[5] = 0xAAAAAAA5;
	cmd[6] = 0xAAAAAAA6;
	cmd[7] = 0xAAAAAAA7;
	aport_nwrite(getID(2)+READ_CH, getID(0), 0, 1, cmd, 7); // fwd_id, ret_id, go, sel, val, n
	aport_nread(getID(2)+READ_CH, getID(0), 1, rsp, 7);
	for (int i = 1; i < 8; i++) printf("lsur[%d]:%08X\n", i, rsp[i]);

	return EXIT_SUCCESS;
}
