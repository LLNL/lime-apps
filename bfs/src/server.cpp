
typedef __UINT32_TYPE__ index_t;

#include "config.h"
#include "IndexArray.hpp"

int main()
{
	IndexArray<index_t> dre;

	Xil_DCacheEnable();
	dre.command_server();

	return 0;
}
