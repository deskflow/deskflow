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

#if HAVE_VSNPRINTF

#if !defined(ARCH_VSNPRINTF)
#	define ARCH_VSNPRINTF vsnprintf
#endif

int
ARCH_STRING::vsnprintf(char* str, int size, const char* fmt, va_list ap)
{
	int n = ::ARCH_VSNPRINTF(str, size, fmt, ap);
	if (n > size) {
		n = -1;
	}
	return n;
}

#elif UNIX_LIKE // !HAVE_VSNPRINTF

#include <stdio.h>

int
ARCH_STRING::vsnprintf(char* str, int size, const char* fmt, va_list ap)
{
	static FILE* bitbucket = fopen("/dev/null", "w");
	if (bitbucket == NULL) {
		// uh oh
		if (size > 0) {
			str[0] = '\0';
		}
		return 0;
	}
	else {
		// count the characters using the bitbucket
		int n = vfprintf(bitbucket, fmt, ap);
		if (n + 1 <= size) {
			// it'll fit so print it into str
			vsprintf(str, fmt, ap);
		}
		return n;
	}
}

#else // !HAVE_VSNPRINTF && !UNIX_LIKE

// FIXME
#error vsnprintf not implemented

#endif // !HAVE_VSNPRINTF
