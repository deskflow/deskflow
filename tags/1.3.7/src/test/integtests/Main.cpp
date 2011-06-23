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

#include <iostream>
#include <gtest/gtest.h>
#include "CArch.h"
#include "CLog.h"
#include "LogOutputters.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#if SYSAPI_UNIX
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#define LOCK_FILE "/tmp/integtests.lock"
#endif

#define ERROR_ALREADY_RUNNING 1

using namespace std;

#if SYSAPI_UNIX

void
signalHandler(int signal);

void
removeLock();

#endif

int
main(int argc, char **argv)
{
#if SYSAPI_WIN32
	if (CArchMiscWindows::isWindows95Family())
	{
		std::cerr << "Windows 95 family not supported." << std::endl;
		return 1;
	}
#endif

	CArch arch;

#if SYSAPI_WIN32
	// only add std output logger for windows (unix
	// already outputs to standard streams).
	CStdLogOutputter stdLogOutputter;
	CLOG->insert(&stdLogOutputter, true);
#endif

	CLOG->setFilter(kDEBUG2);

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
