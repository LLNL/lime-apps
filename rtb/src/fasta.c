/*
$Id: fasta.c,v 1.4 2010/09/03 20:14:35 scott Exp $

Description: FASTA file read and write

$Log: fasta.c,v $
Revision 1.4  2010/09/03 20:14:35  scott
Update FASTA_TEST section code.

Revision 1.3  2010/05/12 23:10:58  scott
Update comments for version control

Revision 1.2  2010/04/23 23:40:50  scott
Add lengths to struct, entry argument, and Fasta_Max_Length

Revision 1.1  2009/06/29 20:18:14  scott
Initial revision

*/

#include <stdio.h>
#include <stdlib.h> /* malloc, realloc, free */
#include <ctype.h> /* isspace */

#include "fasta.h"

int Fasta_Read_Entry(FILE *fp, sequence *entry)
{
	int ch;
	size_t nelem, nused;
	char *ptr;

	/* find start of header */
	while ((ch = fgetc(fp)) != EOF && ch != '>') ;
	if (ch == EOF) return(0);

	/* skip any leading space */
	while ((ch = fgetc(fp)) != EOF && (ch == ' ' || ch == '\t')) ;
	if (ch == EOF) return(0);
	ungetc(ch, fp);

	/* save header */
	nelem = 32;
	ptr = malloc(nelem*sizeof(char));
	if (ptr == NULL) return(0);
	nused = 0;
	while ((ch = fgetc(fp)) != EOF && ch != '\n') {
		ptr[nused++] = ch;
		if (nused == nelem) {
			nelem <<= 1;
			ptr = realloc(ptr, nelem*sizeof(char));
			if (ptr == NULL) return(0);
		}
	}
	ptr[nused] = '\0';
	/* trim any header trailing space */
	while (nused > 0 && isspace(ptr[nused-1])) ptr[--nused] = '\0';
	entry->hdr = ptr;
	entry->hdr_len = nused;

	/* save sequence */
	nelem = 128;
	ptr = malloc(nelem*sizeof(char));
	if (ptr == NULL) return(0);
	nused = 0;
	while ((ch = fgetc(fp)) != EOF && ch != '>') {
		if (!isspace(ch)) ptr[nused++] = ch;
		if (nused == nelem) {
			nelem <<= 1;
			ptr = realloc(ptr, nelem*sizeof(char));
			if (ptr == NULL) return(0);
		}
	}
	ptr[nused] = '\0';
	entry->str = ptr;
	entry->str_len = nused;

	if (ch == '>') ungetc(ch, fp);
	return(nused);
}

int Fasta_Read_File(FILE *fp, sequence **entry)
{
	size_t nelem, nused;

	nelem = 16;
	*entry = malloc(nelem*sizeof(sequence));
	if (*entry == NULL) return(0);
	nused = 0;
	while (Fasta_Read_Entry(fp, &(*entry)[nused])) {
		if (++nused == nelem) {
			nelem <<= 1;
			*entry = realloc(*entry, nelem*sizeof(sequence));
			if (*entry == NULL) return(0);
		}
	}
	return(nused);
}

void Fasta_Write_File(FILE *fp, sequence *entry, int ecount, int width)
{
	int i, j;
	char *s;

	for (i = 0; i < ecount; i++) {
		fprintf(fp, ">%s", entry[i].hdr);
		for (j = 0, s = entry[i].str; s[j]; j++) {
			if (j % width == 0) fputc('\n', fp);
			fputc(s[j], fp);
		}
		fputc('\n', fp);
	}
}

int Fasta_Max_Length(sequence *entry, int ecount)
{
	int i, max_len = 0;

	for (i = 0; i < ecount; i++) {
		if (entry[i].str_len > max_len) max_len = entry[i].str_len;
	}
	return(max_len);
}

#ifdef FASTA_TEST

#include <string.h> /* strlen */

#ifndef VERSION
#define VERSION ?.?
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_WIDTH 70

#define VFLAG 0x1000

int flags; /* argument flags */
int warg = DEFAULT_WIDTH; /* width argument */


int main(int argc, char *argv[])
{
	int nok = 0;
	char *s;

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s; s++)
			switch (*s) {
			case 'w':
				if (isdigit(s[1])) warg = atoi(s+1);
				else nok = 1;
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
		fprintf(stderr, "Usage: fasta -v -w<int> <in_file> [<out_file>]\n");
		fprintf(stderr, "  -w  width (columns) of entry, default %d\n", DEFAULT_WIDTH);
		fprintf(stderr, "  -v  version\n");
		exit(EXIT_FAILURE);
	}
	{
		FILE *fin, *fout;
		sequence *entry;
		int ecount, maxseqlen;

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

		ecount = Fasta_Read_File(fin, &entry);
		fclose(fin);

		maxseqlen = Fasta_Max_Length(entry, ecount);
		printf("sequences:%d maxlen:%d\n", ecount, maxseqlen);

		Fasta_Write_File(fout, entry, ecount, warg);
		fclose(fout);
	}
	return(0);
}
#endif
