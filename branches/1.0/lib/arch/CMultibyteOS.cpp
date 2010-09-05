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

#include <string.h>
#include <cwchar>

class CArchMBStateImpl {
public:
	mbstate_t			m_mbstate;
};

//
// use C library reentrant multibyte conversion
//

static
void
initMB()
{
	// do nothing
}

static
void
cleanMB()
{
	// do nothing
}

void
ARCH_STRING::initMBState(CArchMBState state)
{
	memset(&state->m_mbstate, 0, sizeof(state->m_mbstate));
}

bool
ARCH_STRING::isInitMBState(CArchMBState state)
{
	return (mbsinit(&state->m_mbstate) != 0);
}

int
ARCH_STRING::convMBToWC(wchar_t* dst, const char* src,
 				 int n, CArchMBState state)
{
	wchar_t dummy;
	return static_cast<int>(mbrtowc(dst != NULL ? dst : &dummy,
							src, static_cast<size_t>(n), &state->m_mbstate));
}

int
ARCH_STRING::convWCToMB(char* dst, wchar_t src, CArchMBState state)
{
	char dummy[MB_LEN_MAX];
	return static_cast<int>(wcrtomb(dst != NULL ? dst : dummy,
							src, &state->m_mbstate));
}
