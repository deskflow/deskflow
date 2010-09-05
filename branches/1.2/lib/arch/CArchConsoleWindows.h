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

#ifndef CARCHCONSOLEWINDOWS_H
#define CARCHCONSOLEWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "IArchConsole.h"
#include "IArchMultithread.h"
#include <windows.h>

#define ARCH_CONSOLE CArchConsoleWindows

//! Win32 implementation of IArchConsole
class CArchConsoleWindows : public IArchConsole {
public:
	CArchConsoleWindows();
	virtual ~CArchConsoleWindows();

	// IArchConsole overrides
	virtual void		openConsole(const char* title);
	virtual void		closeConsole();
	virtual void		writeConsole(const char*);
	virtual const char*	getNewlineForConsole();

private:
	static BOOL WINAPI	signalHandler(DWORD);

private:
	static CArchThread	s_thread;

	CArchMutex			m_mutex;
	HANDLE				m_output;
};

#endif
