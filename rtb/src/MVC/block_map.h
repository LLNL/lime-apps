
/*
  The main differences of block_multimap from unordered_multimap are:
  1) Block oriented methods (e.g., insert, equal_range, find) have been
     added that operate on arrays of elements.
  2) A maximum number of elements per key can be specified with MAX_ELEM.
  3) USE_ACC Only: Values are stored in result arrays without being paired
     with keys. Hence, equal_range() returns a mapped_type pointer instead
     of a pair of iterators. The first mapped_type entry is a count of values
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

/* these are global to avoid overhead at runtime on the stack */
tick_t tA, tB, tC, tD;
unsigned long long tsetup, tfill, tdrain, tcache;

/* * * * * * * * * * Map, Accelerator * * * * * * * * * */

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class block_map {
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
	// typedef const value_type* const_local_iterator;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	KVstore<key_type, mapped_type> acc; // Key-Value Accelerator

	// TODO: move up and make template based on value like in hashtable_policy.h:300
	// C:\cygwin\lib\gcc\i686-pc-cygwin\5.4.0\include\c++\bits\hashtable_policy.h
	struct const_local_iterator {

		std::pair<key_type, mapped_type> val;
		void *slot; // TODO: change to slot_s pointer

		const_local_iterator() noexcept
		: slot(nullptr) { }

		explicit
		const_local_iterator(void* __p) noexcept
		: slot(__p) { }

		const_pointer
		operator->() const noexcept
		{
			return reinterpret_cast<const_pointer>(&this->val);
		}

		const_local_iterator
		operator++(int) noexcept
		{
			const_local_iterator __tmp(*this);
			// this->_M_incr();
			return __tmp;
		}

		// template<class _Key1, class _Tp1>
		// friend bool
		// operator==(
			// const typename block_map<_Key1, _Tp1>::const_local_iterator& __x,
			// const typename block_map<_Key1, _Tp1>::const_local_iterator& __y)
		// noexcept;
	};

	size_type
	count(const key_type& __x) const
	{
		return 0; // TODO:
	}

	size_type
	bucket_size(size_type __n) const
	{
		return 0; // TODO:
	}

	const_local_iterator
	cbegin(size_type __n) const
	{
		return const_local_iterator(nullptr); // TODO:
	}

	const_local_iterator
	cend(size_type __n) const
	{
		return const_local_iterator(nullptr);
	}

	iterator
	end() noexcept
	{
		return iterator(nullptr);
	}

	inline std::pair<iterator, bool>
	insert(const value_type& __x) // returns true if new
	{
		acc._put(__x.first, __x.second);
		// FIXME: figure how to return iterator when
		// entire value_type is not stored in container
		// or just return mapped_type pointer?
		return std::make_pair(nullptr, false);
	}

#if 1
	void
	insert(const_pointer __arr, size_type __n)
	{
#if defined(USE_STREAM) && defined(__arm__)
		mtcp(XREG_CP15_CACHE_SIZE_SEL, 0);
#endif
		while (__n--) insert(*__arr++);
#if defined(USE_STREAM) && defined(__arm__)
		dsb();
		// tget(tA);
		// acc.cache_flush();
		// tget(tB);
		// tinc(tcache, tdiff(tB,tA));
#endif
	}
#else
	// TODO: version with drain only, modify args to drain(kvpair)?
	void
	insert(const_pointer __arr, size_type __n)
	{
		tget(tA);
		std::sort(__arr, __arr+__n); // uses combination of quicksort, heapsort, and insertion sort
		tget(tB);
		CACHE_SEND(acc, __arr, sizeof(value_type)*__n);
		tget(tC);
		acc.fill(value, __n, __arr, sizeof(value_type));
		tget(tD);
		CACHE_RECV(acc, value, sizeof(mapped_type)*__n);
		tget(tE);
	}
#endif

	inline mapped_type*
	equal_range(const key_type& __x) // returns nullptr if no match
	{
		return nullptr;
	}

	void
	equal_range(
		mapped_type** __rng,
		const key_type* __x,
		size_type __n)
	{
		while (__n--) *__rng++ = equal_range(*__x++);
	}

	inline mapped_type
	find(const key_type& __x) // returns zero if no match
	{
		mapped_type __v;
		acc._get(__x, __v);
		return __v;
	}

#if 0
	void
	find(mapped_type* __fnd, const key_type* __x, size_type __n)
	{
		while (__n--) *__fnd++ = find(*__x++);
	}
#else
	void
	find(mapped_type* __fnd, const key_type* __x, size_type __n)
	{
		tget(tA);
		CACHE_SEND(acc, __x, sizeof(key_type)*__n);
		tget(tB);
		acc.fill(__fnd, __n, __x, sizeof(key_type));
		tget(tC);
		CACHE_RECV(acc, __fnd, sizeof(mapped_type)*__n);
		tget(tD);
		tinc(tcache, tdiff(tB,tA) + tdiff(tD,tC));
		tinc(tfill, tdiff(tC,tB));
	}
#endif

	size_type
	erase(const key_type& __k)
	{
		return 0; /* TODO: */
	}

	size_type
	size() const noexcept
	{
		return acc.elements;
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
		printf("size:%lu\n", (ul_t)size());
		printf("load_factor (elem):%f\n", load_factor());
		printf("bucket_count:%lu\n", (ul_t)bucket_count());
		printf("max_psl:%lu\n", (ul_t)acc.topsearch);
	}

	void clear_time(void)
	{
		tsetup = tfill = tdrain = tcache = 0;
	}

	void print_time(void)
	{
		printf("  Fill  time: %f sec\n", tvesec(tfill));
		printf("  Drain time: %f sec\n", tvesec(tdrain));
		printf("  Cache time: %f sec\n", tvesec(tcache));
	}

}; // class block_map

