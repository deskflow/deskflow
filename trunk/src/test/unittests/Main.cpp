/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include <gtest/gtest.h>
#include "CArch.h"
#include "CLog.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

int
main(int argc, char **argv)
{
#if SYSAPI_WIN32
	// HACK: shouldn't be needed, but logging fails without this.
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

	CLOG->setFilter(kDEBUG2);

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
