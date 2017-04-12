
/*
  The main differences of block_multimap from unordered_multimap are:
  1) Block oriented methods (e.g., insert, equal_range) have been added
     that operate on arrays of elements.
  2) A maximum number of elements per key can be specified with MAX_ELEM.
  3) USE_ACC Only: Values are stored in arrays without being paired with
     keys. Hence, equal_range() returns a mapped_type pointer instead of
     a pair of iterators. The first mapped_type entry is a count of values
     that follow.
*/

#ifndef _BLOCK_MAP_H
#define _BLOCK_MAP_H

#include <cstddef> // size_t
#include <utility> // std::pair

#define MAX_ELEM 7 // comment out for performance testing

#if defined(USE_ACC)

#define CHUNK_SZ 4
#include "KVstore.hpp"

tick_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
unsigned long long tsetup, tfill, tdrain, tinsert, toper, tcache;

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class block_multimap {
public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;
	// typedef _Hash hasher;
	// typedef _Pred key_equal;
	// typedef _Alloc allocator_type;

	// iterator-related
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef value_type* iterator;
	typedef const value_type* const_iterator;
	typedef value_type* local_iterator;
	typedef const value_type* const_local_iterator;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	size_type element_count; // total number of elements in container
	size_type maxelem; // maximum number of elements per key

	KVstore<key_type, mapped_type*> acc; // Key-Value Accelerator

	iterator
	end() noexcept
	{
		return iterator(nullptr);
	}

	// TODO: benchmark insert into a map (not multimap), single element per key

	iterator
	insert(const value_type& __x) // returns no indication if existing or not
	{
		mapped_type *mptr = nullptr;
		typename KVstore<key_type, mapped_type*>::slot_s *slot;

		slot = acc._update1(__x.first, mptr);
		// tget(t6);
		if (mptr == acc.null_value) {
			mptr = (mapped_type *)malloc(sizeof(mapped_type)*CHUNK_SZ);
			// chk_alloc(mptr, sizeof(mapped_type)*CHUNK_SZ, "malloc in insert()");
			mptr[0] = 1;
			if (maxelem < 1) maxelem = 1;
			mptr[1] = __x.second;
		} else {
			if (++mptr[0] > maxelem) maxelem = mptr[0];
			if (mptr[0] <= MAX_ELEM) {
				if (mptr[0] % CHUNK_SZ == 0) {
					unsigned chunks = mptr[0] / CHUNK_SZ;
					mptr = (mapped_type *)realloc(mptr, sizeof(mapped_type)*CHUNK_SZ*(chunks+1));
					// chk_alloc(mptr, sizeof(mapped_type)*CHUNK_SZ*(chunks+1), "realloc in insert()");
				}
				mptr[mptr[0]] = __x.second;
			}
		}
		element_count++;
		// tget(t7);
		acc._update2(slot, mptr);
		// tinc(tinsert, tdiff(t7,t6));
		// FIXME: figure how to return iterator when
		// entire value_type is not stored in container
		// or just return mptr.
		return end();
	}

#if 1
	void
	insert(const_pointer __arr, size_type __n)
	{
#if defined(USE_STREAM) && defined(__arm__)
		mtcp(XREG_CP15_CACHE_SIZE_SEL, 0);
#endif
		tget(t6);
		while (__n--) insert(*__arr++);
		tget(t7);
#if defined(USE_STREAM) && defined(__arm__)
		dsb();
		// acc.cache_flush();
		// tget(t8);
		// tinc(tcache, tdiff(t8,t7));
#endif
		tinc(tinsert, tdiff(t7,t6));
	}
#else
	void
	insert(const_pointer __arr, size_type __n)
	{
		std::sort(__arr, __arr+__n); // uses combination of quicksort, heapsort, and insertion sort
		tget(t3);
		CACHE_SEND(acc, __arr, sizeof(value_type)*__n);
		tget(t4);
		acc.fill(value, __n, __arr, sizeof(value_type));
		tget(t5);
		CACHE_RECV(acc, value, sizeof(mapped_type*)*co__nunt);
		tget(t6);
	}
#endif

	inline mapped_type*
	equal_range(const key_type& __x) // returns nullptr if no match
	{
		mapped_type* mptr;
		acc._get(__x, mptr);
		return mptr;
	}

#if 0
	void
	equal_range(
		mapped_type** __rng,
		const key_type* __x,
		size_type __n)
	{
		while (__n--) *__rng++ = equal_range(*__x++);
	}
#else
	void
	equal_range(
		mapped_type** __rng,
		const key_type* __x,
		size_type __n)
	{
		tget(t3);
		CACHE_SEND(acc, __x, sizeof(key_type)*__n);
		tget(t4);
		acc.fill(__rng, __n, __x, sizeof(key_type));
		tget(t5);
		CACHE_RECV(acc, __rng, sizeof(mapped_type*)*__n);
		tget(t6);
		tinc(tcache, tdiff(t4,t3) + tdiff(t6,t5));
		tinc(tfill, tdiff(t5,t4));
	}
#endif

	iterator
	find(const key_type& __x) // returns end if no match
	{
		return end(); /* TODO: */
	}

	void
	find(iterator* __fnd, const key_type* __x, size_type __n)
	{
		while (__n--) *__fnd++ = find(*__x++);
	}

	size_type
	erase(const key_type& __k)
	{
		return 0; /* TODO: */
	}

	size_type
	size() const noexcept
	{
		return element_count;
	}

	size_type
	bucket_count() const noexcept
	{
		return acc.data_len; // acc.data_len + acc.topsearch?
	}

	float
	load_factor() const noexcept
	{
		return static_cast<float>(size()) /
			static_cast<float>(bucket_count());
	}

	void
	reserve(size_type __n)
	{
		acc.setup(__n);
	}

	// * * * * * * * * * * non-standard methods * * * * * * * * * * //

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		printf("size:%lu unique:%lu duplicates:%lu %.2f%%\n",
			(ul_t)size(), (ul_t)acc.elements, (ul_t)(size()-acc.elements),
			(double)(size()-acc.elements)/size()*100.0);
		printf("load_factor (elem):%f\n", load_factor());
		printf("load_factor (keys):%f\n", (double)acc.elements/bucket_count());
		printf("bucket_count:%lu\n", (ul_t)bucket_count());
		printf("max_elem_per_key:%lu\n", (ul_t)maxelem);
		printf("max_psl:%lu\n", (ul_t)acc.topsearch);
	}

	void clear_time(void)
	{
		tfill = tdrain = tinsert = tcache = toper = 0;
	}

	void print_time(unsigned long long total)
	{
		toper = total-tfill-tdrain-tinsert-tcache;
		printf("Fill    time: %f sec\n", tfill/(double)TICKS_ESEC);
		printf("Drain   time: %f sec\n", tdrain/(double)TICKS_ESEC);
		printf("Insert  time: %f sec\n", tinsert/(double)TICKS_ESEC);
		printf("Cache   time: %f sec\n", tcache/(double)TICKS_ESEC);
		printf("Oper.   time: %f sec\n", toper/(double)TICKS_ESEC);
	}

}; // class block_multimap

