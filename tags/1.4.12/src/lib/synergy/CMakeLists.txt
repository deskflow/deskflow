# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
# Copyright (C) 2009 Nick Bolton
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
	CClientTaskBarReceiver.h
	CServerTaskBarReceiver.h
	CPortableTaskBarReceiver.h
	CApp.h
	CClientApp.h
	CServerApp.h
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
	GameDeviceTypes.h
	CDaemonApp.h
	CAppUtil.h
	CArgsBase.h
	IAppUtil.h
	CEventGameDevice.h
	CVncClient.h
	CCryptoStream.h
	CCryptoMode.h
	ECryptoMode.h
	CCryptoOptions.h
)

set(src
	CClientTaskBarReceiver.cpp
	CServerTaskBarReceiver.cpp
	CPortableTaskBarReceiver.cpp
	CApp.cpp
	CClientApp.cpp
	CServerApp.cpp
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
	ISecondaryScreen.cpp
	CDaemonApp.cpp
	CAppUtil.cpp
	CArgsBase.cpp
	CEventGameDevice.cpp
	CVncClient.cpp
	CGameDevice.cpp
	CCryptoStream.cpp
	CCryptoMode.cpp
	CCryptoOptions.cpp
)

if (WIN32)
	list(APPEND inc
		CAppUtilWindows.h
		CGameDevice.h
	)	

	list(APPEND src
		${inc}
		CAppUtilWindows.cpp
	)
elseif(UNIX)
	list(APPEND src
	  	CAppUtilUnix.cpp
	)
endif()

set(inc
	../arch
	../base
	../client
	../common
	../io
	../ipc
	../mt
	../net
	../platform
	../server
	../synergy
	../..
	../../../tools
)

if (UNIX)
	list(APPEND inc
		../../..
	)
elseif (WIN32)
	list(APPEND inc
		../../vnc/win
	)
endif()

include_directories(${inc})
add_library(synergy STATIC ${src})

if (UNIX)
	target_link_libraries(synergy arch client ipc net base platform mt server cryptopp)
endif()
