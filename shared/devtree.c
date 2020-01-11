/*
 * devtree.c
 *
 *  Created on: Jan 3, 2020
 *      Author: lloyd23
 */

#include <stdio.h> /* fopen, fread */
#include <string.h> /* strcmp, strcpy, memcpy, strerror */
#include <fcntl.h> /* open */
#include <dirent.h> /* opendir, readdir, closedir */
#include <sys/stat.h> /* stat */
#include <errno.h> /* errno */
#include <unistd.h> /* sysconf */

#include "devtree.h"

#define eq(a, b) (strcmp((a), (b)) == 0)
#define NAME_SZ 64

static int mfd;
static off_t page_size;


void *dev_mmap(off_t paddr)
{
	off_t page_addr;
	char *name = (char *)"/dev/mem";

	if (page_size == 0) {
		page_size = sysconf(_SC_PAGESIZE);

		/* Open device */
		mfd = open(name, O_RDWR);
		if (mfd < 1) {
			fprintf(stderr, "Can't open(%s): %s\n", name, strerror(errno));
			return MAP_FAILED;
		}
	}

	/* Map the device into memory */
	page_addr = (paddr & (~(page_size-1)));
	return mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, page_addr);
}

int dev_munmap(void *vaddr)
{
	return munmap(vaddr, page_size);
}

void dev_reverse(void *ptr, int size)
{
	int i;
	char tmp;
	char *data = (char *)ptr;

	for (i = 0; i < size/2; i++) {
		tmp = data[i];
		data[i] = data[size-i-1];
		data[size-i-1] = tmp;
	}
}

/* return data and non-zero value if name and prop are found */

int dev_search(const char *dir, const char *name, int inst, const char *prop, void *data, size_t n)
{
	DIR *ds;
	struct dirent *de;
	struct stat st;
	FILE *pfile;
	char *path;
	size_t plen;
	size_t elem;
	int ret = 0;

	// printf("dir:%s\n", dir);
	if (strlen(prop) > NAME_SZ) {
		fprintf(stderr, "property name too long: %s\n", prop);
		return ret;
	}

	plen = strlen(dir);
	path = (char *)malloc(plen+NAME_SZ+2);
	if (path == NULL) {
		fprintf(stderr, "Can't malloc(%lu): %s\n",
			(long)plen+NAME_SZ+2, strerror(errno));
		return ret;
	}
	memcpy(path, dir, plen);
	path[plen++] = '/';

	strcpy(path+plen, "name");
	if ((pfile = fopen(path, "rb")) != NULL) {
		char tmp[NAME_SZ];
		elem = fread(tmp, 1, sizeof(tmp), pfile);
		fclose(pfile);
		// printf("  name:%s\n", tmp);
		if (elem > 0 && elem < sizeof(tmp) && eq(name, tmp)) {
			strcpy(path+plen, prop);
			if ((pfile = fopen(path, "rb")) != NULL) {
				elem = fread(data, 1, n, pfile);
				fclose(pfile);
				if (elem == n) {
					dev_reverse(data, n);
					free(path);
					return 1;
				}
			}
		}
	}

	ds = opendir(dir);
	if (ds == NULL) {
		fprintf(stderr, "Can't opendir(%s): %s\n", dir, strerror(errno));
		free(path);
		return ret;
	}

	while (ret < inst && (de = readdir(ds)) != NULL) {
		if (eq(de->d_name, ".") || eq(de->d_name, "..") ||
		    eq(de->d_name, "name") || eq(de->d_name, prop)) continue;

		if (strlen(de->d_name) > NAME_SZ) {
			fprintf(stderr, "Name too long: %s\n", de->d_name);
			free(path); closedir(ds);
			return ret;
		}
		strcpy(path+plen, de->d_name);

		if (stat(path, &st) < 0) {
			fprintf(stderr, "Can't stat(%s): %s\n", path, strerror(errno));
			free(path); closedir(ds);
			return ret;
		}

		if (S_ISDIR(st.st_mode)) {
			ret += dev_search(path, name, inst, prop, data, n);
		}
	}

	free(path);
	closedir(ds);
	return ret;
}
