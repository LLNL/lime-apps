#ifndef _BATCH_H
#define _BATCH_H

#include <cstdlib> // rand
#include <cstdint> // uintptr_t
#include <utility> // std::swap

#include "kvop.h" // kvop


enum {C_NOP, C_INSERT, C_FIND, C_ERASE};

template<class _Key, class _Tp, template <class> class _Cont>
class batch : public _Cont<kvop<_Key, _Tp> > {
private:
	using base_type = _Cont<kvop<_Key, _Tp> >;

public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef std::pair<const key_type, mapped_type> value_type;

	typedef typename base_type::size_type size_type;
	typedef typename base_type::reference reference;

	size_type insert_yes, find_yes, erase_yes;
	size_type insert_tot, find_tot, erase_tot;

	batch()
	{ clear_counts(); }

	inline void insert(const value_type& p)
	{
		this->resize(this->size()+1);
		reference eref = this->back();
		eref.set_cmd(C_INSERT);
		eref.set_stat(0);
		eref.set_kvpair(p);
		insert_tot++;
	}

	inline void find(const key_type& k)
	{
		this->resize(this->size()+1);
		reference eref = this->back();
		eref.set_cmd(C_FIND);
		eref.set_stat(0);
		eref.set_key(k);
		eref.set_value(0);
		find_tot++;
	}

	inline void erase(const key_type& k)
	{
		this->resize(this->size()+1);
		reference eref = this->back();
		eref.set_cmd(C_ERASE);
		eref.set_stat(0);
		eref.set_key(k);
		eref.set_value(0);
		erase_tot++;
	}

	// TODO: inc, dec, swap

	// Fisher-Yates shuffle or Knuth shuffle
	void shuffle(void)
	{
		for (size_type i = this->size()-1; i > 0; --i) {
			size_type j = rand() % (i+1);
			std::swap((*this)[i], (*this)[j]);
			// (*this)[i].swap((*this)[j]);
		}
	}

	inline bool is_valid(const mapped_type &v)
	{
		return static_cast<uintptr_t>(v) != 0;
	}

	void clear_counts(void)
	{
		insert_yes = find_yes = erase_yes = 0;
		insert_tot = find_tot = erase_tot = 0;
	}

	void update_counts(void) // make sure this matches with batch execute
	{
		for (size_type i = 0; i < this->size(); i++) {
			switch ((*this)[i].get_cmd()) {
			case C_INSERT:
				if ((*this)[i].get_stat()) insert_yes++;
				break;
			case C_FIND:
				if (is_valid((*this)[i].get_value())) find_yes++;
				break;
			case C_ERASE:
				if ((*this)[i].get_stat()) erase_yes++;
				break;
			}
		}
	}

}; // class batch

#endif // _BATCH_H
