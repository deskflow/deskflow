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
#include "stdset.h"
#include <windows.h>

//! Miscellaneous win32 functions.
class CArchMiscWindows {
public:
	enum EValueType {
		kUNKNOWN,
		kNO_VALUE,
		kUINT,
		kSTRING,
		kBINARY
	};
	enum EBusyModes {
		kIDLE   = 0x0000,
		kSYSTEM  = 0x0001,
		kDISPLAY = 0x0002
	};

	typedef int			(*RunFunc)(void);

	//! Initialize
	static void			init();

	//! Test if windows 95, et al.
	/*!
	Returns true iff the platform is win95/98/me.
	*/
	static bool			isWindows95Family();

	//! Test if windows 95, et al.
	/*!
	Returns true iff the platform is win98 or win2k or higher (i.e.
	not windows 95 or windows NT).
	*/
	static bool			isWindowsModern();

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

	//! Get daemon quit message
	/*!
	Delegates to CArchDaemonWindows.
	*/
	static UINT			getDaemonQuitMessage();

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

	//! Get type of value
	static EValueType	typeOfValue(HKEY key, const TCHAR* name);

	//! Set a string value in the registry
	static void			setValue(HKEY key, const TCHAR* name,
							const std::string& value);

	//! Set a DWORD value in the registry
	static void			setValue(HKEY key, const TCHAR* name, DWORD value);

	//! Set a BINARY value in the registry
	/*!
	Sets the \p name value of \p key to \p value.data().
	*/
	static void			setValueBinary(HKEY key, const TCHAR* name,
							const std::string& value);

	//! Read a string value from the registry
	static std::string	readValueString(HKEY, const TCHAR* name);

	//! Read a DWORD value from the registry
	static DWORD		readValueInt(HKEY, const TCHAR* name);

	//! Read a BINARY value from the registry
	static std::string	readValueBinary(HKEY, const TCHAR* name);

	//! Add a dialog
	static void			addDialog(HWND);

	//! Remove a dialog
	static void			removeDialog(HWND);

	//! Process dialog message
	/*!
	Checks if the message is destined for a dialog.  If so the message
	is passed to the dialog and returns true, otherwise returns false.
	*/
	static bool			processDialog(MSG*);

	//! Disable power saving
	static void			addBusyState(DWORD busyModes);

	//! Enable power saving
	static void			removeBusyState(DWORD busyModes);

private:
	//! Read a string value from the registry
	static std::string	readBinaryOrString(HKEY, const TCHAR* name, DWORD type);

	//! Set thread busy state
	static void			setThreadExecutionState(DWORD);

	static DWORD WINAPI	dummySetThreadExecutionState(DWORD);

private:
	typedef std::set<HWND> CDialogs;
	typedef DWORD (WINAPI *STES_t)(DWORD);

	static CDialogs*	s_dialogs;
	static DWORD		s_busyState;
	static STES_t		s_stes;
};

#endif
