
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/rmat_graph_generator.hpp>
#include <boost/random.hpp>
#include <cstddef> // size_t
#include <cstdlib> // exit
#include <cstdio> // printf
#include <unistd.h> // getopt, optarg
#include <algorithm> // min
#include <vector>
#include <iostream> // cout, endl
#include <fstream> // ofstream
#if defined(_OPENMP)
#include <omp.h>
#endif
using namespace std;

#include "hash_nbits.hpp"

// Example main arguments
// #define MARGS "-s16 -v15"
// #define MARGS "-s18 -v15" *
// #define MARGS "-s19 -v15"
// #define MARGS "-s22 -v15"

#include "lime.h"

#define DEFAULT_RMAT_SCALE 16U // log 2 size
#define DEFAULT_BLOCK_LSZ 12 // log 2 size
//#define PARTIAL 131072 // used to shorten run time for trace capture

// Use 32-bit integers for *smaller* graph
typedef unsigned int index_t; // used in adjacency list
typedef size_t gsize_t; // graph size type

typedef double* double_p;
typedef index_t* index_p;
#if defined(USE_STREAM) && defined(__linux__)
#include <allocator.h>
typedef std::vector<index_t, ACC_NS::allocator<index_t> > idx_vec_t;
typedef std::vector<idx_vec_t, ACC_NS::allocator<idx_vec_t> > graph_t;
typedef std::vector<double, ACC_NS::allocator<double> > double_vec_t;
#else
typedef std::vector<std::vector<index_t> > graph_t;
typedef std::vector<double> double_vec_t;
#endif

typedef boost::compressed_sparse_row_graph<boost::directedS, boost::no_property, boost::no_property, boost::no_property, index_t, index_t> DummyForRMAT;
typedef boost::rmat_iterator<boost::mt19937, DummyForRMAT> RMATGen;

#if defined(PARTIAL)
bool cflag = false;
#else
bool cflag = true;
#endif
bool pflag = false;
unsigned rmat_scale = DEFAULT_RMAT_SCALE;
unsigned block_sz = 1U<<DEFAULT_BLOCK_LSZ;
char *oname = NULL;

// TODO: find a better place for these globals

tick_t t0, t1, t2, t3, t4, t5, t6, t7, t8;
unsigned long long tsetup, treorg, toper, tcache;


//------------------ Support ------------------//

#if defined(USE_ACC)

#include "IndexArray.hpp"

#define MIN_EDGES 128
#define restrict __restrict__

IndexArray<index_t> dre; // Data Reorganization Engine

/**
 * Single Iteration of Page Rank
 * @param adj_list_graph  Adjacency list graph.
 *        Must be either undirected, or the transpose of a directed graph!
 * @param cur_pr_vector   Current PR vector
 * @param next_pr_vector  Next PR vector
 */
void page_rank_itr(
	graph_t& adj_list_graph,
	double_vec_t& cur_pr_vector,
	double_vec_t& next_pr_vector,
	double damping_factor,
	bool div
	)
{
	gsize_t num_vertices = adj_list_graph.size();
	gsize_t onepct = (num_vertices < 100) ? 1 : num_vertices/100;
	double jump_prob = (double(1) - damping_factor)/num_vertices;
	double_p cur_pr = cur_pr_vector.data();
	double_p next_pr = next_pr_vector.data();
	double_p restrict block = SP_NALLOC(double, block_sz/sizeof(double)); // view block
#if defined(PARTIAL)
	num_vertices = PARTIAL;
#endif

	tget(t0);
	// receive block, not needed when memory has been invalidated before entry.
	// Xil_L1DCacheInvalidateRange((INTPTR)block, block_sz);
	tget(t1);

	// Accumulate PRs for each vertex
	for (gsize_t i = 0; i < num_vertices; ++i) {
		gsize_t edges = adj_list_graph[i].size();
		index_p edge_i = adj_list_graph[i].data();
		if (pflag && i%onepct == 0) cout << i/onepct << "%\r" << flush;
		double tmp = next_pr[i];

		if (edges < MIN_EDGES) {
			// Loop over adjacency list
			for (gsize_t j = 0; j < edges; ++j) {
				tmp += cur_pr[edge_i[j]];
			}
		} else {
			tget(t2);
			// Set up view of cur_pr
			dre.setup(cur_pr, sizeof(double), edge_i, edges);
			dre.wait();
			tget(t3);
			// Loop over adjacency list
			gsize_t view_off;
			gsize_t view_end = edges * sizeof(double);
			for (view_off = 0; view_off < view_end; view_off += block_sz) {
				unsigned view_sz = min((gsize_t)block_sz, view_end-view_off);
				unsigned j;
				tget(t4);
				// fill block with indexed view
				dre.fill(block, view_sz, view_off);
				dre.wait();
				tget(t5);
				// receive block
				Xil_L1DCacheInvalidateRange((INTPTR)block, CEIL(view_sz,ALIGN_SZ));
				tget(t6);
				unsigned view_n = view_sz / sizeof(double);
				for (j = 0; j < view_n; ++j) {
					tmp += block[j];
				}
				tinc(treorg, tdiff(t5,t4));
				tinc(tcache, tdiff(t6,t5));
			}
			tinc(tsetup, tdiff(t3,t2));
		}

		// Apply damping
		tmp = jump_prob + (damping_factor * tmp);

		// Divide by out-degree
		if (div && edges) {
			tmp /= edges;
		}

		next_pr[i] = tmp;
	}
	tget(t7);
	// flush output product to memory
	host::cache_flush(next_pr, num_vertices*sizeof(double));
	// make sure to invalidate cache before delete
	CACHE_DISPOSE(dre, block, block_sz)
	tget(t8);
	SP_NFREE((double*)block);
	tinc(tcache, tdiff(t1,t0)+tdiff(t8,t7));
}

