/*
$Id: path.h,v 1.2 2010/05/12 23:19:21 scott Stab $

Description: File name and path manipulation

$Log: path.h,v $
Revision 1.2  2010/05/12 23:19:21  scott
Update comments for version control

Revision 1.1  2009/06/29 20:26:27  scott
Initial revision

*/

#ifndef _PATH_H
#define _PATH_H

#if defined(__cplusplus)
extern "C" {
#endif

void parse_path(int dirc, char *path, char **dir, char **base, char **ext);
char *build_path(int dirc, char *dir, char *base, char *ext);

#if defined(__cplusplus)
}
#endif

#endif /* _PATH_H */
