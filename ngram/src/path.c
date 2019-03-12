/*
$Id: path.c,v 1.4 2010/09/17 16:19:28 scott Exp $

Description: File name and path manipulation

$Log: path.c,v $
Revision 1.4  2010/09/17 16:19:28  scott
When PATH_TEST is defined, #include <stdio.h>

Revision 1.3  2010/09/03 20:48:18  scott
Fix parse_path issues with dots in dir ('.' '..') and in base (/.foo).
Add a description of parse_path.

Revision 1.2  2010/05/12 23:19:48  scott
Update comments for version control

Revision 1.1  2009/07/06 22:46:23  scott
Initial revision

*/

#include <stdlib.h> /* malloc */
#include <string.h> /* strrchr, strlen, strcmp, memcpy */

/*
parse_path

path: input,  the path to be parsed
dir:  output, any chars before and including the last dirc of path
base: output, any chars after the last dirc of path excluding the ext
ext:  output, any chars after the last dot including the dot

A beginning or ending dot in a base name is not considered part of an
extension. A dot '.' is just like any other filename character. If ext
is NULL, any extension will be included in base. A base name of "." or
".." is handled as a special case and is included in dir.
*/

void parse_path(int dirc, char *path, char **dir, char **base, char **ext)
{
	char *s, *t, *u;

	if (path == NULL) return;
	/* u = point to end of path */
	u = strrchr(path, '\0');
	/* s = point to base name */
	if ((s = strrchr(path, dirc)) == NULL) s = path; else s++;
	/* t = point to extension */
	if (ext == NULL || (t = strrchr(s, '.')) == NULL || t == s || t+1 == u) t = u;
	/* include "." or ".." in dir */
	if (strcmp(s, ".") == 0 || strcmp(s, "..") == 0) s = u;

	if (dir != NULL) {
		*dir = (char *)malloc(s-path+1);
		memcpy(*dir, path, s-path);
		(*dir)[s-path] = '\0';
	}
	if (base != NULL) {
		*base = (char *)malloc(t-s+1);
		memcpy(*base, s, t-s);
		(*base)[t-s] = '\0';
	}
	if (ext != NULL) {
		*ext = (char *)malloc(u-t+1);
		memcpy(*ext, t, u-t);
		(*ext)[u-t] = '\0';
	}
}

char *build_path(int dirc, char *dir, char *base, char *ext)
{
	size_t dlen, blen, elen;
	char *s, *t;

	if (dir != NULL) dlen = strlen(dir); else dlen = 0;
	if (base != NULL) blen = strlen(base); else blen = 0;
	if (ext != NULL) elen = strlen(ext); else elen = 0;
	s = t = (char *)malloc(dlen+blen+elen+3); /* add space for /, ., & \0 */
	memcpy(t, dir, dlen); t += dlen;
	if (dlen && dir[dlen-1] != dirc) *t++ = dirc;
	memcpy(t, base, blen); t += blen;
	if (elen && ext[0] != '.') *t++ = '.';
	memcpy(t, ext, elen); t += elen;
	*t = '\0';
	return(s);
}

#ifdef PATH_TEST

#include <stdio.h>

#define DIRC '/' /* directory separator character */


void test_parse_path(int dirc)
{
	char *dir, *base, *ext;
	char *path;

	path = "/usr/src/";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "/usr/src/foo.txt";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "/usr/src/foo.txt";
	parse_path(dirc, path, NULL, &base, NULL);
	printf("path:'%s'\nparse_path(dirc, path, NULL, &base, NULL)\ncomp:'%s' '%s' '%s'\n\n", path, NULL, base, NULL);

	path = "/usr/src/foo.txt";
	parse_path(dirc, path, &dir, NULL, &ext);
	printf("path:'%s'\nparse_path(dirc, path, &dir, NULL, &ext)\ncomp:'%s' '%s' '%s'\n\n", path, dir, NULL, ext);

	path = "/usr/src/foo";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "../src/foo";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "..";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "./";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "/..";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "usr.dir/";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "foo.txt";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "/usr/src/.foo";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = ".foo";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "foo.";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);

	path = "foo";
	parse_path(dirc, path, &dir, &base, &ext);
	printf("path:'%s'\ncomp:'%s' '%s' '%s'\n\n", path, dir, base, ext);
}

void test_build_path(int dirc)
{
	char *dir, *base, *ext;
	char *path;

	dir = "/usr/src/";
	base = "foo";
	ext =  ".txt";
	path = build_path(dirc, dir, base, ext);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", dir, base, ext, path);

	dir = "/usr/src";
	base = "foo";
	ext =  "txt";
	path = build_path(dirc, dir, base, ext);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", dir, base, ext, path);

	dir = "";
	base = "foo";
	ext =  ".txt";
	path = build_path(dirc, dir, base, ext);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", dir, base, ext, path);

	dir = "";
	base = "foo";
	ext =  ".txt";
	path = build_path(dirc, NULL, base, ext);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", NULL, base, ext, path);

	dir = "/usr/src/";
	base = "foo";
	ext =  "";
	path = build_path(dirc, dir, base, ext);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", dir, base, ext, path);

	dir = "/usr/src/";
	base = "foo";
	ext =  "";
	path = build_path(dirc, dir, base, NULL);
	printf("comp:'%s' '%s' '%s'\npath:'%s'\n\n", dir, base, NULL, path);
}


int main(int argc, char *argv[])
{
	test_parse_path(DIRC);
	test_build_path(DIRC);
	return(0);
}
#endif
