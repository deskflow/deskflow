/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#define WIN32_LEAN_AND_MEAN

#include "CArchMiscWindows.h"
#include "CArchDaemonWindows.h"
#include <windows.h>

//
// CArchMiscWindows
//

bool
CArchMiscWindows::isWindows95Family()
{
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize = sizeof(version);
	if (GetVersionEx(&version) == 0) {
		// cannot determine OS;  assume windows 95 family
		return true;
	}
	return (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

int
CArchMiscWindows::runDaemon(RunFunc runFunc)
{
	return CArchDaemonWindows::runDaemon(runFunc);
}

void
CArchMiscWindows::daemonRunning(bool running)
{
	CArchDaemonWindows::daemonRunning(running);
}

void
CArchMiscWindows::daemonFailed(int result)
{
	CArchDaemonWindows::daemonFailed(result);
}
