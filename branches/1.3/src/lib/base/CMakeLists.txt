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

set(inc
	CEvent.h
	CEventQueue.h
	CFunctionEventJob.h
	CFunctionJob.h
	CLog.h
	CPriorityQueue.h
	CSimpleEventQueueBuffer.h
	CStopwatch.h
	CString.h
	CStringUtil.h
	CUnicode.h
	IEventJob.h
	IEventQueue.h
	IEventQueueBuffer.h
	IJob.h
	ILogOutputter.h
	LogOutputters.h
	TMethodEventJob.h
	TMethodJob.h
	XBase.h
	ELevel.h
)

set(src
	CEvent.cpp
	CEventQueue.cpp
	CFunctionEventJob.cpp
	CFunctionJob.cpp
	CLog.cpp
	CSimpleEventQueueBuffer.cpp
	CStopwatch.cpp
	CStringUtil.cpp
	CUnicode.cpp
	IEventQueue.cpp
	LogOutputters.cpp
	XBase.cpp
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	../arch
	../common
	../mt
	../synergy
)

if (UNIX)
	list(APPEND inc
		../../..
		../base
	)
endif()

include_directories(${inc})
add_library(base STATIC ${src})
