
/* TODO: consider batch being just exec */
/* FIXME: constructors and destructors */
/*
  TODO: update this comment
  The main differences of batch_multimap from unordered_multimap are:
  1) Block oriented methods (e.g., insert, equal_range, find) have been
     added that operate on arrays of elements.
  2) A maximum number of elements per key can be specified with MAX_ELEM.
  3) USE_ACC Only: Values are stored in result arrays without being paired
     with keys. Hence, equal_range() returns a mapped_type pointer instead
     of a pair of iterators. The first mapped_type entry is a count of values
     that follow.
*/

#ifndef _BATCH_MAP_H
#define _BATCH_MAP_H

#include <cstddef> // size_t
#include <utility> // std::pair

#include "kvop.h" // class kvop
#include "batch.h" // operation enum

#define MAX_ELEM 7 // comment out for performance testing

#if defined(USE_ACC)
#error "Accelerator configuration under construction"

#define CHUNK_SZ 4
#include "KVstore.hpp"

/* these are global to avoid overhead at runtime on the stack */
tick_t tA, tB, tC, tD;
unsigned long long tsetup, tfill, tdrain, tcache;

/* * * * * * * * * * Map, Accelerator * * * * * * * * * */

// See C:\cygwin\lib\gcc\i686-pc-cygwin\5.4.0\include\c++\bits\hashtable_policy.h:300,1420

template<class _Key, class _Tp>
struct clocal_iterator {
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;
	typedef const value_type* const_pointer;

	typedef typename KVstore<key_type, mapped_type>::slot_s slot_s;

	slot_s *slot;
	std::pair<_Key, _Tp> val;

	inline void update_val(void)
	{
		if (slot == nullptr) return;
		if (slot->probes == 0) { slot = nullptr; return; }
		val.first = slot->key;
		val.second = slot->value;
	}

	clocal_iterator() noexcept
	: slot(nullptr) { }

	explicit
	clocal_iterator(slot_s *__p) noexcept
	: slot(__p) { update_val(); }

	const_pointer
	operator->() const noexcept
	{
		if (slot == nullptr) return nullptr;
		return reinterpret_cast<const_pointer>(&val);
	}

	clocal_iterator
	operator++(int) noexcept
	{
		clocal_iterator __tmp(*this);
		slot = nullptr;
		return __tmp;
	}

};

