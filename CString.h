#ifndef CSTRING_H
#define CSTRING_H

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
	typedef char _E;
	typedef _E CharT;
	typedef std::allocator<_E> _A;
	typedef std::string _Myt;
	typedef const_iterator _It;

	// same constructors as base class
	CSTRING_DEF_CTOR
	CString(const _Myt& _X) : _Myt(_X) { }
	CString(const _Myt& _X, size_type _P, size_type _M CSTRING_ALLOC1) :
								_Myt(_X, _P, _M CSTRING_ALLOC2) { }
	CString(const _E *_S, size_type _N CSTRING_ALLOC1) :
								_Myt(_S, _N CSTRING_ALLOC2) { }
	CString(const _E *_S CSTRING_ALLOC1) :
								_Myt(_S CSTRING_ALLOC2) { }
	CString(size_type _N, _E _C CSTRING_ALLOC1) :
								_Myt(_N, _C CSTRING_ALLOC2) { }
	CString(_It _F, _It _L CSTRING_ALLOC1) :
								_Myt(_F, _L CSTRING_ALLOC2) { }
};

#endif

