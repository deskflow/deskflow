/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "common.h"

// include appropriate architecture implementation
#if WINDOWS_LIKE
#	include "CArchMiscWindows.cpp"
#	include "CArchConsoleWindows.cpp"
#	include "CArchDaemonWindows.cpp"
#	include "CArchFileWindows.cpp"
#	include "CArchLogWindows.cpp"
#	include "CArchMultithreadWindows.cpp"
#	include "CArchNetworkWinsock.cpp"
#	include "CArchSleepWindows.cpp"
#	include "CArchStringWindows.cpp"
#	include "CArchTaskBarWindows.cpp"
#	include "CArchTimeWindows.cpp"
#	include "XArchWindows.cpp"
#elif UNIX_LIKE
#	include "CArchConsoleUnix.cpp"
#	include "CArchDaemonUnix.cpp"
#	include "CArchFileUnix.cpp"
#	include "CArchLogUnix.cpp"
#	if HAVE_PTHREAD
#		include "CArchMultithreadPosix.cpp"
#	endif
#	include "CArchNetworkBSD.cpp"
#	include "CArchSleepUnix.cpp"
#	include "CArchStringUnix.cpp"
#	include "CArchTaskBarXWindows.cpp"
#	include "CArchTimeUnix.cpp"
#	include "XArchUnix.cpp"
#endif