// See C:\cygwin\lib\gcc\i686-pc-cygwin\5.4.0\include\c++\bits\hashtable_policy.h:315,1497
template<class _Key, class _Tp>
inline bool
operator==(
	const clocal_iterator<_Key, _Tp>& __x,
	const clocal_iterator<_Key, _Tp>& __y)
{
	return __x.slot == __y.slot;
}

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class batch_map {
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
	// See C:\cygwin\lib\gcc\i686-pc-cygwin\5.4.0\include\c++\bits\hashtable_policy.h:1679
	using const_local_iterator = clocal_iterator<key_type, mapped_type>;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	KVstore<key_type, mapped_type> acc; // Key-Value Accelerator
	typedef typename KVstore<key_type, mapped_type>::slot_s slot_s;

	size_type
	count(const key_type& __x) // const
	{
		slot_s *slot = acc._search(__x);
		return (slot) ? 1 : 0;
	}

	size_type
	bucket_size(size_type __n) const
	{
		slot_s *slot = acc.data_base + __n;
		return (slot->probes) ? 1 : 0;
	}

	const_local_iterator
	cbegin(size_type __n) const
	{
		slot_s *slot = acc.data_base + __n;
		return const_local_iterator(slot);
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

#if 1 // begin insert
	void
	insert(const_pointer __arr, size_type __n)
	{
#if 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
		mtcp(XREG_CP15_CACHE_SIZE_SEL, 0);
#endif // end USE_STREAM && __ARM_ARCH
		while (__n--) insert(*__arr++);
#if 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
		dsb();
		// tget(tA);
		// acc.cache_flush();
		// tget(tB);
		// tinc(tcache, tdiff(tB,tA));
#endif // end USE_STREAM && __ARM_ARCH
	}
#else // else insert
	// TODO: version with drain only, modify args to drain(kvpair)?
	// NOTE: Be careful of overlapping probes even when keys are sorted.
	// Probes will need to be tested for overlap to prevent a hazard.
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
#endif // end insert

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

#if 0 // begin find
	void
	find(mapped_type* __fnd, const key_type* __x, size_type __n)
	{
		while (__n--) *__fnd++ = find(*__x++);
	}
#else // else find
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
#endif // end find

	size_type
	erase(const key_type& __k)
	{
		return 0; /* TODO: */
	}

	inline size_type
	size() const noexcept
	{
		return acc.elements;
	}

	inline size_type
	bucket_count() const noexcept
	{
		// acc.topsearch needed to iterate through all buckets
		return acc.data_len + acc.topsearch - 1;
	}

	inline float
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

	typedef kvop<_Key, _Tp> entry_type;

	void exec(entry_type* __ent, size_type __n)
	{
		// TODO: batch_map::exec() for accelerator
	}

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		printf("table addr: %p\n", acc.data_base);
		printf("table size: %lu\n", (ul_t)acc.data_len*sizeof(slot_s));
		printf("size: %lu\n", (ul_t)size());
		printf("load_factor (elem): %f\n", load_factor());
		printf("bucket_count: %lu\n", (ul_t)bucket_count());
		printf("max_psl: %lu\n", (ul_t)acc.topsearch);
#if defined(PSL_HISTO)
		{
			unsigned *hbin = (unsigned *)malloc(sizeof(unsigned)*acc.topsearch);
			acc.psl_histo(hbin);
			printf("PSL Count\n");
			for (unsigned i = 0; i < acc.topsearch; i++) {
				printf("%u %u\n", i+1, hbin[i]);
			}
			printf("PSL End\n");
			free(hbin);
		}
#endif // end PSL_HISTO
#if defined(SPILL)
		printf("spill size: %lu\n", (ul_t)acc.spill.size());
#endif // end SPILL
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

}; // class batch_map

/* * * * * * * * * * MultiMap, Accelerator * * * * * * * * * */

template<class _Key, class _Tp>
struct clocal_iterator_mm {
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;
	typedef const value_type* const_pointer;

	typedef typename KVstore<key_type, mapped_type*>::slot_s slot_s;

	slot_s *slot;
	unsigned idx;
	std::pair<_Key, _Tp> val;

	inline void update_val(void)
	{
		if (slot == nullptr) return;
		if (slot->probes == 0 ||
		    slot->value == nullptr ||
		    idx > slot->value[0]) { slot = nullptr; return; }
		val.first = slot->key;
		val.second = slot->value[idx];
	}

	clocal_iterator_mm() noexcept
	: slot(nullptr), idx(0) { }

	explicit
	clocal_iterator_mm(slot_s *__p) noexcept
	: slot(__p), idx(1) { update_val(); }

	const_pointer
	operator->() const noexcept
	{
		if (slot == nullptr) return nullptr;
		return reinterpret_cast<const_pointer>(&val);
	}

	clocal_iterator_mm
	operator++(int) noexcept
	{
		clocal_iterator_mm __tmp(*this);
		if (slot != nullptr) {
			idx++;
			update_val();
		}
		return __tmp;
	}

};

template<class _Key, class _Tp>
inline bool
operator==(
	const clocal_iterator_mm<_Key, _Tp>& __x,
	const clocal_iterator_mm<_Key, _Tp>& __y)
{
	return __x.slot == __y.slot;
}

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class batch_multimap {
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
	using const_local_iterator = clocal_iterator_mm<key_type, mapped_type>;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	size_type element_count; // total number of elements in container
	size_type maxelem; // maximum number of elements per key

	KVstore<key_type, mapped_type*> acc; // Key-Value Accelerator
	typedef typename KVstore<key_type, mapped_type*>::slot_s slot_s;

	size_type
	count(const key_type& __x) // const
	{
		slot_s *slot = acc._search(__x);
		return (slot && slot->value) ? slot->value[0] : 0;
	}

	size_type
	bucket_size(size_type __n) const
	{
		slot_s *slot = acc.data_base + __n;
		return (slot->probes && slot->value) ? slot->value[0] : 0;
	}

	const_local_iterator
	cbegin(size_type __n) const
	{
		slot_s *slot = acc.data_base + __n;
		return const_local_iterator(slot);
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

	iterator
	insert(const value_type& __x) // returns no indication if new
	{
		mapped_type *mptr = nullptr;
		slot_s *slot;

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

#if 1 // begin insert
	void
	insert(const_pointer __arr, size_type __n)
	{
#if 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
		mtcp(XREG_CP15_CACHE_SIZE_SEL, 0);
#endif // end USE_STREAM && __ARM_ARCH
		while (__n--) insert(*__arr++);
#if 0 && defined(USE_STREAM) && defined(__ARM_ARCH)
		dsb();
		// tget(tA);
		// acc.cache_flush();
		// tget(tB);
		// tinc(tcache, tdiff(tB,tA));
#endif // end USE_STREAM && __ARM_ARCH
	}
#else // else insert
	// NOTE: Be careful of overlapping probes even when keys are sorted.
	// Probes will need to be tested for overlap to prevent a hazard.
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
		CACHE_RECV(acc, value, sizeof(mapped_type*)*__n);
		tget(tE);
	}
#endif // end insert

	inline mapped_type*
	equal_range(const key_type& __x) // returns nullptr if no match
	{
		mapped_type* mptr;
		acc._get(__x, mptr);
		return mptr;
	}

#if 0 // begin equal_range
	void
	equal_range(
		mapped_type** __rng,
		const key_type* __x,
		size_type __n)
	{
		while (__n--) *__rng++ = equal_range(*__x++);
	}
#else // else equal_range
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
#endif // end equal_range

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

	inline size_type
	size() const noexcept
	{
		return element_count;
	}

	inline size_type
	bucket_count() const noexcept
	{
		// acc.topsearch needed to iterate through all buckets
		return acc.data_len + acc.topsearch - 1;
	}

	inline float
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

	typedef kvop<_Key, _Tp> entry_type;

	void exec(entry_type* __ent, size_type __n)
	{
		// TODO: batch_multimap::exec() for accelerator
	}

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		printf("table addr: %p\n", acc.data_base);
		printf("table size: %lu\n", (ul_t)acc.data_len*sizeof(slot_s));
		printf("size: %lu unique: %lu duplicates: %lu %.2f%%\n",
			(ul_t)size(), (ul_t)acc.elements, (ul_t)(size()-acc.elements),
			(double)(size()-acc.elements)/size()*100.0);
		printf("load_factor (elem): %f\n", load_factor());
		printf("load_factor (keys): %f\n", (double)acc.elements/bucket_count());
		printf("bucket_count: %lu\n", (ul_t)bucket_count());
		printf("max_elem_per_key: %lu\n", (ul_t)maxelem);
		printf("max_psl: %lu\n", (ul_t)acc.topsearch);
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

}; // class batch_multimap

#else // not USE_ACC

#include <unordered_map>

/* * * * * * * * * * Map, Standard * * * * * * * * * */

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class batch_map
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

	// * * * * * * * * * * non-standard methods * * * * * * * * * * //

	typedef kvop<_Key, _Tp> entry_type;

	void exec(entry_type* __ent, size_type __n)
	{
#if defined(_OPENMP)
		#pragma omp parallel for
#endif // end _OPENMP
		for (size_type i = 0; i < __n; i++) {
			switch (__ent[i].get_cmd()) {
			case C_INSERT: {
				// res: pair<new or matching element, true if new>
				std::pair<iterator, bool> res = base_map::insert(__ent[i].get_kvpair());
				__ent[i].set_stat(res.second);
				} break;
			case C_FIND: {
				// res: const_iterator to element if found, end() otherwise
				const_iterator res = base_map::find(__ent[i].get_key());
				if (res != base_map::end()) {
					__ent[i].set_value(res->second);
					__ent[i].set_stat(1);
				}
				} break;
			case C_ERASE: {
				// res: size_type number of elements erased
				__ent[i].set_stat(base_map::erase(__ent[i].get_key()));
				} break;
			}
		}
	}

	void print_stats(void)
	{
		typedef unsigned long ul_t;
		size_type max_elem_per_bucket = 0;

		// gather stats in one pass through table
		for (size_type i = 0; i < this->bucket_count(); ++i) {
			size_type bkt_sz = this->bucket_size(i);
			if (bkt_sz > max_elem_per_bucket) max_elem_per_bucket = bkt_sz;
		}
		printf("size: %lu\n", (ul_t)this->size());
		printf("load_factor (elem): %f\n", this->load_factor());
		printf("bucket_count: %lu\n", (ul_t)this->bucket_count());
		printf("max_elem_per_bucket: %lu\n", (ul_t)max_elem_per_bucket);
	}

	void clear_time(void)
	{
	}

	void print_time(void)
	{
	}

}; // class batch_map

/* * * * * * * * * * MultiMap, Standard * * * * * * * * * */

template<class _Key, class _Tp> //,
	// class _Hash = hash<_Key>,
	// class _Pred = std::equal_to<_Key>,
	// class _Alloc = std::allocator<std::pair<const _Key, _Tp> >
class batch_multimap
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

#else // not MAX_ELEM

	inline iterator
	insert(const value_type& __x) // returns no indication if new
	{ return base_map::insert(__x); }

#endif // end MAX_ELEM

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
	{
#if defined(_OPENMP)
		#pragma omp parallel for
		for (size_type i = 0; i < __n; i++)
			__rng[i] = this->equal_range(__x[i]);
#else // not _OPENMP
		while (__n--) *__rng++ = this->equal_range(*__x++);
#endif // end _OPENMP
	}

	inline iterator
	find(const key_type& __x) // returns end if no match
	{ return base_map::find(__x); }

	void
	find(iterator* __fnd, const key_type* __x, size_type __n)
	{ while (__n--) *__fnd++ = this->find(*__x++); }

	// * * * * * * * * * * non-standard methods * * * * * * * * * * //

	typedef kvop<_Key, _Tp> entry_type;

	void exec(entry_type* __ent, size_type __n)
	{
#if defined(_OPENMP)
		#pragma omp parallel for
#endif // end _OPENMP
		for (size_type i = 0; i < __n; i++) {
			switch (__ent[i].get_cmd()) {
			case C_INSERT: {
				// res: iterator to the newly inserted element
				iterator res = base_map::insert(__ent[i].get_kvpair());
				__ent[i].set_stat(1);
				} break;
			case C_FIND: {
				// res: const_iterator to element if found, end() otherwise
				const_iterator res = base_map::find(__ent[i].get_key());
				if (res != base_map::end()) {
					__ent[i].set_value(res->second);
					__ent[i].set_stat(1);
				}
				} break;
			case C_ERASE: {
				// res: size_type number of elements erased
				__ent[i].set_stat(base_map::erase(__ent[i].get_key()));
				} break;
			}
		}
	}

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
			size_type bkt_sz = this->bucket_size(i);
			if (bkt_sz > max_elem_per_bucket) max_elem_per_bucket = bkt_sz;
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
		}
		printf("size: %lu unique: %lu duplicates: %lu %.2f%%\n",
			(ul_t)this->size(), (ul_t)keycnt, (ul_t)(this->size()-keycnt),
			(double)(this->size()-keycnt)/this->size()*100.0);
		printf("load_factor (elem): %f\n", this->load_factor());
		printf("load_factor (keys): %f\n", (double)keycnt/this->bucket_count());
		printf("bucket_count: %lu\n", (ul_t)this->bucket_count());
		printf("max_elem_per_bucket: %lu\n", (ul_t)max_elem_per_bucket);
		printf("max_elem_per_key: %lu\n", (ul_t)max_elem_per_key);
		printf("max_psl: %lu\n", (ul_t)max_psl);
	}

	void clear_time(void)
	{
	}

	void print_time(void)
	{
	}

}; // class batch_multimap

