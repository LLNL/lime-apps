
typedef __UINT32_TYPE__ index_t;

#define SERVER 1
#include "IndexArray.hpp"

int main()
{
	IndexArray<index_t> dre;

	Xil_DCacheEnable();
	dre.command_server();

	return 0;
}
