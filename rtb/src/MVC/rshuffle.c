/*
$Id: test.c,v 1.2 2010/05/12 23:19:48 scott Stab $

Description: Test file

$Log: test.c,v $
*/

#include <stdio.h> /* printf */
#include <stdlib.h> /* exit, atoi, malloc, realloc, free, rand */
#include <string.h> /* strrchr, strlen, strcmp, memcpy */
#include <ctype.h> /* isspace */
#include <errno.h> /* errno */
//#define NDEBUG 1
//#include <assert.h> /* assert */

#include "foo.h"

#ifndef VERSION
#define VERSION ?.?
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_INT 1
#define DEFAULT_STR "ABC"

#define BFLAG 0x01

#define VFLAG 0x1000

int flags; /* argument flags */
int iarg = DEFAULT_INT; /* int argument */
char *sarg = DEFAULT_STR; /* string argument */


int main(int argc, char *argv[])
{
	int nok = 0;
	char *s;

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s; s++)
			switch (*s) {
			case 'b':
				flags |= BFLAG;
				break;
			case 'i':
				if (isdigit(s[1])) iarg = atoi(s+1);
				else nok = 1;
				s += strlen(s+1);
				break;
			case 's':
				sarg = s+1;
				s += strlen(s+1);
				break;
			case 'v':
				flags |= VFLAG;
				break;
			default:
				nok = 1;
				fprintf(stderr, " -- not an option: %c\n", *s);
				break;
			}

	if (flags & VFLAG) fprintf(stderr, "Version: %s\n", TOSTRING(VERSION));
	if (nok || argc < 1 || (argc > 0 && *argv[0] == '?')) {
		fprintf(stderr, "Usage: test -bv -i<int> -s<str> <in_file> [<out_file>]\n");
		fprintf(stderr, "  -b  boolean argument\n");
		fprintf(stderr, "  -i  integer argument, default: %d\n", DEFAULT_INT);
		fprintf(stderr, "  -s  string argument, default: %s\n", DEFAULT_STR);
		fprintf(stderr, "  -v  version\n");
		exit(EXIT_FAILURE);
	}

	{
		FILE *fin, *fout;

		if ((fin = fopen(argv[0], "r")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		if (argc < 2) {
			fout = stdout;
		} else if ((fout = fopen(argv[1], "w")) == NULL) {
			fprintf(stderr, " -- can't open file: %s\n", argv[1]);
			exit(EXIT_FAILURE);
		}

		/* do something */

		fclose(fin);
		fclose(fout);
	}
	return(EXIT_SUCCESS); /* or EXIT_FAILURE */
}

/* TODO:
 */