// TODO: move up and make like hashtable_policy.h:315
// C:\cygwin\lib\gcc\i686-pc-cygwin\5.4.0\include\c++\bits\hashtable_policy.h
template<class _Key1, class _Tp1>
inline bool
operator==(
	const typename block_map<_Key1, _Tp1>::const_local_iterator& __x,
	const typename block_map<_Key1, _Tp1>::const_local_iterator& __y)
noexcept
{
	return __x.slot == __y.slot;
}

/* * * * * * * * * * MultiMap, Accelerator * * * * * * * * * */

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

	iterator
	insert(const value_type& __x) // returns no indication if new
	{
		mapped_type *mptr = nullptr;
		typename KVstore<key_type, mapped_type*>::slot_s *slot;

		slot = acc._update1(__x.first, mptr);
		// tget(tA);
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
		// tget(tB);
		acc._update2(slot, mptr);
		// tinc(tupdate, tdiff(tB,tA));
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
		while (__n--) insert(*__arr++);
#if defined(USE_STREAM) && defined(__arm__)
		dsb();
		// tget(tA);
		// acc.cache_flush();
		// tget(tB);
		// tinc(tcache, tdiff(tB,tA));
#endif
	}
#else
	void
	insert(const_pointer __arr, size_type __n)
	{
		tget(tA);
		std::sort(__arr, __arr+__n); // uses combination of quicksort, heapsort, and insertion sort
		tget(tB);
		CACHE_SEND(acc, __arr, sizeof(value_type)*__n);
		tget(tC);
		acc.fill(value, __n, __arr, sizeof(value_type));
		tget(tD);
		CACHE_RECV(acc, value, sizeof(mapped_type*)*co__nunt);
		tget(tE);
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
		tget(tA);
		CACHE_SEND(acc, __x, sizeof(key_type)*__n);
		tget(tB);
		acc.fill(__rng, __n, __x, sizeof(key_type));
		tget(tC);
		CACHE_RECV(acc, __rng, sizeof(mapped_type*)*__n);
		tget(tD);
		tinc(tcache, tdiff(tB,tA) + tdiff(tD,tC));
		tinc(tfill, tdiff(tC,tB));
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
		tsetup = tfill = tdrain = tcache = 0;
	}

	void print_time(void)
	{
		printf("  Fill  time: %f sec\n", tvesec(tfill));
		printf("  Drain time: %f sec\n", tvesec(tdrain));
		printf("  Cache time: %f sec\n", tvesec(tcache));
	}

}; // class block_multimap