#else // defined(USE_ACC)

#include <unordered_map>

tick_t t6, t7;
unsigned long long tinsert, tlookup, toper;

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class block_multimap
	: public std::unordered_multimap<_Key, _Tp>
{
private:
	using base_map = std::unordered_multimap<_Key, _Tp>;
	typename base_map::size_type element_count;

public:
	typedef typename base_map::key_type key_type;
	typedef typename base_map::mapped_type mapped_type;
	typedef typename base_map::value_type value_type;
	// typedef typename base_map::hasher hasher;
	// typedef typename base_map::key_equal key_equal;
	// typedef typename base_map::allocator_type allocator_type;

	// iterator-related
	typedef typename base_map::pointer pointer;
	typedef typename base_map::const_pointer const_pointer;
	typedef typename base_map::reference reference;
	typedef typename base_map::const_reference const_reference;
	typedef typename base_map::iterator iterator;
	typedef typename base_map::const_iterator const_iterator;
	typedef typename base_map::local_iterator local_iterator;
	typedef typename base_map::const_local_iterator const_local_iterator;
	typedef typename base_map::size_type size_type;
	typedef typename base_map::difference_type difference_type;

	// TODO: other insert, erase methods

#if defined(MAX_ELEM)
	// limit number of stored elements per key

	iterator
	insert(const value_type& __x) // no indication if existing or not
	{
		element_count++;
		if (base_map::count(__x.first) < MAX_ELEM)
			return base_map::insert(__x);
		return base_map::end();
	}

	size_type
	erase(const key_type& __k)
	{
		element_count--;
		return base_map::erase(__k);
	}

	size_type
	size() const noexcept
	{ return element_count; }

	float
	load_factor() const noexcept
	{
		return static_cast<float>(element_count) /
			static_cast<float>(base_map::bucket_count());
	}

#else

	inline iterator
	insert(const value_type& __x) // no indication if existing or not
	{ return base_map::insert(__x); }

#endif // defined(MAX_ELEM)

	void
	insert(const_pointer __arr, size_type __n) // no indication if existing
	{
		tget(t6);
		while (__n--) this->insert(*__arr++);
		tget(t7);
		tinc(tinsert, tdiff(t7,t6));
	}

	inline std::pair<iterator, iterator>
	equal_range(const key_type& __x) // returns {end, end} if no match
	{ return base_map::equal_range(__x); }

	void
	equal_range(
		std::pair<iterator, iterator>* __rng,
		const key_type* __x,
		size_type __n)
	{
		tget(t6);
		while (__n--) *__rng++ = this->equal_range(*__x++);
		tget(t7);
		tinc(tlookup, tdiff(t7,t6));
	}

	inline iterator
	find(const key_type& __x) // returns end if no match
	{ return base_map::find(__x); }

	void
	find(iterator* __fnd, const key_type* __x, size_type __n)
	{ while (__n--) *__fnd++ = this->find(*__x++); }

	// * * * * * * * * * * non-standard methods * * * * * * * * * * //

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		size_type max_elem_per_bucket = 0;
		size_type max_elem_per_key = 0;
		size_type max_psl = 0; // max probe sequence length
		size_type keycnt = 0; // key count

		// gather stats in one pass through table
		std::unordered_map<key_type,size_type> epkcnt; // elem per key count
		for (size_type i = 0; i < this->bucket_count(); ++i) {
			size_type psl = 0; // probe sequence length
			epkcnt.clear();
			for (auto local_it = this->cbegin(i); local_it != this->cend(i); ++local_it) {
				psl++;
				if (++epkcnt[local_it->first] == 1 && psl > max_psl) max_psl = psl;
			}
			for (auto& x: epkcnt) {
				if (x.second > max_elem_per_key) {
					max_elem_per_key = x.second;
				}
			}
			keycnt += epkcnt.size();
			if (this->bucket_size(i) > max_elem_per_bucket)
				max_elem_per_bucket = this->bucket_size(i);
		}
		printf("size:%lu unique:%lu duplicates:%lu %.2f%%\n",
			(ul_t)this->size(), (ul_t)keycnt, (ul_t)(this->size()-keycnt),
			(double)(this->size()-keycnt)/this->size()*100.0);
		printf("load_factor (elem):%f\n", this->load_factor());
		printf("load_factor (keys):%f\n", (double)keycnt/this->bucket_count());
		printf("bucket_count:%lu\n", (ul_t)this->bucket_count());
		printf("max_elem_per_bucket:%lu\n", (ul_t)max_elem_per_bucket);
		printf("max_elem_per_key:%lu\n", (ul_t)max_elem_per_key);
		printf("max_psl:%lu\n", (ul_t)max_psl);
	}

	void clear_time(void)
	{
		tinsert = tlookup = toper = 0;
	}

	void print_time(unsigned long long total)
	{
		toper = total-tinsert-tlookup;
		printf("Insert  time: %f sec\n", tinsert/(double)TICKS_ESEC);
		printf("Lookup  time: %f sec\n", tlookup/(double)TICKS_ESEC);
		printf("Oper.   time: %f sec\n", toper/(double)TICKS_ESEC);
	}

}; // class block_multimap

#endif // defined(USE_ACC)

#endif /* _BLOCK_MAP_H */
