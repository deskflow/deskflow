#ifndef CSTRING_H
#define CSTRING_H

#include "stdpre.h"
#include <string>
#include "stdpost.h"

// use to get appropriate type for string constants.  it depends on
// the internal representation type of CString.
#define _CS(_x) 		_x

typedef std::string CString;

class CStringUtil {
public:
	class CaselessCmp {
	  public:
		bool			operator()(const CString&, const CString&) const;
		static bool		less(const CString&, const CString&);
		static bool		equal(const CString&, const CString&);
		static bool		cmpLess(const CString::value_type&,
							const CString::value_type&);
		static bool		cmpEqual(const CString::value_type&,
							const CString::value_type&);
	};
};

#endif

