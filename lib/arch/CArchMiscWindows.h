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

#include "common.h"

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
};

#endif
