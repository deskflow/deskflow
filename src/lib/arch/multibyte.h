/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "common/common.h"

#include <climits>
#include <cstdlib>
#include <cstring>
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

static inline int mbtowc(wchar_t *dst, const char *src, int n)
{
  *dst = static_cast<wchar_t>(*src);
  return 1;
}

static inline int wctomb(char *dst, wchar_t src)
{
  *dst = static_cast<char>(src);
  return 1;
}

#endif
