#ifndef _VCON
#define _VCON

#include <cstddef> // size_t
#include <memory> // std::allocator

#include "config.h"
#include "alloc.h" // NALLOC, SP_NALLOC, chk_alloc


// Simple vector container, uses (SP_)NALLOC

template<class _Tp, bool _Scratchpad = false>
class vcon {
public:
	typedef _Tp value_type;
	// typedef _Alloc allocator_type;

	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;

	typedef size_t size_type;

	value_type *buf;
	size_type entries;
	size_type top_idx;

	vcon()
	: buf(nullptr), entries(0), top_idx(0) {  }

	// doesn't destruct
	~vcon() noexcept
	{ if (buf != nullptr) {if (_Scratchpad) SP_NFREE(buf); else NFREE(buf);} }

	inline size_type
	size() const noexcept
	{ return top_idx; }

	inline void
	resize(size_type __new_size) noexcept
	{ top_idx = __new_size; }

	inline size_type
	capacity() const noexcept
	{ return entries; }

	// doesn't construct
	void
	reserve(size_type __n)
	{
		if (buf != nullptr) {if (_Scratchpad) SP_NFREE(buf); else NFREE(buf);}
		if (_Scratchpad) buf = SP_NALLOC(value_type, __n);
		else buf = NALLOC(value_type, __n);
		chk_alloc(buf, sizeof(value_type)*__n, "Allocate buf in reserve()");
		entries = __n;
		top_idx = 0;
	}

	inline reference
	operator[](size_type __n) noexcept
	{ return buf[__n]; }

	inline const_reference
	operator[](size_type __n) const noexcept
	{ return buf[__n]; }

	inline reference
	back() noexcept
	{ return buf[top_idx-1]; }

	reference
	front() noexcept
	{ return buf[0]; }

	const_reference
	front() const noexcept
	{ return buf[0]; }

	inline const_reference
	back() const noexcept
	{ return buf[top_idx-1]; }

	inline _Tp*
	data() noexcept
	{ return buf; }

	inline const _Tp*
	data() const noexcept
	{ return buf; }

	inline void
	clear() noexcept
	{ top_idx = 0; }

}; // class vcon

#endif // _VCON
