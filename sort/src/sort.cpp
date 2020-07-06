
#include <cstddef> // size_t
#include <cstdlib> // exit
#include <algorithm> // random_shuffle, sort
#include <iostream> // cout, endl
#include <unistd.h> // getopt, optarg
using namespace std;

// Example main arguments
// #define MARGS "-c -s20" *

#include "lime.h"

#define DEFAULT_SCALE 16U // log 2 size

typedef unsigned long long elem_t; // element type

bool cflag = false;
unsigned scale = DEFAULT_SCALE;

// TODO: find a better place for these globals

tick_t t0, t1;
unsigned long long tsetup, treorg, toper, tcache;


//------------------ Main ------------------//


int MAIN(int argc, char *argv[])
{
	/* * * * * * * * * * get arguments beg * * * * * * * * * */
	int opt;
	bool nok = false;

	while ((opt = getopt(argc, argv, "cs:")) != -1) {
		switch (opt) {
		case 'c':
			cflag = true;
			break;
		case 's':
			scale = atoi(optarg);
			break;
		default: /* '?' */
			nok = true;
		}
	}
	if (nok) {
		fprintf(stderr, "Usage: sort -c -s<int>\n");
		fprintf(stderr, "  -c  check\n");
		fprintf(stderr, "  -s  scale 2^n, default: n=%d\n", DEFAULT_SCALE);
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
	printf("scale: %u\n", scale);

	/* * * * * * * * * * get arguments end * * * * * * * * * */

	tick_t start, finish;
	double secs;

	// Allocate Data
	size_t num_elems = size_t(1) << scale;
	elem_t *data = (elem_t*)malloc(num_elems * sizeof(elem_t));
	if (data == NULL) {
		cerr << "Failed to allocate memory" << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Element size: " << sizeof(elem_t) << endl;
	cout << "Array elements: " << num_elems << endl;
	cout << "Array bytes: " << num_elems*sizeof(elem_t) << endl;

	// Init Data
	tget(start);
	for (size_t i = 0; i < num_elems; ++i) {data[i] = i;}
	tget(finish);
	secs = tsec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_SEC;
	cout << "Array init, time: " << secs << " sec" << endl;

	// Scramble Data
	tget(start);
	random_shuffle(data, data+num_elems);
	tget(finish);
	secs = tsec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_SEC;
	cout << "Scramble, time: " << secs << " sec" << endl;

	// Sort Data
	tsetup = treorg = tcache = 0;
	CLOCKS_EMULATE
	CACHE_BARRIER(NULL)
	TRACE_START
	STATS_START

	// assume input data is in memory and not cached (flushed and invalidated)
	tget(start);
	sort(data, data+num_elems);
	tget(t0);
	host::cache_flush();
	tget(t1);
	tinc(tcache, tdiff(t1,t0));
	// assume output data is in memory (flushed)
	tget(finish);

	CACHE_BARRIER(NULL)
	STATS_STOP
	TRACE_STOP
	CLOCKS_NORMAL
	toper = tdiff(finish,start)-tsetup-treorg-tcache;
	secs = tesec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_ESEC;
	cout << "Sort time: " << secs << " sec" << endl;
	cout << "Oper. time: " << toper/(double)TICKS_ESEC << " sec" << endl;
	cout << "Cache time: " << tcache/(double)TICKS_ESEC << " sec" << endl;
	STATS_PRINT

	// Validate Data
	if (cflag) {
		for (size_t i = 0; i < num_elems; ++i) {
			if (data[i] != i) {
				cerr << "Found an error!" << endl;
				exit(EXIT_FAILURE);
			}
		}
		cout << "No errors" << endl;
	}

	// Free data
	free(data);

	TRACE_CAP
	return EXIT_SUCCESS;
}