#else /* USE_ACC */

/**
 * Single Iteration of Page Rank
 * @param adj_list_graph  Adjacency list graph.
 *        Must be either undirected, or the transpose of a directed graph!
 * @param cur_pr_vector   Current PR vector
 * @param next_pr_vector  Next PR vector
 */
void page_rank_itr(
	graph_t& adj_list_graph,
	double_vec_t& cur_pr_vector,
	double_vec_t& next_pr_vector,
	double damping_factor,
	bool div
	)
{
	gsize_t num_vertices = adj_list_graph.size();
	gsize_t onepct = (num_vertices < 100) ? 1 : num_vertices/100;
	double jump_prob = (double(1) - damping_factor)/num_vertices;
	double_p cur_pr = cur_pr_vector.data();
	double_p next_pr = next_pr_vector.data();
#if defined(PARTIAL)
	num_vertices = PARTIAL;
#endif

	// Accumulate PRs for each vertex
#if defined(_OPENMP)
	#pragma omp parallel for
#endif
	for (gsize_t i = 0; i < num_vertices; ++i) {
		gsize_t edges = adj_list_graph[i].size();
		index_p edge_i = adj_list_graph[i].data();
		if (pflag && i%onepct == 0) cout << i/onepct << "%\r" << flush;
		double tmp = next_pr[i];

		// Loop over adjacency list
		for (gsize_t j = 0; j < edges; ++j) {
			tmp += cur_pr[edge_i[j]];
		}

		// Apply damping
		tmp = jump_prob + (damping_factor * tmp);

		// Divide by out-degree
		if (div && edges) {
			tmp /= edges;
		}

		next_pr[i] = tmp;
	}
	tget(t0);
	// flush output product to memory
	host::cache_flush(next_pr, num_vertices*sizeof(double));
	tget(t1);
	tinc(tcache, tdiff(t1,t0));
}

#endif /* USE_ACC */

void page_rank_itr_check(
	graph_t& adj_list_graph,
	double_vec_t& cur_pr_vector,
	double_vec_t& next_pr_vector,
	double damping_factor,
	bool div
	)
{
	gsize_t num_vertices = adj_list_graph.size();
	gsize_t onepct = (num_vertices < 100) ? 1 : num_vertices/100;
	double jump_prob = (double(1) - damping_factor)/num_vertices;
	double_p cur_pr = cur_pr_vector.data();
	double_p next_pr = next_pr_vector.data();
	gsize_t errors = 0;

	// Accumulate PRs for each vertex
	for (gsize_t i = 0; i < num_vertices; ++i) {
		gsize_t edges = adj_list_graph[i].size();
		index_p edge_i = adj_list_graph[i].data();
		if (pflag && i%onepct == 0) cout << i/onepct << "%\r" << flush;
		double tmp = 0.0; //next_pr[i];

		// Loop over adjacency list
		for (gsize_t j = 0; j < edges; ++j) {
			tmp += cur_pr[edge_i[j]];
		}

		// Apply damping
		tmp = jump_prob + (damping_factor * tmp);

		// Divide by out-degree
		if (div && edges) {
			tmp /= edges;
		}

		if (fabs(next_pr[i]-tmp) > 1e-9) {
			//cerr << " i:" << i << " found:" << next_pr[i] << " expected:" << tmp << endl;
			errors++;
		}
	}
	cerr << "errors:" << errors << endl;
}

//------------------ Main ------------------//