#endif // end USE_ACC

#endif /* end _BATCH_MAP_H */

/* NOTES:
 * Skip multimap implementation for now?
 * The exec interface to the KV store does not have to be the same as STL
   (insert, equal_range, erase) which interacts through KV pairs and iterators.
#*If the map type is a reference or a pointer then old values need to be
   returned on insert or delete for memory management purposes.
 * Modifying an indirect map type will require multiple phases:
   1) find the entry (possibly save the slot),
   2) chase indirection to the item
   3) do the update.
 * Multimap inserts through a link require 3 phases,
   find link, update (allocate) link, write link
 * Saving the slot index from a previous phase could speedup the write-back
   phase for a multimap if the slot index was used instead of the key to
   write the link. Recomputing the hash would not be necessary.
 * The batch container could be implemented with a stream interface. This
   would allow entries of variable length, but then indexed access would
   not be allowed. With fixed length entries, OpenMP or multiple threads
   can be used to parallelize operations on a batch not done by the
   accelerator.
*/

/* TODO:
 * Consider batch_multimap as a wrapper around batch_map.
   - How to configure the entry_type? - Is a different type really needed?
 * In exec don't use status for return values from STD functions unless
   it matches what would be returned from hardware (e.g. new on insert).
   - Use returned last value on insert to determine new or not.
 * In kvop, consider generic storage and multiple accessors for different
   types (of returns from various commands) sharing the same storage.
 * Consider using an input and an output command buffer as arguments
   in batch exec. May still use one buffer as both in and out.
 * Can the multiple passes or stages needed for multimap all be
   implemented in hardware?
 * Figure out how to extend the batch exec interface to support a multimap.
   - Need a return value (map_type*) that is different from the KV pair.
   - Need to configure the entry_type with a return value.
     ~ Pass return_type as a template parameter.
#  - Use union for second element of KV pair to hold the value on insert
     and the map_type* on return or use a union around the whole KV pair.
#  - Use a bit in the status field to indicate valid fields and their types
     in a kvop entry.
   - How to structure the classes and inheritance so that the entry_type
     (with return_type) flows from the selection of multimap.
     ~ Export entry_type from batch_multimap?
 * Figure out how to return pair<iterator,iterator> from equal_range()
   when executing a batch on a multimap.
   - Don't expect a pair from the batch exec method, use a map_type*.
*/
