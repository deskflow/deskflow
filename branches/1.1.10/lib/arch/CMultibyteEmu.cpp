/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CArch.h"
#include <string.h>
#if HAVE_WCHAR_H
#	include <wchar.h>
#elif __APPLE__
	// wtf?  Darwin puts mbtowc() et al. in stdlib
#	include <stdlib.h>
#else
	// platform apparently has no wchar_t support.  provide dummy
	// implementations.  hopefully at least the C++ compiler has
	// a built-in wchar_t type.
#	undef HAVE_MBSINIT

static inline
int
mbtowc(wchar_t* dst, const char* src, int n)
{
	*dst = static_cast<wchar_t>(*src);
	return 1;
}

static inline
int
wctomb(char* dst, wchar_t src)
{
	*dst = static_cast<char>(src);
	return 1;
}

#endif

class CArchMBStateImpl {
public:
#if HAVE_MBSINIT
	mbstate_t			m_mbstate;
#else
	int					m_mbstate;	// dummy data
#endif
};

//
// use C library non-reentrant multibyte conversion with mutex
//

static CArchMutex		s_mutex;

static
void
initMB()
{
	s_mutex = ARCH->newMutex();
}

static
void
cleanMB()
{
	ARCH->closeMutex(s_mutex);
}

void
ARCH_STRING::initMBState(CArchMBState state)
{
	memset(&state->m_mbstate, 0, sizeof(state->m_mbstate));
}

bool
ARCH_STRING::isInitMBState(CArchMBState state)
{
	// if we're using this file mbsinit() probably doesn't exist
	// but use it if it does
#if HAVE_MBSINIT
	return (mbsinit(&state->m_mbstate) != 0);
#else
	return true;
#endif
}

int
ARCH_STRING::convMBToWC(wchar_t* dst, const char* src, int n, CArchMBState)
{
	wchar_t dummy;
	ARCH->lockMutex(s_mutex);
	int result = mbtowc(dst != NULL ? dst : &dummy, src, n);
	ARCH->unlockMutex(s_mutex);
	if (result < 0)
		return -1;
	else
		return result;
}

int
ARCH_STRING::convWCToMB(char* dst, wchar_t src, CArchMBState)
{
	char dummy[MB_LEN_MAX];
	ARCH->lockMutex(s_mutex);
	int n = wctomb(dst != NULL ? dst : dummy, src);
	ARCH->unlockMutex(s_mutex);
	return n;
}
