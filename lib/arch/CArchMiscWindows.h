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

#ifndef CARCHMISCWINDOWS_H
#define CARCHMISCWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "common.h"
#include "stdstring.h"
#include <windows.h>

//! Miscellaneous win32 functions.
class CArchMiscWindows {
public:
	typedef int			(*RunFunc)(void);

	//! Test if windows 95, et al.
	/*!
	Returns true iff the platform is win95/98/me.
	*/
	static bool			isWindows95Family();

	//! Run the daemon
	/*!
	Delegates to CArchDaemonWindows.
	*/
	static int			runDaemon(RunFunc runFunc);

	//! Indicate daemon is in main loop
	/*!
	Delegates to CArchDaemonWindows.
	*/
	static void			daemonRunning(bool running);

	//! Indicate failure of running daemon
	/*!
	Delegates to CArchDaemonWindows.
	*/
	static void			daemonFailed(int result);

	//! Open and return a registry key, closing the parent key
	static HKEY			openKey(HKEY parent, const TCHAR* child);

	//! Open and return a registry key, closing the parent key
	static HKEY			openKey(HKEY parent, const TCHAR* const* keyPath);

	//! Close a key
	static void			closeKey(HKEY);

	//! Delete a key (which should have no subkeys)
	static void			deleteKey(HKEY parent, const TCHAR* name);

	//! Delete a value
	static void			deleteValue(HKEY parent, const TCHAR* name);

	//! Test if a value exists
	static bool			hasValue(HKEY key, const TCHAR* name);

	//! Set a string value in the registry
	static void			setValue(HKEY key, const TCHAR* name,
							const std::string& value);

	//! Set a DWORD value in the registry
	static void			setValue(HKEY key, const TCHAR* name, DWORD value);

	//! Read a string value from the registry
	static std::string	readValueString(HKEY, const TCHAR* name);

	//! Read a DWORD value from the registry
	static DWORD		readValueInt(HKEY, const TCHAR* name);
};

#endif
