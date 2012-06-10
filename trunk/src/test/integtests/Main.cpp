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
#include <fstream>
#include <gtest/gtest.h>
#include "CArch.h"
#include "CLog.h"

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#define LOCK_TIMEOUT 30

using namespace std;

void lock(string lockFile);
void unlock(string lockFile);

int
main(int argc, char **argv)
{
#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

	string lockFile;
	for (int i = 0; i < argc; i++) {
		if (string(argv[i]).compare("--lock-file") == 0) {
			lockFile = argv[i + 1];
		}
	}

	if (!lockFile.empty()) {
		lock(lockFile);
	}

	CLOG->setFilter(kDEBUG2);

	testing::InitGoogleTest(&argc, argv);

	int result = RUN_ALL_TESTS();

	if (!lockFile.empty()) {
		unlock(lockFile);
	}

	return result;
}

void
lock(string lockFile)
{
	double start = ARCH->time();
	
	// keep checking until timeout is reached.
	while ((ARCH->time() - start) < LOCK_TIMEOUT) {

		ifstream is(lockFile.c_str());
		bool noLock = !is;
		is.close();

		if (noLock) {
			break;
		}

		// check every second if file has gone.
		ARCH->sleep(1);
	}

	// write empty lock file.
	ofstream os(lockFile.c_str());
	os.close();
}

void
unlock(string lockFile) 
{
	remove(lockFile.c_str());
}
