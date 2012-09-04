/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CArch.h"

//
// CArch
//

CArch*					CArch::s_instance = NULL;

CArch::CArch()
{
	assert(s_instance == NULL);
	s_instance = this;
}

CArch::~CArch()
{
#if SYSAPI_WIN32
	CArchMiscWindows::cleanup();
#endif
}

void
CArch::init()
{
	ARCH_NETWORK::init();
#if SYSAPI_WIN32
	ARCH_TASKBAR::init();
	CArchMiscWindows::init();
#endif
}

CArch*
CArch::getInstance()
{
	assert(s_instance != NULL);
	return s_instance;
}
