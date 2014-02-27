/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// TODO: consider whether or not to use either encapsulation (as below)
// or inheritance (as it is now) for the ARCH stuff.
//
// case for encapsulation:
// pros:
//  - compiler errors for missing pv implementations are not absolutely bonkers.
//  - function names don't have to be so verbose.
//  - easier to understand and debug.
//  - ctors in IArch implementations can call other implementations.
// cons:
//  - slightly more code for calls to ARCH.
//  - you'll have to modify each ARCH call.
//
// also, we may want to consider making each encapsulated
// class lazy-loaded so that apps like the daemon don't load
// stuff when they don't need it.

#ifndef CARCH_H
#define CARCH_H

#include "common.h"

#if SYSAPI_WIN32
#	include "CArchConsoleWindows.h"
#	include "CArchDaemonWindows.h"
#	include "CArchFileWindows.h"
#	include "CArchLogWindows.h"
#	include "CArchMiscWindows.h"
#	include "CArchMultithreadWindows.h"
#	include "CArchNetworkWinsock.h"
#	include "CArchSleepWindows.h"
#	include "CArchStringWindows.h"
#	include "CArchSystemWindows.h"
#	include "CArchTaskBarWindows.h"
#	include "CArchTimeWindows.h"
#	include "CArchPluginWindows.h"
#	include "CArchInternetWindows.h"
#elif SYSAPI_UNIX
#	include "CArchConsoleUnix.h"
#	include "CArchDaemonUnix.h"
#	include "CArchFileUnix.h"
#	include "CArchLogUnix.h"
#	if HAVE_PTHREAD
#		include "CArchMultithreadPosix.h"
#	endif
#	include "CArchNetworkBSD.h"
#	include "CArchSleepUnix.h"
#	include "CArchStringUnix.h"
#	include "CArchSystemUnix.h"
#	include "CArchTaskBarXWindows.h"
#	include "CArchTimeUnix.h"
#	include "CArchPluginUnix.h"
#	include "CArchInternetUnix.h"
#endif

/*!
\def ARCH
This macro evaluates to the singleton CArch object.
*/
#define ARCH	(CArch::getInstance())

//! Delegating implementation of architecture dependent interfaces
/*!
This class is a centralized interface to all architecture dependent
interface implementations (except miscellaneous functions).  It
instantiates an implementation of each interface and delegates calls
to each method to those implementations.  Clients should use the
\c ARCH macro to access this object.  Clients must also instantiate
exactly one of these objects before attempting to call any method,
typically at the beginning of \c main().
*/
class CArch : public ARCH_CONSOLE,
				public ARCH_DAEMON,
				public ARCH_FILE,
				public ARCH_LOG,
				public ARCH_MULTITHREAD,
				public ARCH_NETWORK,
				public ARCH_SLEEP,
				public ARCH_STRING,
				public ARCH_SYSTEM,
				public ARCH_TASKBAR,
				public ARCH_TIME {
public:
	CArch();
	virtual ~CArch();

	//! Call init on other arch classes.
	/*!
	Some arch classes depend on others to exist first. When init is called
	these clases will have ARCH available for use.
	*/
	virtual void init();

	//
	// accessors
	//

	//! Return the singleton instance
	/*!
	The client must have instantiated exactly once CArch object before
	calling this function.
	*/
	static CArch*		getInstance();

	ARCH_PLUGIN&		plugin() const { return (ARCH_PLUGIN&)m_plugin; }
	ARCH_INTERNET&		internet() const { return (ARCH_INTERNET&)m_internet; }

private:
	static CArch*		s_instance;
	ARCH_PLUGIN			m_plugin;
	ARCH_INTERNET		m_internet;
};

//! Convenience object to lock/unlock an arch mutex
class CArchMutexLock {
public:
	CArchMutexLock(CArchMutex mutex) : m_mutex(mutex)
	{
		ARCH->lockMutex(m_mutex);
	}
	~CArchMutexLock()
	{
		ARCH->unlockMutex(m_mutex);
	}

private:
	CArchMutex			m_mutex;
};

#endif
