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

#ifndef COMMON_H
#define COMMON_H

// this file should be included, directly or indirectly by every other.

#if HAVE_CONFIG_H
#	include "config.h"
#else
	// we may not have run configure on win32
#	if defined(_WIN32)
#		define SYSAPI_WIN32 1
#		define WINAPI_MSWINDOWS 1
#	endif
#endif

// VC++ specific
#if (_MSC_VER >= 1200)
	// work around for statement scoping bug
#	define for if (false) { } else for

	// turn off bonehead warnings
#	pragma warning(disable: 4786) // identifier truncated in debug info
#	pragma warning(disable: 4514) // unreferenced inline function removed

	// this one's a little too aggressive
#	pragma warning(disable: 4127) // conditional expression is constant

	// emitted incorrectly under release build in some circumstances
#	if defined(NDEBUG)
#		pragma warning(disable: 4702) // unreachable code
#		pragma warning(disable: 4701) // variable maybe used uninitialized
#	endif
#endif // (_MSC_VER >= 1200)

// VC++ has built-in sized types
#if defined(_MSC_VER)
#	include <wchar.h>
#	define TYPE_OF_SIZE_1 __int8
#	define TYPE_OF_SIZE_2 __int16
#	define TYPE_OF_SIZE_4 __int32
#else
#	define SIZE_OF_CHAR		1
#	define SIZE_OF_SHORT	2
#	define SIZE_OF_INT		4
#	define SIZE_OF_LONG		4
#endif

// FIXME -- including fp.h from Carbon.h causes a undefined symbol error
// on my build system.  the symbol is scalb.  since we don't need any
// math functions we define __FP__, the include guard macro for fp.h, to
// prevent fp.h from being included.
#if defined(__APPLE__)
#define __FP__
#endif

// define NULL
#ifndef NULL
#define NULL 0
#endif

// make assert available since we use it a lot
#include <assert.h>

#endif
