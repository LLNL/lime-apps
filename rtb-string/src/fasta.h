/*
$Id: fasta.h,v 1.3 2010/05/12 23:11:02 scott Stab $

Description: FASTA file read and write

$Log: fasta.h,v $
Revision 1.3  2010/05/12 23:11:02  scott
Update comments for version control

Revision 1.2  2010/04/23 20:41:49  scott
Add lengths to struct, entry argument, and Fasta_Max_Length

Revision 1.1  2009/06/29 20:17:17  scott
Initial revision

*/

#ifndef _FASTA_H
#define _FASTA_H

#include <stdio.h>

typedef struct {char *hdr, *str; int hdr_len, str_len;} sequence;

#if defined(__cplusplus)
extern "C" {
#endif

int Fasta_Read_Entry(FILE *fp, sequence *entry);
int Fasta_Read_File(FILE *fp, sequence **entry);
void Fasta_Write_File(FILE *fp, sequence *entry, int ecount, int width);
int Fasta_Max_Length(sequence *entry, int ecount);

#if defined (__cplusplus)
}
#endif

#endif /* _FASTA_H */
