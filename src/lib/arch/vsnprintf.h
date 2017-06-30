/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include "arch/IArchString.h"

#if HAVE_VSNPRINTF

#if !defined(ARCH_VSNPRINTF)
#define ARCH_VSNPRINTF vsnprintf
#endif

int
IArchString::vsnprintf (char* str, int size, const char* fmt, va_list ap) {
    int n = ::ARCH_VSNPRINTF (str, size, fmt, ap);
    if (n > size) {
        n = -1;
    }
    return n;
}

#elif SYSAPI_UNIX // !HAVE_VSNPRINTF

#include <stdio.h>

int
IArchString::vsnprintf (char* str, int size, const char* fmt, va_list ap) {
    static FILE* bitbucket = fopen ("/dev/null", "w");
    if (bitbucket == NULL) {
        // uh oh
        if (size > 0) {
            str[0] = '\0';
        }
        return 0;
    } else {
        // count the characters using the bitbucket
        int n = vfprintf (bitbucket, fmt, ap);
        if (n + 1 <= size) {
            // it'll fit so print it into str
            vsprintf (str, fmt, ap);
        }
        return n;
    }
}

#else // !HAVE_VSNPRINTF && !SYSAPI_UNIX

#error vsnprintf not implemented

#endif // !HAVE_VSNPRINTF
