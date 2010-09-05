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

#include "CArchConsoleWindows.h"
#include "IArchMultithread.h"
#include "CArch.h"
#include <cstdio>

//
// CArchConsoleWindows
//

CArchThread				CArchConsoleWindows::s_thread = 0;

CArchConsoleWindows::CArchConsoleWindows() :
	m_output(NULL)
{
	s_thread = ARCH->newCurrentThread();

	m_mutex = ARCH->newMutex();

	// dummy write to stderr to create locks in stdio from the main
	// thread.  if we open the console from another thread then we
	// can deadlock in stdio when trying to write from a 3rd thread.
	// writes to stderr without a console don't go anywhere so the
	// user won't notice this.
	fprintf(stderr, "\n");
}

CArchConsoleWindows::~CArchConsoleWindows()
{
	ARCH->closeMutex(m_mutex);
	ARCH->closeThread(s_thread);
}

void
CArchConsoleWindows::openConsole(const char* title)
{
	ARCH->lockMutex(m_mutex);
	if (m_output == NULL) {
		if (AllocConsole()) {
			// get console output handle
			m_output = GetStdHandle(STD_ERROR_HANDLE);

			// set console title
			if (title != NULL) {
				SetConsoleTitle(title);
			}

			// prep console.  windows 95 and its ilk have braindead
			// consoles that can't even resize independently of the
			// buffer size.  use a 25 line buffer for those systems.
			OSVERSIONINFO osInfo;
			COORD size = { 80, 1000 };
			osInfo.dwOSVersionInfoSize = sizeof(osInfo);
			if (GetVersionEx(&osInfo) &&
				osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				size.Y = 25;
			SetConsoleScreenBufferSize(m_output, size);
			SetConsoleTextAttribute(m_output,
									FOREGROUND_RED |
									FOREGROUND_GREEN |
									FOREGROUND_BLUE);

			// catch console signals
			SetConsoleCtrlHandler(&CArchConsoleWindows::signalHandler, TRUE);

			// reopen stderr to point at console
			freopen("con", "w", stderr);
		}
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchConsoleWindows::closeConsole()
{
	ARCH->lockMutex(m_mutex);
	if (m_output != NULL) {
		if (FreeConsole()) {
			m_output = NULL;
		}
	}
	ARCH->unlockMutex(m_mutex);
}

void
CArchConsoleWindows::writeConsole(const char* str)
{
	fprintf(stderr, "%s", str);
}

const char*
CArchConsoleWindows::getNewlineForConsole()
{
	return "\r\n";
}

BOOL WINAPI
CArchConsoleWindows::signalHandler(DWORD ctrlType)
{
	// terminate app and skip remaining handlers
	switch (ctrlType) {
	case CTRL_C_EVENT:
		ARCH->raiseSignal(CArch::kINTERRUPT);
		return TRUE;

	case CTRL_BREAK_EVENT:
		ARCH->raiseSignal(CArch::kTERMINATE);
		return TRUE;

	default:
		ARCH->raiseSignal(CArch::kINTERRUPT);
		return TRUE;
	}
}
