# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2009 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(src
	CArch.cpp
	CArchDaemonNone.cpp
	XArch.cpp
)

if (WIN32)

	set(inc
		CArchConsoleWindows.h
		CArchDaemonWindows.h
		CArchFileWindows.h
		CArchLogWindows.h
		CArchMiscWindows.h
		CArchMultithreadWindows.h
		CArchNetworkWinsock.h
		CArchSleepWindows.h
		CArchStringWindows.h
		CArchSystemWindows.h
		CArchTaskBarWindows.h
		CArchTimeWindows.h
		XArchWindows.h
	)

	list(APPEND src
		${inc}
		CArchConsoleWindows.cpp
		CArchDaemonWindows.cpp
		CArchFileWindows.cpp
		CArchLogWindows.cpp
		CArchMiscWindows.cpp
		CArchMultithreadWindows.cpp
		CArchNetworkWinsock.cpp
		CArchSleepWindows.cpp
		CArchStringWindows.cpp
		CArchSystemWindows.cpp
		CArchTaskBarWindows.cpp
		CArchTimeWindows.cpp
		XArchWindows.cpp
	)
	
elseif (UNIX)

	list(APPEND src
		CArchConsoleUnix.cpp
		CArchDaemonUnix.cpp
		CArchFileUnix.cpp
		CArchLogUnix.cpp
		CArchMultithreadPosix.cpp
		CArchNetworkBSD.cpp
		CArchSleepUnix.cpp
		CArchStringUnix.cpp
		CArchSystemUnix.cpp
		CArchTaskBarXWindows.cpp
		CArchTimeUnix.cpp
		XArchUnix.cpp
	)
	
endif()

set(inc
	../base
	../common
	../mt
	../platform
	../synergy
)

if (UNIX)
	list(APPEND inc
		../../..
		../arch
	)
endif()

include_directories(${inc})
add_library(arch STATIC ${src})
