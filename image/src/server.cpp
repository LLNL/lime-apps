
#include "config.h"
#include "Decimate2D.hpp"

int main()
{
	Decimate2D dre;

	Xil_DCacheEnable();
	dre.command_server();

	return 0;
}
