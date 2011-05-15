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

int
ensureSingleInstance();

#if SYSAPI_UNIX

void
signalHandler(int signal);

void
removeLock();

#endif

int
main(int argc, char **argv)
{
	// make sure integ tests don't run in parallel.
	int err = ensureSingleInstance();
	if (err != 0)
		return err;

#if SYSAPI_UNIX
	// register SIGINT handling (to delete lock file)
	signal(SIGINT, signalHandler);
	atexit(removeLock);
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

	// fail if lock file exists
	struct stat info;
	int statResult = stat(LOCK_FILE, &info);
	if (statResult == 0)
	{
		cerr << "error: lock file exists: " << LOCK_FILE << endl;
		return ERROR_ALREADY_RUNNING;
	}

	// write an empty lock file
	cout << "creating lock: " << LOCK_FILE << endl;

	ofstream stream;
	stream.open(LOCK_FILE);
	if (!stream.is_open())
		cerr << "error: could not create lock" << endl;

	stream << "";
	stream.close();

#endif

	return 0;
}

#if SYSAPI_UNIX
void
signalHandler(int signal)
{
	removeLock();
}

void
removeLock()
{
	// remove lock file so other instances can run.
	cout << "removing lock: " << LOCK_FILE << endl;
	unlink(LOCK_FILE);
}
#endif
