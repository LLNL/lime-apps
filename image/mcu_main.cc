
#define SERVER 1
#include "Decimate2D.hpp"

int main()
{
	Decimate2D dre;

	Xil_DCacheEnable();
	dre.command_server();

	return 0;
}
