//////////////////////////////////////////////////////////////////////
// ColorImage.cpp
//////////////////////////////////////////////////////////////////////

#include <cstddef> // NULL, size_t
#include <cstdlib> // abs
#include <cstdio> // fopen, fgetc, fread, fwrite, fscanf, fprintf
#include <cctype> // isspace, toupper

#include "ColorImage.hpp"

#include "config.h"
#include "alloc.h" // NEWA, DELETEA
//#include "cache.h" // CACHE_DISPOSE

//------------------ Access ------------------//
#include <cstring> // memcpy
#define memcpy ::memcpy

//------------------ Cache ------------------//
// used for cache coherency between DRE and HOST
#if defined(CLIENT) && defined(ZYNQ)
#include "xil_cache.h"
#define CACHE_DISPOSE(p,n) {/* no dre's */ Xil_DCacheInvalidateRange((INTPTR)p,n);}
#else
#define CACHE_DISPOSE(p,n)
#endif

//--------------------------------------------------------------------------//

static inline void packColor(unsigned char *buf, pixel_t rgba)
{
	buf[0] = (unsigned char)(rgba);     // R
	buf[1] = (unsigned char)(rgba>>8);  // G
	buf[2] = (unsigned char)(rgba>>16); // B
}

static inline pixel_t unpackColor(unsigned char *buf)
{
	return (pixel_t)(buf[0] | buf[1]<<8 | buf[2]<<16);
}

//--------------------------------------------------------------------------//

ColorImage::ColorImage(int w, int h)
{
	if (w <= 0 || w <= 0) {
		width = 0; height = 0; A = NULL;
		return;
	}

	width = w;
	height = h;
	A = NEWA(pixel_t, w * h);
}
//-------------------------------------------------------------------------//

ColorImage::ColorImage(ColorImage &image)
{
	size_t size;

	width = image.width;
	height = image.height;
	A = NEWA(pixel_t, width * height);

	size = width * height * sizeof(pixel_t);
	memcpy(A, image.A, size);
}
//-------------------------------------------------------------------------//

ColorImage::~ColorImage()
{
	if (A != NULL) {
		CACHE_DISPOSE(A, width*height*sizeof(pixel_t))
		DELETEA((pixel_t*)A);
	}
}
//-------------------------------------------------------------------------//

