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

#else // !HAVE_VSNPRINTF

// FIXME
#error vsnprintf not implemented

#endif // !HAVE_VSNPRINTF
