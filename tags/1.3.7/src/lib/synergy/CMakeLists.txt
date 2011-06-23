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
	CClipboard.h
	CKeyMap.h
	CKeyState.h
	CPacketStreamFilter.h
	CPlatformScreen.h
	CProtocolUtil.h
	CScreen.h
	ClipboardTypes.h
	IClient.h
	IClipboard.h
	IKeyState.h
	IPlatformScreen.h
	IPrimaryScreen.h
	IScreen.h
	IScreenSaver.h
	ISecondaryScreen.h
	KeyTypes.h
	MouseTypes.h
	OptionTypes.h
	ProtocolTypes.h
	XScreen.h
	XSynergy.h
)

set(src
	CClipboard.cpp
	CKeyMap.cpp
	CKeyState.cpp
	CPacketStreamFilter.cpp
	CPlatformScreen.cpp
	CProtocolUtil.cpp
	CScreen.cpp
	IClipboard.cpp
	IKeyState.cpp
	IPrimaryScreen.cpp
	IScreen.cpp
	KeyTypes.cpp
	ProtocolTypes.cpp
	XScreen.cpp
	XSynergy.cpp
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	../arch
	../base
	../client
	../common
	../io
	../mt
	../net
	../platform
	../server
	../synergy
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_library(synergy STATIC ${src})

if (UNIX)
	target_link_libraries(synergy arch client net base platform mt server)
endif()