int MAIN(int argc, char *argv[])
{
	/* * * * * * * * * * get arguments beg * * * * * * * * * */
	int opt;
	bool nok = false;

#if defined(USE_ACC)
	dre.wait(); // wait for DRE initialization
#endif
	while ((opt = getopt(argc, argv, "cps:v:o:")) != -1) {
		switch (opt) {
		case 'c':
			cflag = true;
			break;
		case 'p':
			pflag = true;
			break;
		case 's':
			rmat_scale = atoi(optarg);
			break;
		case 'v':
			block_sz = (1U << atoi(optarg));
			if (block_sz < 8) {
				fprintf(stderr, "view buffer block size must be 8 or greater.\n");
				nok = true;
			}
			break;
		case 'o':
			oname = optarg;
			break;
		default: /* '?' */
			nok = true;
		}
	}
	if (nok) {
		fprintf(stderr, "Usage: pager -cp -s<int> -v<int> -o<out_file>\n");
		fprintf(stderr, "  -c  check first iteration\n");
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "  -s  RMAT scale 2^n, default: n=%d\n", DEFAULT_RMAT_SCALE);
		fprintf(stderr, "  -v  view buffer block size 2^n, default: n=%d\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -o  output page rank, default: NULL\n");
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
#if defined(_OPENMP)
	// to control the number of threads use: export OMP_NUM_THREADS=N
	printf("threads: %d\n", omp_get_max_threads());
#endif
#ifdef USE_SP
	if (block_sz > SP_SIZE) block_sz = SP_SIZE;
#endif
	printf("rmat scale: %u\n", rmat_scale);
#if defined(USE_ACC)
	printf("view_size: %u\n", block_sz);
	printf("min_edges: %u\n", MIN_EDGES); // threshold
#endif

	/* * * * * * * * * * get arguments end * * * * * * * * * */

	unsigned EDGE_FACTOR = 16;
	double rmat_a = 0.57;
	double rmat_b = 0.19;
	double rmat_c = 0.19;
	double rmat_d = 0.05;
	tick_t start, finish;
	double secs;

	boost::mt19937 rng;
	gsize_t num_vertices = gsize_t(1) << rmat_scale;
	gsize_t num_edges = num_vertices * EDGE_FACTOR;
	gsize_t onepct = (num_edges < 100) ? 1 : num_edges/100;

	// Start graph storage as adjacency list
	graph_t adj_list_graph(num_vertices);

	// Generate edges and insert into adjacency list
	cout << "RMAT generation, vertices:" << num_vertices << endl;
	cout << "RMAT generation, edges:" << num_edges << endl;
	cout << "RMAT generation, bytes:" << num_vertices*sizeof(void*)+num_edges*sizeof(index_t)*2 << endl;
	tget(start);
	RMATGen rmat_itr(rng, num_vertices, num_edges, rmat_a, rmat_b, rmat_c, rmat_d, false);
	for (gsize_t i = 0; i < num_edges; ++i) {
		if (pflag && i%onepct == 0) cout << i/onepct << "%\r" << flush;
		// Generates rmat edge
		std::pair<index_t, index_t> edge = *rmat_itr++;
		// Permute Vertex Labels
		edge.first  = hash_nbits(edge.first, rmat_scale);
		edge.second = hash_nbits(edge.second, rmat_scale);
		// Adds both directions to adjacency list (undirected)
		adj_list_graph[edge.first].push_back(edge.second);
		adj_list_graph[edge.second].push_back(edge.first);
	}
	tget(finish);
	secs = tsec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_SEC;
	cout << "RMAT generation, time:" << secs << " sec, EPS:" << (double)num_edges/secs << endl;

	// Prepare for PageRank iteration
	double_vec_t cur_pr_vector(num_vertices);
	double_vec_t next_pr_vector(num_vertices);
	boost::uniform_01<double> u01_dist;
	for (gsize_t i = 0; i < adj_list_graph.size(); ++i) {
		// Initializes PR to small random number.
		// Does not need normalization, but could...
		cur_pr_vector[i] = u01_dist(rng);
		// Divide by out-degree
		if (adj_list_graph[i].size())
			cur_pr_vector[i] /= adj_list_graph[i].size();
		next_pr_vector[i] = 0;
	}
	cout << "PageRank Vectors, bytes:" << num_vertices*sizeof(double)*2 << endl;

	tsetup = treorg = tcache = 0;
	CLOCKS_EMULATE
	CACHE_BARRIER(dre)
	TRACE_START
	STATS_START

	// assume input data is in memory and not cached (flushed and invalidated)
	tget(start);
	page_rank_itr(adj_list_graph, cur_pr_vector, next_pr_vector, 0.85, false);
	// assume output data is in memory (flushed)
	tget(finish);

	CACHE_BARRIER(dre)
	STATS_STOP
	TRACE_STOP
	CLOCKS_NORMAL
	toper = tdiff(finish,start)-tsetup-treorg-tcache;
	secs = tesec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_ESEC;
	cout << "page rank time:" << secs << " sec" << endl;
#if defined(USE_ACC)
	cout << "Setup time: " << tsetup/(double)TICKS_ESEC << " sec" << endl;
	cout << "Reorg time: " << treorg/(double)TICKS_ESEC << " sec" << endl;
#endif
	cout << "Oper. time: " << toper/(double)TICKS_ESEC << " sec" << endl;
	cout << "Cache time: " << tcache/(double)TICKS_ESEC << " sec" << endl;
	STATS_PRINT

	if (cflag) page_rank_itr_check(adj_list_graph, cur_pr_vector, next_pr_vector, 0.85, false);

	/* output page rank */
	if (oname != NULL) {
		std::ofstream ofs(oname, std::ofstream::out);
		for (double_vec_t::iterator it = next_pr_vector.begin(); it != next_pr_vector.end(); ++it)
			ofs << ' ' << *it << endl;
		ofs.close();
	}

	TRACE_CAP
	return EXIT_SUCCESS;
}