#else // defined(USE_ACC)

#include <unordered_map>

/* * * * * * * * * * Map, Standard * * * * * * * * * */

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class block_map
	: public std::unordered_map<_Key, _Tp>
{
private:
	using base_map = std::unordered_map<_Key, _Tp>;

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

	inline std::pair<iterator, bool>
	insert(const value_type& __x) // returns true if new
	{ return base_map::insert(__x); }

	void
	insert(const_pointer __arr, size_type __n)
	{ while (__n--) this->insert(*__arr++); }

	inline std::pair<iterator, iterator>
	equal_range(const key_type& __x) // returns {end, end} if no match
	{ return base_map::equal_range(__x); }

	void
	equal_range(
		std::pair<iterator, iterator>* __rng,
		const key_type* __x,
		size_type __n)
	{ while (__n--) *__rng++ = this->equal_range(*__x++); }

#if 0

	inline iterator
	find(const key_type& __x) // returns end if no match
	{ return base_map::find(__x); }

	void
	find(iterator* __fnd, const key_type* __x, size_type __n)
	{ while (__n--) *__fnd++ = this->find(*__x++); }

#else

	inline mapped_type
	find(const key_type& __x) // returns zero if no match
	{
		iterator __it = base_map::find(__x);
		if (__it == base_map::end()) return 0;
		return __it->second;
	}

	void
	find(mapped_type* __fnd, const key_type* __x, size_type __n)
	{ while (__n--) *__fnd++ = this->find(*__x++); }

#endif

	// * * * * * * * * * * non-standard methods * * * * * * * * * * //

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		size_type max_elem_per_bucket = 0;

		// gather stats in one pass through table
		for (size_type i = 0; i < this->bucket_count(); ++i) {
			if (this->bucket_size(i) > max_elem_per_bucket)
				max_elem_per_bucket = this->bucket_size(i);
		}
		printf("size:%lu\n", (ul_t)this->size());
		printf("load_factor (elem):%f\n", this->load_factor());
		printf("bucket_count:%lu\n", (ul_t)this->bucket_count());
		printf("max_elem_per_bucket:%lu\n", (ul_t)max_elem_per_bucket);
	}

	void clear_time(void)
	{
	}

	void print_time(void)
	{
	}

}; // class block_map

/* * * * * * * * * * MultiMap, Standard * * * * * * * * * */

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
	insert(const value_type& __x) // returns no indication if new
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
	{
		return element_count;
	}

	float
	load_factor() const noexcept
	{
		return static_cast<float>(element_count) /
			static_cast<float>(base_map::bucket_count());
	}

#else

	inline iterator
	insert(const value_type& __x) // returns no indication if new
	{ return base_map::insert(__x); }

#endif // defined(MAX_ELEM)

	void
	insert(const_pointer __arr, size_type __n)
	{ while (__n--) this->insert(*__arr++); }

	inline std::pair<iterator, iterator>
	equal_range(const key_type& __x) // returns {end, end} if no match
	{ return base_map::equal_range(__x); }

	void
	equal_range(
		std::pair<iterator, iterator>* __rng,
		const key_type* __x,
		size_type __n)
	{ while (__n--) *__rng++ = this->equal_range(*__x++); }

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
	}

	void print_time(void)
	{
	}

}; // class block_multimap

#endif // defined(USE_ACC)

#endif /* _BLOCK_MAP_H */
