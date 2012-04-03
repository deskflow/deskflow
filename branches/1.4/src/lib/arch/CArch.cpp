/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "common.h"
#include "CArch.h"
#include "CLog.h"

//
// CArch
//

CArch*					CArch::s_instance = NULL;

CArch::CArch()
{
}

CArch::~CArch()
{
}

void
CArch::init()
{
	// initialization that requires ARCH is done here.
	ARCH_TASKBAR::init();
	ARCH_NETWORK::init();

#if SYSAPI_WIN32
	CArchMiscWindows::init();
#endif
}

CArch*
CArch::getInstance()
{
	if (s_instance == NULL) {
		s_instance = new CArch();
		s_instance->init();
	}

	return s_instance;
}
