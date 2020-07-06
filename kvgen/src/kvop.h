#ifndef _KVOP_H
#define _KVOP_H

#include <utility> // std::pair

template<class _Key, class _Tp>
class kvop {
public:
	typedef _Key key_type;
	typedef _Tp mapped_type;
	typedef long index_type;
	typedef mapped_type* result_type;
	typedef std::pair<key_type, mapped_type> kvpair_type;
	struct kvpair_st {
		key_type first;
		mapped_type second;
	};

	// kvop() = default; // may cause problem if pair<> has a string

	inline unsigned int get_cmd(void) {return cmd;}
	inline unsigned int get_stat(void) {return stat;}
	inline key_type     get_key(void) {return kvpair.first;}
	// inline index_type   get_index(void) {return idx;}
	inline mapped_type  get_value(void) {return kvpair.second;}
	// inline kvpair_type& get_kvpair(void) {return *reinterpret_cast<kvpair_type*>(&kvpair);}
	inline kvpair_type& get_kvpair(void) {return kvpair;}

	inline void set_cmd(unsigned int c) {cmd = c;}
	inline void set_stat(unsigned int s) {stat = s;}
	inline void set_key(key_type k) {kvpair.first = k;}
	// inline void set_index(index_type i) {idx = i;}
	inline void set_value(mapped_type v) {kvpair.second = v;}
	// inline void set_kvpair(const kvpair_type& p) {kvpair = *reinterpret_cast<const kvpair_st*>(&p);}
	inline void set_kvpair(const kvpair_type& p) {kvpair = p;}

	// TODO: provide a swap method?

private:
	unsigned int cmd;
	unsigned int stat;
	kvpair_type kvpair;
	// union {
		// kvpair_st kvpair;
		// struct {
			// union {
				// key_type key;
				// index_type idx;
			// };
			// union {
				// mapped_type map;
				// result_type res;
			// };
		// };
	// };
}; // class kvop

#endif // _KVOP_H

/* TODO:
 * Restrict types to default constructible?
 * Rethink reusing an array of entries for input and output.
 * - Separate entry type (result) for output array?
 * - Next stage would then require copying keys
 * - Leave key in place and restrict result and mapped type to simple types?
*/
