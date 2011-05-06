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

#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif

#define ERROR_ALREADY_RUNNING 1

using namespace std;

void
ensureSingleInstance();

int
main(int argc, char **argv)
{
	// make sure integ tests don't run in parallel.
	int err = ensureSingleInstance();
	if (err != 0)
		return err;

#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

	CArch arch;

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

int
ensureSingleInstance()
{
#if SYSAPI_WIN32

	// get info for current process (we'll use the name later).
	PROCESSENTRY32 selfEntry;
	if (!CArchMiscWindows::getSelfProcessEntry(selfEntry))
		cerr << "could not process info for self "
		<< "(error: " << GetLastError() << ")" << endl;

	// get current task list.
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		cerr << "could not get process snapshot "
			<< "(error: " << GetLastError() << ")" << endl;

	PROCESSENTRY32 entry;
	BOOL gotEntry = Process32First(snapshot, &entry);
	if (!gotEntry)
		cerr << "could not get first process entry "
		<< "(error: " << GetLastError() << ")" << endl;

	while (gotEntry)
	{
		string checkName(entry.szExeFile);

		// if entry has the same name as this process, but is not
		// the current process...
		if ((checkName.find(selfEntry.szExeFile) != string::npos) &&
			(entry.th32ProcessID != selfEntry.th32ProcessID))
		{
			cerr << "error: process already running: " 
				<< entry.th32ProcessID << " -> " << entry.szExeFile << endl;

			return ERROR_ALREADY_RUNNING;
		}

		gotEntry = Process32Next(snapshot, &entry);
	}
#elif SYSAPI_UNIX
	// TODO
#endif

	return 0;
}
