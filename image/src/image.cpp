
#include <cstdlib> // rand, exit, abs, system
#include <cstdio> // printf
#include <cstring> // strncmp, memset
#include <unistd.h> // getopt, optarg
#include <algorithm> // min
#include <iostream> // cout, endl
#if defined(_OPENMP)
#include <omp.h>
#endif
using namespace std;

#include "ColorImage.hpp"

// Example main arguments
// #define MARGS "-d1 -v15 -w12800 -h6400 pat pat"
// #define MARGS "-d4:16 -v2:12 -b -l1000"
// #define MARGS "-d4:16 -w10800 -h5400 pat"
// #define MARGS "-d4:16 -v12:15 -w16000 -h8000 pat pat"
// #define MARGS "-d16 -v15 -w16000 -h8000 pat pat" *

#include "lime.h"

#define DEFAULT_LOOP_COUNT 10000
#define DEFAULT_DECIMATE_MIN 4
#define DEFAULT_DECIMATE_MAX 4
#define DEFAULT_BLOCK_MIN 12 // log 2 size
#define DEFAULT_BLOCK_MAX 12 // log 2 size
#define DEFAULT_IMAGE_W 512
#define DEFAULT_IMAGE_H 512

#define ALIGN2(n,s) ((n) & ~((s)-1))

#define restrict __restrict__

typedef unsigned char uchar_t;
typedef unsigned long ulong_t;

typedef uchar_t* uchar_p;

// TODO: find a better place for these globals

tick_t start, finish;
tick_t t0, t1, t2, t3, t4, t5, t6, t7;
unsigned long long tsetup, treorg, toper, tcache;

#if defined(USE_ACC)

#include "Decimate2D.hpp"

#ifdef ENTIRE
#define fill_X fill_E
#else
#define fill_X fill_R
#endif

Decimate2D dre1; // Data Reorganization Engine
Decimate2D dre2;

#endif /* USE_ACC */


//------------------ Support ------------------//

#define system(command)

/* initialize the reference array with a pattern */

void pattern(void *ref, size_t ref_w, size_t ref_h, size_t elem_sz, size_t decimate)
{
	size_t x, y;
	char *ptr = (char *)ref;

	for (y = 0; y < ref_h; y++) {
		for (x = 0; x < ref_w; x++) {
			char tmp = (((y%decimate+1)&0xF)<<4) | ((x%decimate+1)&0xF);
			memset(ptr, tmp, elem_sz);
			ptr += elem_sz;
		}
	}
}

/* check buffer for correct decimated pattern */

bool check(void *buf, size_t buf_sz)
{
	size_t i, ecount = 0;
	unsigned char *ptr = (unsigned char *)buf;

	fflush(stdout);
	for (i = 0; i < buf_sz; i++) {
		int tmp = ptr[i];
		if (tmp != 0x11) {
			fprintf(stderr, " -- error: offset:%lu value:%02x\n", (ulong_t)i, tmp);
			ecount++;
			if (ecount == 5) break;
		}
	}
	return ecount != 0;
}

//------------------  ------------------//


