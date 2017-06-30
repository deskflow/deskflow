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

#pragma once

#include "common/common.h"
#include "arch/Arch.h"

#include <climits>
#include <cstring>
#include <cstdlib>
#if HAVE_LOCALE_H
#include <locale.h>
#endif
#if HAVE_WCHAR_H || defined(_MSC_VER)
#include <wchar.h>
#elif __APPLE__
// wtf?  Darwin puts mbtowc() et al. in stdlib
#include <cstdlib>
#else
// platform apparently has no wchar_t support.  provide dummy
// implementations.  hopefully at least the C++ compiler has
// a built-in wchar_t type.

static inline int
mbtowc (wchar_t* dst, const char* src, int n) {
    *dst = static_cast<wchar_t> (*src);
    return 1;
}

static inline int
wctomb (char* dst, wchar_t src) {
    *dst = static_cast<char> (src);
    return 1;
}

#endif
