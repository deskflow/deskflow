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

#ifndef IMSWINDOWSSCREENEVENTHANDLER_H
#define IMSWINDOWSSCREENEVENTHANDLER_H

#include "IScreenEventHandler.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//! MS Windows screen event handler interface
class IMSWindowsScreenEventHandler : public IScreenEventHandler {
public:
	//! @name manipulators
	//@{

	//! Notify of window creation
	/*!
	This is called after the window is created.
	*/
	virtual void		postCreateWindow(HWND) = 0;

	//! Notify of window destruction
	/*!
	This is called before the window is destroyed.
	*/
	virtual void		preDestroyWindow(HWND) = 0;

	//! Notify of newly accessible desktop
	/*!
	This is called when the user switched from an inaccessible desktop
	to an accessible desktop.
	*/
	virtual void		onAccessibleDesktop() = 0;

	//@}

	// IScreenEventHandler overrides
	virtual void		onScreensaver(bool activated) = 0;
	virtual bool		onPreDispatch(const CEvent* event) = 0;
	virtual bool		onEvent(CEvent* event) = 0;
	virtual SInt32		getJumpZoneSize() const = 0;
};

#endif
