#ifndef CSTRING_H
#define CSTRING_H

#include <stdarg.h>
#include "stdpre.h"
#include <string>
#include "stdpost.h"

typedef std::string CString;

class CStringUtil {
public:
	// format a string using positional arguments.  fmt has literal
	// characters and conversion specifications introduced by `%':
	//   %%    literal `%'
	//   %{n}  positional element n, n a positive integer, {} are literal
	// all arguments in the variable list are const char*.  positional
	// elements are indexed from 1.
	static CString		format(const char* fmt, ...);
	static CString		vformat(const char* fmt, va_list);

	// print a string using printf-style formatting
	static CString		print(const char* fmt, ...);
	static CString		vprint(const char* fmt, va_list);

	// like print but print into a given buffer.  if the resulting string
	// will not fit into the buffer then a new buffer is allocated and
	// returned, otherwise the input buffer is returned.  the caller must
	// delete[] the returned buffer if is not the passed-in buffer.
	//
	// prefix and suffix must be >= 0.  exactly prefix characters and
	// at least suffix characters are available in the buffer before
	// and after the printed string, respectively.  bufferLength is the
	// length of buffer and should not be adjusted by the caller to
	// account for prefix or suffix.
	static char*		vsprint(char* buffer, int bufferLength,
							int prefix, int suffix, const char* fmt, va_list);

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