bool ColorImage::copyImage(ColorImage &image)
{
	size_t size;

	if (setSize(image.width, image.height)) {
		size = width * height * sizeof(pixel_t);
		memcpy(A, image.A, size);
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------//

bool ColorImage::setSize(int w, int h)
{
	if (w <= 0 || h <= 0) return false;
	if (w == width && h == height) return true;
	if (A != NULL) {
		CACHE_DISPOSE(A, width*height*sizeof(pixel_t))
		DELETEA((pixel_t*)A);
	}
	width = w;
	height = h;
	A = NEWA(pixel_t, w * h);
	return true;
}
//-------------------------------------------------------------------------//

void ColorImage::clear(pixel_t rgba)
{
	int i, size, end1;

	size = width * height;
	end1 = size - (size & 3);

	i = 0;
	while (i < end1) {
		A[i++] = rgba;
		A[i++] = rgba;
		A[i++] = rgba;
		A[i++] = rgba;
	}

	while (i<size) {
		A[i++] = rgba;
	}
}
//-------------------------------------------------------------------------//

bool ColorImage::writeToFile(char *fName, bool bin, char *headerInfo)
{
	const int magic = (bin) ? 6 : 3;
	const int chan = 3;
	const int maxv = 255;
	FILE *fp;
	char *buf = NULL;
	bool rval = false;

	fp = fopen(fName, "wb");
	if (fp == NULL) goto CI_WTF;
	if (fprintf(fp, "P%d\n", magic) < 0) goto CI_WTF;
	if (headerInfo != NULL)
		if (fprintf(fp, "# %s\n", headerInfo) < 0) goto CI_WTF;
	if (fprintf(fp, "%d %d\n%d\n", width, height, maxv) < 0) goto CI_WTF;
	switch (magic) {
	case 3:
		for (int i = height-1; i >= 0; i--) {
			int count = 0;
			for (int j = 0; j < width; j++) {
				int r, g, b, a;
				separateColor(A[i*width + j], r, g, b, a);
				if (fprintf(fp, "  %3d %3d %3d", r, g, b) < 0) goto CI_WTF;
				if (++count == 4) { // lines shouldn't be longer than 70 chars
					if (fputc('\n', fp) == EOF) goto CI_WTF;
					count = 0;
				}
			}
			if (count && fputc('\n', fp) == EOF) goto CI_WTF;
			if (fputc('\n', fp) == EOF) goto CI_WTF;
		}
		break;
	case 6:
		buf = new char [chan * width];
		for (int i = height-1; i >= 0; i--) {
			unsigned char* ptr = (unsigned char*)buf;
			for (int j = 0; j < width; j++) {
				packColor(ptr, A[i*width + j]);
				ptr += 3;
			}
			if (fwrite(buf, chan, width, fp) != (size_t)width) goto CI_WTF;
		}
		break;
	default:
		goto CI_WTF;
	}

	rval = true;
CI_WTF:
	if (buf != NULL) delete[] buf;
	if (fp != NULL) fclose(fp);
	return rval;	
}
//--------------------------------------------------------------------------//

bool ColorImage::loadFromFile(char *fName)
{
	FILE *fp;
	char *buf = NULL;
	bool rval = false;
	bool comment = false;
	int c;
	int magic, chan;
	int width;
	int height;
	int maxv;
	enum {WIDTH, HEIGHT, MAXV, DATA} state = WIDTH;

	fp = fopen(fName, "rb");
	if (fp == NULL) goto CI_LFF;
	if ((c = fgetc(fp)) == EOF || toupper(c) != 'P') goto CI_LFF;
	if ((c = fgetc(fp)) == EOF || c < '1' || c > '6') goto CI_LFF;
	magic = c - '0';
	chan = (magic == 3 || magic == 6) ? 3 : 1; // channels
	do {
		if ((c = fgetc(fp)) == EOF) goto CI_LFF;
		if (comment) {comment = c != '\n'; continue;}
		if (c == '#') {comment = true; continue;}
		if (isspace(c)) continue;
		ungetc(c, fp);
		switch (state) {
		case WIDTH:
			if (fscanf(fp, "%d", &width) != 1) goto CI_LFF;
			state = HEIGHT;
			break;
		case HEIGHT:
			if (fscanf(fp, "%d", &height) != 1) goto CI_LFF;
			state = MAXV;
			break;
		case MAXV:
			if (fscanf(fp, "%d", &maxv) != 1) goto CI_LFF;
			state = DATA;
			break;
		default:
			break;
		}
	} while (state != DATA);
	if ((c = fgetc(fp)) == EOF || !isspace(c)) goto CI_LFF;

	if (maxv > 255) goto CI_LFF; // implementation restriction
	if (!setSize(width, height)) goto CI_LFF;
	switch (magic) {
	case 3:
		for (int i = height-1; i >= 0; i--) {
			for (int j = 0; j < width; j++) {
				int r, g, b;
				if (fscanf(fp, "%d%d%d", &r,&g,&b) != 3) goto CI_LFF;
				A[i*width + j] = makeColor(r, g, b);
			}
		}
		break;
	case 6:
		buf = new char [chan * width];
		for (int i = height-1; i >= 0; i--) {
			unsigned char* ptr = (unsigned char*)buf;
			if (fread(buf, chan, width, fp) != (size_t)width) goto CI_LFF;
			for (int j = 0; j < width; j++) {
				A[i*width + j] = unpackColor(ptr);
				ptr += 3;
			}
		}
		break;
	default:
		goto CI_LFF;
	}

	rval = true;
CI_LFF:
	if (buf != NULL) delete[] buf;
	if (fp != NULL) fclose(fp);
	return rval;	
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

pixel_t makeColor(int r, int g, int b, int a)
{
	return (pixel_t)(r | g<<8 | b<<16 | a<<24);
}

pixel_t makeColor(float r, float g, float b, float a)
{
	if (r > 1.0f) r = 1.0f;
	if (g > 1.0f) g = 1.0f;
	if (b > 1.0f) b = 1.0f;
	if (a > 1.0f) a = 1.0f;
	return makeColor((int)(r*255.0f), (int)(g*255.0f), (int)(b*255.0f), (int)(a*255.0f));
}

void separateColor(pixel_t rgba, int &r, int &g, int &b, int &a)
{
	r =  rgba      & 0xff;
	g = (rgba>>8)  & 0xff;
	b = (rgba>>16) & 0xff;
	a = (rgba>>24) & 0xff;
}

int colorDiff(pixel_t A, pixel_t B)
{
	return
		(abs)((int)(( A     &RMASK) - ( B     &RMASK))) +
		(abs)((int)(((A>> 8)&RMASK) - ((B>> 8)&RMASK))) +
		(abs)((int)(((A>>16)&RMASK) - ((B>>16)&RMASK))) +
		(abs)((int)(((A>>24)&RMASK) - ((B>>24)&RMASK)));
}

int squaredColorDiff(pixel_t A, pixel_t B)
{
	return
		(( A     &RMASK)-( B     &RMASK)) * (( A     &RMASK)-( B     &RMASK)) +
		(((A>> 8)&RMASK)-((B>> 8)&RMASK)) * (((A>> 8)&RMASK)-((B>> 8)&RMASK)) +
		(((A>>16)&RMASK)-((B>>16)&RMASK)) * (((A>>16)&RMASK)-((B>>16)&RMASK)) +
		(((A>>24)&RMASK)-((B>>24)&RMASK)) * (((A>>24)&RMASK)-((B>>24)&RMASK));
}

pixel_t colorAverage(pixel_t A, pixel_t B)
{
	return ((A&0xfefefefe)>>1) + ((B&0xfefefefe)>>1) + ((A|B) & 0x01010101);
}

pixel_t colorAverage(pixel_t A, pixel_t B, pixel_t C, pixel_t D)
{
	return (colorAverage(colorAverage(A,B), colorAverage(C,D)));
}

//--------------------------------------------------------------------------//
