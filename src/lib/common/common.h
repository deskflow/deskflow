/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// this file should be included, directly or indirectly by every other.

#if HAVE_CONFIG_H
#	include "config.h"

	// don't use poll() on mac
#	if defined(__APPLE__)
#		undef HAVE_POLL
#	endif
#else
	// we may not have run configure on win32
#	if defined(_WIN32)
#		define SYSAPI_WIN32 1
#		define WINAPI_MSWINDOWS 1
#	endif

	// we may not have run configure on OS X
#	if defined(__APPLE__)
#		define SYSAPI_UNIX 1
#		define WINAPI_CARBON 1

#		define HAVE_CXX_BOOL 1
#		define HAVE_CXX_CASTS 1
#		define HAVE_CXX_EXCEPTIONS 1
#		define HAVE_CXX_MUTABLE 1
#		define HAVE_CXX_STDLIB 1
#		define HAVE_GETPWUID_R 1
#		define HAVE_GMTIME_R 1
#		define HAVE_INET_ATON 1
#		define HAVE_INTTYPES_H 1
#		define HAVE_ISTREAM 1
#		define HAVE_MEMORY_H 1
#		define HAVE_NANOSLEEP 1
#		define HAVE_OSTREAM 1
#		define HAVE_POSIX_SIGWAIT 1
#		define HAVE_PTHREAD 1
#		define HAVE_PTHREAD_SIGNAL 1
#		include <sys/types.h>
#		include <sys/socket.h>
#		if defined(_SOCKLEN_T)
#			define HAVE_SOCKLEN_T 1
#		endif
#		define HAVE_SSTREAM 1
#		define HAVE_STDINT_H 1
#		define HAVE_STDLIB_H 1
#		define HAVE_STRINGS_H 1
#		define HAVE_STRING_H 1
#		define HAVE_SYS_SELECT_H 1
#		define HAVE_SYS_SOCKET_H 1
#		define HAVE_SYS_STAT_H 1
#		define HAVE_SYS_TIME_H 1
#		define HAVE_SYS_TYPES_H 1
#		define HAVE_SYS_UTSNAME_H 1
#		define HAVE_UNISTD_H 1
#		define HAVE_VSNPRINTF 1
/* disable this so we can build with the 10.2.8 SDK */
/*#		define HAVE_WCHAR_H 1*/

#		define SELECT_TYPE_ARG1 int
#		define SELECT_TYPE_ARG234 (fd_set *)
#		define SELECT_TYPE_ARG5 (struct timeval *)
#		define SIZEOF_CHAR 1
#		define SIZEOF_INT 4
#		define SIZEOF_LONG 4
#		define SIZEOF_SHORT 2
#		define STDC_HEADERS 1
#		define TIME_WITH_SYS_TIME 1
#		define X_DISPLAY_MISSING 1
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

	// Code Analysis
#	pragma warning(disable: 6011)


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
#	define __FP__
#endif

// define NULL
#include <stddef.h>

// if not c++0x, future proof code by allowing use of nullptr
#ifndef nullptr
#	define nullptr NULL
#endif

// make assert available since we use it a lot
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
	kExitSuccess      = 0, // successful completion
	kExitFailed       = 1, // general failure
	kExitTerminated   = 2, // killed by signal
	kExitArgs         = 3, // bad arguments
	kExitConfig       = 4, // cannot read configuration
	kExitSubscription = 5  // subscription error
};