int MAIN(int argc, char *argv[])
{
	/* get arguments */
	int opt;
	bool nok = false;
	bool bflag = false;
	int loop_count = DEFAULT_LOOP_COUNT;
	int decimate_min = DEFAULT_DECIMATE_MIN;
	int decimate_max = DEFAULT_DECIMATE_MAX;
	size_t block_min = 1U<<DEFAULT_BLOCK_MIN;
	size_t block_max = 1U<<DEFAULT_BLOCK_MAX;
	int image_w = DEFAULT_IMAGE_W;
	int image_h = DEFAULT_IMAGE_H;
	char *oname = NULL;
	bool sflag = false;

#if defined(USE_ACC)
	dre1.wait(); // wait for DRE initialization
	dre2.wait();
#endif
	while ((opt = getopt(argc, argv, "bd:l:v:w:h:o:s")) != -1) {
		switch (opt) {
		case 'b':
			bflag = true;
			break;
		case 's':
			sflag = true;
			break;
		case 'd':
			switch (sscanf(optarg, "%u:%u", &decimate_min, &decimate_max)) {
			case 1: decimate_max = decimate_min; break;
			case 2: break;
			default: fprintf(stderr, "error with decimate option.\n"); nok = true; break;
			}
			break;
		case 'l':
			loop_count = atoi(optarg);
			break;
		case 'v':
		{
			unsigned int tmp_min, tmp_max;
			switch (sscanf(optarg, "%u:%u", &tmp_min, &tmp_max)) {
			case 1: block_min = (1U<<tmp_min); block_max = block_min; break;
			case 2: block_min = (1U<<tmp_min); block_max = (1U<<tmp_max); break;
			default: fprintf(stderr, "error with block size option.\n"); nok = true; break;
			}
			break;
		}
		case 'w':
			image_w = atoi(optarg);
			break;
		case 'h':
			image_h = atoi(optarg);
			break;
		case 'o':
			oname = optarg;
			break;
		default: /* '?' */
			nok = true;
		}
	}
	nok |= !bflag && !(argc-optind);
	if (decimate_min < 1 || decimate_max < 1) {
		fprintf(stderr, "decimate factor must be 1 or greater.\n");
		nok = true;
	}
	if (decimate_max < decimate_min) {
		int tmp = decimate_min; decimate_min = decimate_max; decimate_max = tmp;
	}
	if (block_min < 4 || block_max < 4) {
		fprintf(stderr, "view buffer block size must be 4 or greater.\n");
		nok = true;
	}
	if (block_max < block_min) {
		size_t tmp = block_min; block_min = block_max; block_max = tmp;
	}
	if (nok) {
		fprintf(stderr, "Usage: image -b -s -d<int>[:<int>] -l<int> -v<int>[:<int>] -w<int> -h<int> -o<out_file_ppm> <in_file_ppm>...\n");
		fprintf(stderr, "  -b  block test\n");
		fprintf(stderr, "  -s  show output image with pamx\n");
		fprintf(stderr, "  -d  decimate, default: %d\n", DEFAULT_DECIMATE_MIN);
		fprintf(stderr, "  -l  loop count for test, default: %d\n", DEFAULT_LOOP_COUNT);
		fprintf(stderr, "  -v  view buffer block size (2^n), default: n=%d\n", DEFAULT_BLOCK_MIN);
		fprintf(stderr, "  -w  width of image, default: %d\n", DEFAULT_IMAGE_W);
		fprintf(stderr, "  -h  height of image, default: %d\n", DEFAULT_IMAGE_H);
		fprintf(stderr, "  -o  output PPM image, default: NULL\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "One image argument: decimate the specified image.\n");
		fprintf(stderr, "Two image arguments: decimate the specified images\n");
		fprintf(stderr, "and then take the absolute difference between each\n");
		fprintf(stderr, "pixel value. If \"pat\" is used for an image argument,\n");
		fprintf(stderr, "an image will be auto generated with a pattern.\n");
		return EXIT_FAILURE;
	}
#if defined(_OPENMP)
	// to control the number of threads use: export OMP_NUM_THREADS=N
	printf("threads: %d\n", omp_get_max_threads());
#endif


	/* * * * * * * * * * block test * * * * * * * * * */

	if (bflag) {
		int i;
		typedef struct {int x,y;} point_t;
		typedef point_t* point_p;
		const int ref_width = image_w;
		const int ref_height = image_h;
		typedef unsigned int element_t;
		typedef element_t* element_p;
#ifdef USE_SP
		if (block_max > SP_SIZE) block_max = SP_SIZE;
#endif
		point_p pt = new point_t [loop_count]; // list of points in decimated view
		element_p ref = NEWA(element_t, ref_width*ref_height); // reference array
		uchar_p block = SP_NALLOC(uchar_t, block_max); // view block

		for (int decimate = decimate_min; decimate <= decimate_max; decimate <<= 1) {
		printf("decimate: %d\n", decimate);

		/* reference initialization */
		pattern(ref, ref_width, ref_height, sizeof(element_t), decimate);

		/* select random points in decimated view */
		for (i = 0; i < loop_count; i++) {
			pt[i].x = rand() % (ref_width/decimate);
			pt[i].y = rand() % (ref_height/decimate);
		}

		/* decimate portion of reference image into smaller view buffer around select points */
#if defined(USE_ACC)
		CACHE_SEND(dre1, ref, ref_width*ref_height*sizeof(element_t))
		dre1.setup(ref, ref_width, ref_height, sizeof(element_t), decimate);
		dre1.wait();
		size_t view_end = (ref_width/decimate) * (ref_height/decimate) * sizeof(element_t);
		for (size_t block_sz = block_min; block_sz <= block_max; block_sz <<= 1) {
			bool err = false;
			printf("block size: %lu", (ulong_t)block_sz); fflush(stdout);

			/* check decimation */
			for (i = 0; i < loop_count && !err; i++) {
				size_t offset = ALIGN2(dre1.view_offset(pt[i].x, pt[i].y), block_sz);
				size_t size = min(block_sz, view_end-offset);
				memset(block, 0, size);
				CACHE_SEND(dre1, block, size)
				dre1.fill(block, size, offset);
				dre1.wait();
				CACHE_RECV(dre1, block, size)
				err = check(block, size);
			}
			if (err) break;

			/* time decimation */
			printf(", "); fflush(stdout);
			CLOCKS_EMULATE
			CACHE_BARRIER(dre1)
			STATS_START

			/* assume input data is in memory and not cached (flushed and invalidated) */
			tget(start);

			for (i = 0; i < loop_count; i++) {
				size_t offset = ALIGN2(dre1.view_offset(pt[i].x, pt[i].y), block_sz);
				size_t size = min(block_sz, view_end-offset);
				dre1.fill(block, size, offset);
				dre1.wait();
				CACHE_RECV(dre1, block, size)
			}

			/* assume output data is in memory (flushed) */
			tget(finish);

			CACHE_BARRIER(dre1)
			STATS_STOP
			CLOCKS_NORMAL
			printf("time: %.3f usec\n", tesec(finish, start)/loop_count*1000000);
			STATS_PRINT
		} // for block_sz...
#else /* USE_ACC */
		/* * * * * * * * * * * */
		/* TODO: Stock version */
		/* * * * * * * * * * * */
#endif /* USE_ACC */

		} // for decimate...

		CACHE_DISPOSE(dre1, block, block_max*sizeof(uchar_t));
		CACHE_DISPOSE(dre1, ref, ref_width*ref_height*sizeof(element_t));
		SP_NFREE((uchar_t*)block);
		DELETEA((element_t*)ref);
		delete[] pt;
	}

	/* * * * * * * * * * one image argument * * * * * * * * * */

	if (argc-optind == 1) {
		ColorImage ref; // reference image
		bool cflag = false;

		for (int decimate = decimate_min; decimate <= decimate_max; decimate <<= 1) {
		printf("decimate: %d\n", decimate);

		/* reference initialization */
		tget(start);
		if (strncmp(argv[optind], "pat", 3) == 0) {
			if (!ref.setSize(image_w, image_h)) return EXIT_FAILURE;
			pattern(ref.getDataArray(), image_w, image_h, ref.getPixelSz(), decimate);
			cflag = true;
		} else if (!ref.loadFromFile(argv[optind])) {
			fprintf(stderr, " -- error loading image: %s\n", argv[optind]);
			return EXIT_FAILURE;
		}
		tget(finish);
		printf("ref w:%d h:%d element:%d init-time:%f sec\n",
			ref.getWidth(), ref.getHeight(), ref.getPixelSz(),
			tsec(finish, start));

		/* decimated image and alias */
		ColorImage view(ref.getWidth()/decimate, ref.getHeight()/decimate);
		void *buf = view.getDataArray();
		size_t buf_sz = view.getWidth()*view.getHeight()*ref.getPixelSz();
		printf("view w:%d h:%d\n", view.getWidth(), view.getHeight());
		printf("view size: %lu\n", (ulong_t)buf_sz);

		CLOCKS_EMULATE
		CACHE_BARRIER(dre1)
		STATS_START

		/* decimate entire reference image into view buffer */
		/* assume input data is in memory and not cached (flushed and invalidated) */
		tget(start);

#if defined(USE_ACC)
		dre1.setup(ref.getDataArray(), ref.getWidth(), ref.getHeight(),
			ref.getPixelSz(), decimate);
		dre1.wait();
		dre1.fill(buf, buf_sz, 0);
		dre1.wait();
		CACHE_RECV(dre1, buf, buf_sz)
#else /* USE_ACC */
		/* * * * * * * * * * * */
		/* TODO: Stock version */
		/* * * * * * * * * * * */
#endif /* USE_ACC */

		/* assume output data is in memory (flushed) */
		tget(finish);

		CACHE_BARRIER(dre1)
		STATS_STOP
		CLOCKS_NORMAL
		printf("decimate time: %f sec\n", tesec(finish, start));
		STATS_PRINT

		if (cflag) check(buf, buf_sz);

		/* output decimated copy of reference image */
		if (oname != NULL) {
			char fname[240+16];
			sprintf(fname, "%.240s_%d", oname, decimate);
			view.writeToFile(fname);
			if (sflag) {
				char cmd[16+256];
				sprintf(cmd, "pamx %.256s&", fname);
				system(cmd);
			}
		}
		} // for decimate...

		/* ColorImage ref, view destructors call CACHE_DISPOSE */
	}

	/* * * * * * * * * * two image arguments * * * * * * * * * */

	if (argc-optind == 2) {
		ColorImage ref1, ref2; // reference images
		unsigned int maxd = 0xFF;
#ifdef USE_SP
		if (block_max > (SP_SIZE/4)) block_max = (SP_SIZE/4);
#endif

		for (int decimate = decimate_min; decimate <= decimate_max; decimate <<= 1) {
		printf("decimate: %d\n", decimate);

		/* reference 1 initialization */
		tget(t0);
		if (strncmp(argv[optind], "pat", 3) == 0) {
			if (!ref1.setSize(image_w, image_h)) return EXIT_FAILURE;
			pattern(ref1.getDataArray(), image_w, image_h, ref1.getPixelSz(), decimate);
		} else if (!ref1.loadFromFile(argv[optind])) {
			fprintf(stderr, " -- error loading image: %s\n", argv[optind]);
			return EXIT_FAILURE;
		}
		tget(t1);
		printf("ref1 w:%d h:%d element:%d init-time:%f sec\n",
			ref1.getWidth(), ref1.getHeight(), ref1.getPixelSz(),
			tsec(t1, t0));

		/* reference 2 initialization */
		tget(t0);
		if (strncmp(argv[optind+1], "pat", 3) == 0) {
			if (!ref2.setSize(image_w, image_h)) return EXIT_FAILURE;
			memset(ref2.getDataArray(), 0x11, image_w*image_h*ref2.getPixelSz());
		} else if (!ref2.loadFromFile(argv[optind+1])) {
			fprintf(stderr, " -- error loading image: %s\n", argv[optind+1]);
			return EXIT_FAILURE;
		}
		tget(t1);
		printf("ref2 w:%d h:%d element:%d init-time:%f sec\n",
			ref2.getWidth(), ref2.getHeight(), ref2.getPixelSz(),
			tsec(t1, t0));

		/* make sure reference images are equal in size */
		if (ref1.getWidth() != ref2.getWidth() ||
			ref1.getHeight() != ref2.getHeight()) {
			fprintf(stderr, " -- error, size of reference images differ\n");
			return EXIT_FAILURE;
		}

		/* aliases for reference images */
		size_t elem_sz = ref1.getPixelSz();
		int ref_w = ref1.getWidth();
		int ref_h = ref1.getHeight();
		pixel_p restrict ref_a1 = ref1.getDataArray();
		pixel_p restrict ref_a2 = ref2.getDataArray();

		/* difference image allocation */
		int davg_sz = (ref_w/decimate) * (ref_h/decimate);
		uchar_p davg = new uchar_t [davg_sz];
		printf("view w:%d h:%d\n", ref_w/decimate, ref_h/decimate);
		printf("ref1:%p ref2:%p davg:%p\n", (void*)ref_a1, (void*)ref_a2, (void*)davg);

		for (size_t block_sz = block_min; block_sz <= block_max; block_sz <<= 1) {
#if defined(USE_ACC)
		printf("block size: %lu\n", (ulong_t)block_sz);
#else
		block_max = block_min;
#endif

		tsetup = treorg = tcache = 0;
		CLOCKS_EMULATE
		CACHE_BARRIER2(dre1,dre2)
		TRACE_START
		STATS_START

		/* decimate and difference */
		/* assume input data is in memory and not cached (flushed and invalidated) */
		tget(start);

#if defined(USE_ACC)

		size_t view_off;
		size_t view_end = (ref_w/decimate) * (ref_h/decimate) * elem_sz;
		int v = 0;
		int didx = 0;
		struct view_c {
			int end; /* used by diff() */
			uchar_p restrict block1, block2;

			/* invalidate entire cache */
			void fill_E(size_t view_off, int block_end)
			{
				end = block_end;
				tget(t3);
				//CACHE_RECV_ALL
				Xil_L1DCacheFlush(); /* this also invalidates */
				tget(t4);
				dre1.fill(block1, block_end, view_off);
				dre1.wait();
				dre2.fill(block2, block_end, view_off);
				dre2.wait();
				tget(t5);
//				check(block1, block_end);
//				check(block2, block_end);
				tinc(treorg, tdiff(t5,t4));
				tinc(tcache, tdiff(t4,t3));
			}

			/* invalidate a range of the cache */
			void fill_R(size_t view_off, int block_end)
			{
				end = block_end;
				tget(t3);
				dre1.fill(block1, block_end, view_off);
				dre1.wait();
				dre2.fill(block2, block_end, view_off);
				dre2.wait();
				tget(t4);
				//CACHE_RECV(dre1, block1, block_end)
				//CACHE_RECV(dre2, block2, block_end)
				Xil_L1DCacheInvalidateRange((INTPTR)block1, CEIL(block_end,ALIGN_SZ));
				Xil_L1DCacheInvalidateRange((INTPTR)block2, CEIL(block_end,ALIGN_SZ));
				tget(t5);
//				check(block1, block_end);
//				check(block2, block_end);
				tinc(treorg, tdiff(t4,t3));
				tinc(tcache, tdiff(t5,t4));
			}

			int diff(uchar_p davg, int didx, size_t elem_sz)
			{
				register const uchar_t *ptr1 = block1;
				register const uchar_t *ptr2 = block2;
				register const uchar_t *end1 = ptr1 + end;
				//__asm__ __volatile__ ("nop");
				while (ptr1 < end1) {
					int dR = abs(ptr1[0] - ptr2[0]);
					int dG = abs(ptr1[1] - ptr2[1]);
					int dB = abs(ptr1[2] - ptr2[2]);
					ptr1 += elem_sz;
					ptr2 += elem_sz;
					davg[didx++] = (dR + dG + dB) / 3;
				}
				//__asm__ __volatile__ ("nop");
				return didx;
			}

		} view[2];

		view[0].block1 = SP_NALLOC(uchar_t, block_sz); // view block 1
		view[0].block2 = SP_NALLOC(uchar_t, block_sz); // view block 2
		view[1].block1 = SP_NALLOC(uchar_t, block_sz); // view block 1
		view[1].block2 = SP_NALLOC(uchar_t, block_sz); // view block 2

//		printf("v0.b1:%p v0.b2:%p v1.b1:%p v1.b2:%p\n",
//			(void*)view[0].block1, (void*)view[0].block2,
//			(void*)view[1].block1, (void*)view[1].block2);

		tget(t0);
		// not needed when memory has been invalidated before entry.
		// Xil_L1DCacheInvalidateRange((INTPTR)view[M].blockN, block_sz);
		tget(t1);
		/* set up view of images */
		dre1.setup(ref_a1, ref_w, ref_h, elem_sz, decimate);
		dre1.wait();
		dre2.setup(ref_a2, ref_w, ref_h, elem_sz, decimate);
		dre2.wait();
		tget(t2);
		view[v].fill_X(0, min(block_sz, view_end));
		for (view_off = block_sz; view_off < view_end; view_off += block_sz) {
			/* fill blocks with decimated view */
			view[v^1].fill_X(view_off, min(block_sz, view_end-view_off));
			/* view[v].wait(); write barrier needed after view[v].fill (not view[v^1]) */
			/* find difference between blocks */
			didx = view[v].diff(davg, didx, elem_sz);
			v ^= 1; // toggle
		}
		didx = view[v].diff(davg, didx, elem_sz);
		tget(t6);
		/* flush output product to memory */
		host::cache_flush(davg, davg_sz*sizeof(uchar_t));
		/* make sure to invalidate cache before delete */
		CACHE_DISPOSE(dre2, view[1].block2, block_sz*sizeof(uchar_t));
		CACHE_DISPOSE(dre1, view[1].block1, block_sz*sizeof(uchar_t));
		CACHE_DISPOSE(dre2, view[0].block2, block_sz*sizeof(uchar_t));
		CACHE_DISPOSE(dre1, view[0].block1, block_sz*sizeof(uchar_t));
		tget(t7);

		SP_NFREE((uchar_t*)view[1].block2);
		SP_NFREE((uchar_t*)view[1].block1);
		SP_NFREE((uchar_t*)view[0].block2);
		SP_NFREE((uchar_t*)view[0].block1);

		tinc(tcache, tdiff(t1,t0) + tdiff(t7,t6));
		tinc(tsetup, tdiff(t2,t1));

#else /* USE_ACC */

		size_t ref_inc = elem_sz * decimate;
		int view_w = ref_w / decimate;
		int view_h = ref_h / decimate;
#if defined(_OPENMP)
		#pragma omp parallel for
#endif
		for (int i = 0; i < view_h; i++) {
			register const uchar_t *ptr1 = (uchar_t*)&ref_a1[ref_w*decimate*i];
			register const uchar_t *ptr2 = (uchar_t*)&ref_a2[ref_w*decimate*i];
			register const uchar_t *end1 = ptr1 + elem_sz * ref_w;
			register uchar_t *dptr = &davg[view_w*i];
			//__asm__ __volatile__ ("nop");
			while (ptr1 < end1) {
				int dR = abs(ptr1[0] - ptr2[0]);
				int dG = abs(ptr1[1] - ptr2[1]);
				int dB = abs(ptr1[2] - ptr2[2]);
				ptr1 += ref_inc;
				ptr2 += ref_inc;
				*dptr++ = (dR + dG + dB) / 3;
			}
			//__asm__ __volatile__ ("nop");
		}
		tget(t0);
		/* flush output product to memory */
		host::cache_flush(davg, davg_sz*sizeof(uchar_t));
		tget(t1);
		tinc(tcache, tdiff(t1,t0));

#endif /* USE_ACC */

		/* assume output data is in memory (flushed) */
		tget(finish);

		CACHE_BARRIER2(dre1,dre2)
		STATS_STOP
		TRACE_STOP
		CLOCKS_NORMAL
		toper = tdiff(finish,start)-tsetup-treorg-tcache;
		printf("overall time: %f sec\n", tesec(finish, start));
#if defined(USE_ACC)
		printf("Setup time: %f sec\n", tsetup/(double)TICKS_ESEC);
		printf("Reorg time: %f sec\n", treorg/(double)TICKS_ESEC);
#endif
		printf("Oper. time: %f sec\n", toper/(double)TICKS_ESEC);
		printf("Cache time: %f sec\n", tcache/(double)TICKS_ESEC);
		STATS_PRINT

		/* find maximum difference */
		maxd = 0;
		for (int i = 0; i < davg_sz; i++) if (davg[i] > maxd) maxd = davg[i];
		printf("max difference: %u\n", maxd);
#if defined(USE_ACC)
		if (didx != davg_sz) printf(" -- error: difference incorrect\n");
#endif
		} // for block_sz...

		/* output image difference */
		unsigned int level = 0xFF - maxd;
		if (oname != NULL) {
			char fname[240+16];
			sprintf(fname, "%.240s_%d", oname, decimate);
			int x, y, w = ref_w/decimate, h = ref_h/decimate;
			ColorImage diff(w, h);
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					/* set R, G, B to same value (gray scale) */
					diff.setPixel(x, y, (davg[y*w + x]+level) * 0x010101);
				}
			}
			diff.writeToFile(fname);
			if (sflag) {
				char cmd[16+256];
				sprintf(cmd, "pamx %.256s&", fname);
				system(cmd);
			}
		}

		delete[] davg;
		} // for decimate...

		/* ColorImage ref1, ref2 destructors call CACHE_DISPOSE */
	}

	TRACE_CAP
	return EXIT_SUCCESS;
}
