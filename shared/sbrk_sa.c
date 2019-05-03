
#include <unistd.h> // sbrk
#include <errno.h> // errno, ENOMEM
// #include "xil_printf.h"

extern char _heap_start[];
extern char _heap_end[];

#ifdef __cplusplus
extern "C" {
void *_sbrk(intptr_t increment);
void *sbrk(intptr_t increment);
}
#endif

void *_sbrk(intptr_t increment)
{
	static char *heap_ptr = NULL;
	char *base;

	if (heap_ptr == NULL) heap_ptr = _heap_start;

	base = heap_ptr;
	heap_ptr += increment;

	/* printf causes a hang, probably calls malloc */
	// xil_printf("# heap start:%p end:%p total:%lu used:%lu\r\n# base:%p inc:%lu\r\n",
		// _heap_start, _heap_end, _heap_end - _heap_start, base - _heap_start, base, increment);
	if (heap_ptr <= _heap_end) return base;

	errno = ENOMEM;
	return((char *)-1);
}

void *sbrk(intptr_t increment)
{
	return(_sbrk(increment));
}
