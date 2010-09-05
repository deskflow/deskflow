/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef CMSWINDOWSUTIL_H
#define CMSWINDOWSUTIL_H

#include "CString.h"
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

class CMSWindowsUtil {
public:
	//! Get message string
	/*!
	Gets a string for \p id from the string table of \p instance.
	*/
	static CString		getString(HINSTANCE instance, DWORD id);

	//! Get error string
	/*!
	Gets a system error message for \p error.  If the error cannot be
	found return the string for \p id, replacing ${1} with \p error.
	*/
	static CString		getErrorString(HINSTANCE, DWORD error, DWORD id);
};

#endif
