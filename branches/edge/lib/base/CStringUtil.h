/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CSTRINGUTIL_H
#define CSTRINGUTIL_H

#include "CString.h"
#include <stdarg.h>

//! String utilities
/*!
This class provides various functions for string manipulation.
*/
class CStringUtil {
public:
	//! Format positional arguments
	/*!
	Format a string using positional arguments.  fmt has literal
	characters and conversion specifications introduced by `\%':
	- \c\%\%   -- literal `\%'
	- \c\%{n} -- positional element n, n a positive integer, {} are literal

	All arguments in the variable list are const char*.  Positional
	elements are indexed from 1.
	*/
	static CString		format(const char* fmt, ...);

	//! Format positional arguments
	/*!
	Same as format() except takes va_list.
	*/
	static CString		vformat(const char* fmt, va_list);

	//! Print a string using printf-style formatting
	/*!
	Equivalent to printf() except the result is returned as a CString.
	*/
	static CString		print(const char* fmt, ...);

	//! Case-insensitive comparisons
	/*!
	This class provides case-insensitve comparison functions.
	*/
	class CaselessCmp {
	  public:
		//! Same as less()
		bool			operator()(const CString& a, const CString& b) const;

		//! Returns true iff \c a is lexicographically less than \c b
		static bool		less(const CString& a, const CString& b);

		//! Returns true iff \c a is lexicographically equal to \c b
		static bool		equal(const CString& a, const CString& b);

		//! Returns true iff \c a is lexicographically less than \c b
		static bool		cmpLess(const CString::value_type& a,
							const CString::value_type& b);

		//! Returns true iff \c a is lexicographically equal to \c b
		static bool		cmpEqual(const CString::value_type& a,
							const CString::value_type& b);
	};
};

#endif

