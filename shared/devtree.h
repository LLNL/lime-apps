/*
 * devtree.h
 *
 *  Created on: Jan 3, 2020
 *      Author: lloyd23
 */

#ifndef DEVTREE_H_
#define DEVTREE_H_

#include <stdlib.h> /* size_t */
#include <sys/types.h> /* off_t */
#include <sys/mman.h> /* mmap, MAP_FAILED */


#ifdef __cplusplus
extern "C" {
#endif

extern void *dev_mmap(off_t paddr);
extern int dev_munmap(void *vaddr);
extern void dev_reverse(void *ptr, int size);

/* return data and non-zero value if name and prop are found */
int dev_search(const char *dir, const char *name, int inst, const char *prop, void *data, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* DEVTREE_H_ */
