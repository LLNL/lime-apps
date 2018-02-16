
typedef unsigned long long int key_type;
typedef void * value_type;

#include "config.h"
#include "KVstore.hpp"

int main()
{
	KVstore<key_type,value_type> kva;

	Xil_DCacheEnable();
	kva.command_server();

	return 0;
}
