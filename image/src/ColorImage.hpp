
//////////////////////////////////////////////////////////////////////
// ColorImage.hpp: 
//
// A color image class that can read and write PPM files.
// Also defines some useful functions, such as making a color
// from floating point numbers.
//
//////////////////////////////////////////////////////////////////////

#ifndef _COLOR_IMAGE_
#define _COLOR_IMAGE_

//////////////////////////////////////////////////////////////////////

typedef unsigned int pixel_t;

typedef pixel_t* pixel_p;


class ColorImage
{
protected:
	int width, height;
	pixel_p A;

public:
	ColorImage() {A=NULL; height=0; width=0;}
	ColorImage(int w, int h);
	ColorImage(ColorImage &image);
	~ColorImage();

	bool copyImage(ColorImage &image);
		
	bool setSize(int w, int h);
	void clear(pixel_t rgba=0);
	pixel_p getDataArray(void) {return A;}
	int getWidth(void) {return width;}
	int getHeight(void) {return height;}
	int getPixelSz(void) {return sizeof(pixel_t);}

	pixel_t getPixel(int x, int y) {return(A[width*y + x]);}
	void setPixel(int x, int y, pixel_t rgba) {A[width*y + x] = rgba;}

	// Methods to write the image to files
	bool writeToFile(char *fName, bool bin=true, char *headerInfo=NULL);

	// Methods to load the image from a file
	bool loadFromFile(char *fName);
};

//----------------------------------------------------------------------------//

pixel_t makeColor(int r, int g, int b, int a=0);
pixel_t makeColor(float r, float g, float b, float a=0.0f);
void separateColor(pixel_t rgba, int &r, int &g, int &b, int &a);
int maxColorComponentDiff(pixel_t A, pixel_t B);
int colorDiff(pixel_t A, pixel_t B); 
int squaredColorDiff(pixel_t A, pixel_t B);
pixel_t colorAverage(pixel_t A, pixel_t B);
pixel_t colorAverage(pixel_t A, pixel_t B, pixel_t C, pixel_t D);

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000

#endif
