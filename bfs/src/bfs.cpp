
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/rmat_graph_generator.hpp>
#include <boost/random.hpp>
#include <cstddef> // size_t
#include <cstdlib> // exit
#include <cstdio> // printf
#include <cassert> // assert, define NDEBUG to disable
#include <unistd.h> // getopt, optarg
#include <algorithm> // min
#include <limits> // numeric_limits<T>::max
#include <vector>
#include <iostream> // cout, endl
#include <fstream> // ofstream
using namespace std;

#include "hash_nbits.hpp"

// Example main arguments
// #define MARGS "-s16 -v15"
// #define MARGS "-s18 -v15" *
// #define MARGS "-s19 -v15"
// #define MARGS "-s20 -v15"
// #define MARGS "-s22 -v15"

#include "lime.h"

#define DEFAULT_RMAT_SCALE 16U // log 2 size
#define DEFAULT_BLOCK_LSZ 12 // log 2 size
//#define PARTIAL 131072 // used to shorten run time for trace capture

// Use 32-bit integers for *smaller* graph
typedef unsigned int index_t; // used in adjacency list
typedef size_t gsize_t; // graph size type
typedef unsigned char level_t; // level type

typedef index_t* index_p;
typedef std::vector< std::vector <index_t> > graph_t;

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

#undef USE_ACC // not implemented

#if defined(USE_ACC)

#include "IndexArray.hpp"

#define MIN_EDGES 128
#define restrict __restrict__

IndexArray<index_t> dre; // Data Reorganization Engine

/**
 * Breadth-First Search
 * @param adj_list_graph  Adjacency list graph
 * @param source          Root vertex
 * @param level_info      Output: Level of each vertex in search path
 * @param parent_info     Output: Parent of each vertex in search path
 */
void bfs(
	graph_t& adj_list_graph,
	index_t source,
	std::vector<level_t>& level_info,
	std::vector<index_t>& parent_info
	)
{
	// TODO: accelerated version
}

#else /* USE_ACC */

/**
 * Breadth-First Search
 * @param adj_list_graph  Adjacency list graph
 * @param source          Root vertex
 * @param level_info      Output: Level of each vertex in search path
 * @param parent_info     Output: Parent of each vertex in search path
 */
void bfs(
	graph_t& adj_list_graph,
	index_t source,
	std::vector<level_t>& level_info,
	std::vector<index_t>& parent_info
	)
{
	std::vector<index_t> cur_level, next_level;
	cur_level.reserve(adj_list_graph.size());
	next_level.reserve(adj_list_graph.size());

	cur_level.push_back(source);
	level_info[source] = 0;
	parent_info[source] = source;

	unsigned int bfs_level = 0;

	while (cur_level.size() + next_level.size() > 0) {
		if (cur_level.empty()) {
			++bfs_level;
			next_level.swap(cur_level);
			if (pflag) cout << "BFS Level:" << bfs_level << " count:" << cur_level.size() << endl;
		}
		index_t to_visit = cur_level.back();
		cur_level.pop_back();
		assert(level_info[to_visit] == bfs_level);

		for (gsize_t i = 0; i < adj_list_graph[to_visit].size(); ++i) {
			index_t neighbor = adj_list_graph[to_visit][i];
			if (level_info[neighbor] == std::numeric_limits<level_t>::max()) {
				level_info[neighbor] = bfs_level + 1;
				parent_info[neighbor] = to_visit;
				next_level.push_back(neighbor);
			}
		}
	}
	tget(t0);
	// flush output product to memory
	host::cache_flush(level_info.data(), level_info.size()*sizeof(level_info.front()));
	host::cache_flush(parent_info.data(), parent_info.size()*sizeof(parent_info.front()));
	tget(t1);
	tinc(tcache, tdiff(t1,t0));
}

#endif /* USE_ACC */

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
		fprintf(stderr, "Usage: bfs -cp -s<int> -v<int> -o<out_file>\n");
		fprintf(stderr, "  -c  check\n");
		fprintf(stderr, "  -p  show progress\n");
		fprintf(stderr, "  -s  RMAT scale 2^n, default: n=%d\n", DEFAULT_RMAT_SCALE);
		fprintf(stderr, "  -v  view buffer block size 2^n, default: n=%d\n", DEFAULT_BLOCK_LSZ);
		fprintf(stderr, "  -o  output, default: NULL\n");
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}
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

	// Prepare for BFS
	std::vector<level_t> level_info;
	std::vector<index_t> parent_info;
	level_info.clear();
	level_info.resize(adj_list_graph.size(), std::numeric_limits<level_t>::max());
	parent_info.clear();
	parent_info.resize(adj_list_graph.size(), std::numeric_limits<index_t>::max());    

	// Choose a source that has edges
	index_t source = 0;
	for (; source < adj_list_graph.size(); ++source) {
		if (adj_list_graph[source].size()) break;
	}
	cout << "Running BFS from source:" << source << endl;

	tsetup = treorg = tcache = 0;
	CLOCKS_EMULATE
	CACHE_BARRIER(dre)
	TRACE_START
	STATS_START

	// assume input data is in memory and not cached (flushed and invalidated)
	tget(start);
	bfs(adj_list_graph, source, level_info, parent_info);
	// assume output data is in memory (flushed)
	tget(finish);

	CACHE_BARRIER(dre)
	STATS_STOP
	TRACE_STOP
	CLOCKS_NORMAL
	toper = tdiff(finish,start)-tsetup-treorg-tcache;
	secs = tesec(finish, start);
	if (secs == 0.0) secs = 1.0/TICKS_ESEC;
	cout << "BFS time:" << secs << " sec" << endl;
#if defined(USE_ACC)
	cout << "Setup time: " << tsetup/(double)TICKS_ESEC << " sec" << endl;
	cout << "Reorg time: " << treorg/(double)TICKS_ESEC << " sec" << endl;
#endif
	cout << "Oper. time: " << toper/(double)TICKS_ESEC << " sec" << endl;
	cout << "Cache time: " << tcache/(double)TICKS_ESEC << " sec" << endl;
	STATS_PRINT

	//if (cflag) ; // TODO: verify results

	/* output */
	//if (oname != NULL) {
	//	std::ofstream ofs(oname, std::ofstream::out);
	// TODO: output level_info, parent_info
	//	ofs.close();
	//}

	TRACE_CAP
	return EXIT_SUCCESS;
}
