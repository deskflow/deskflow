#ifndef CSTRING_H
#define CSTRING_H

#include "common.h"
#include <string>

#ifndef CSTRING_DEF_CTOR
#define CSTRING_ALLOC1
#define CSTRING_ALLOC2
#define CSTRING_DEF_CTOR CString() : _Myt() { }
#endif

// use to get appropriate type for string constants.  it depends on
// the internal representation type of CString.
#define _CS(_x) 		_x

class CString : public std::string {
public:
	typedef char _e;
	typedef _e CharT;
	typedef std::allocator<_e> _a;
	typedef std::string _Myt;
	typedef const_iterator _It;

	// same constructors as base class
	CSTRING_DEF_CTOR
	CString(const _Myt& _x) : _Myt(_x) { }
	CString(const _Myt& _x, size_type _p, size_type _m CSTRING_ALLOC1) :
								_Myt(_x, _p, _m CSTRING_ALLOC2) { }
	CString(const _e *_s, size_type _n CSTRING_ALLOC1) :
								_Myt(_s, _n CSTRING_ALLOC2) { }
	CString(const _e *_s CSTRING_ALLOC1) :
								_Myt(_s CSTRING_ALLOC2) { }
	CString(size_type _n, _e _c CSTRING_ALLOC1) :
								_Myt(_n, _c CSTRING_ALLOC2) { }
	CString(_It _f, _It _l CSTRING_ALLOC1) :
								_Myt(_f, _l CSTRING_ALLOC2) { }
};

#endif

