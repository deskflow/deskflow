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

#ifndef CMSWINDOWSDESKTOP_H
#define CMSWINDOWSDESKTOP_H

#include "CString.h"
#include <windows.h>

//! Encapsulate Microsoft Windows desktop
class CMSWindowsDesktop {
public:
	//! Open the input desktop
	/*!
	Opens the input desktop.  The caller must close the desktop.
	*/
	static HDESK		openInputDesktop();

	//! Close a desktop
	/*!
	Closes the given desktop.
	*/
	static void			closeDesktop(HDESK);

	//! Change current desktop
	/*!
	Changes the calling thread's desktop, return true iff successful.
	The call will fail if the calling thread has any windows or a hooks
	on the current desktop.
	*/
	static bool			setDesktop(HDESK);

	//! Get the desktop's name.
	/*!
	Returns the current desktop's name.  Returns a constant string
	on 95/98/Me.
	*/
	static CString		getDesktopName(HDESK);
};

#endif
