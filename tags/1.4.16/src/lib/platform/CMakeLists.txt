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

if (WIN32)

	set(inc
		CMSWindowsClipboard.h
		CMSWindowsClipboardAnyTextConverter.h
		CMSWindowsClipboardBitmapConverter.h
		CMSWindowsClipboardHTMLConverter.h
		CMSWindowsClipboardTextConverter.h
		CMSWindowsClipboardUTF16Converter.h
		CMSWindowsDesks.h
		CMSWindowsEventQueueBuffer.h
		CMSWindowsKeyState.h
		CMSWindowsScreen.h
		CMSWindowsScreenSaver.h
		CMSWindowsUtil.h
		CMSWindowsWatchdog.h
		CMSWindowsHookLibraryLoader.h
		IMSWindowsClipboardFacade.h
		CMSWindowsDebugOutputter.h
		CMSWindowsSession.h
	)

	set(src
		CMSWindowsClipboard.cpp
		CMSWindowsClipboardFacade.cpp
		CMSWindowsClipboardAnyTextConverter.cpp
		CMSWindowsClipboardBitmapConverter.cpp
		CMSWindowsClipboardHTMLConverter.cpp
		CMSWindowsClipboardTextConverter.cpp
		CMSWindowsClipboardUTF16Converter.cpp
		CMSWindowsDesks.cpp
		CMSWindowsEventQueueBuffer.cpp
		CMSWindowsKeyState.cpp
		CMSWindowsScreen.cpp
		CMSWindowsScreenSaver.cpp
		CMSWindowsUtil.cpp
		CMSWindowsWatchdog.cpp
		CMSWindowsHookLibraryLoader.cpp
		CMSWindowsDebugOutputter.cpp
		CMSWindowsSession.cpp
	)

	list(APPEND src
		${inc}
	)

elseif (APPLE)

	set(src
		COSXClipboard.cpp
		COSXClipboardAnyTextConverter.cpp
		COSXClipboardTextConverter.cpp
		COSXClipboardUTF16Converter.cpp
		COSXEventQueueBuffer.cpp
		COSXKeyState.cpp
		COSXScreen.cpp
		COSXScreenSaver.cpp
		COSXScreenSaverUtil.m
		COSXPasteboardPeeker.m
		COSXDragSimulator.m
		COSXDragView.m
	)
	
elseif (UNIX)

	set(src
		CXWindowsClipboard.cpp
		CXWindowsClipboardAnyBitmapConverter.cpp
		CXWindowsClipboardBMPConverter.cpp
		CXWindowsClipboardHTMLConverter.cpp
		CXWindowsClipboardTextConverter.cpp
		CXWindowsClipboardUCS2Converter.cpp
		CXWindowsClipboardUTF8Converter.cpp
		CXWindowsEventQueueBuffer.cpp
		CXWindowsKeyState.cpp
		CXWindowsScreen.cpp
		CXWindowsScreenSaver.cpp
		CXWindowsUtil.cpp
	)
	
endif()

set(inc
	../arch
	../base
	../common
	../mt
	../synergy
	../ipc
	../net
	../io
	../server
	../client
	../synwinhk
	../synwinxt
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

if (APPLE)
	list(APPEND inc
		/System/Library/Frameworks
	)
endif()

include_directories(${inc})
add_library(platform STATIC ${src})
target_link_libraries(platform client ${libs})

if (UNIX)
	target_link_libraries(platform io net ipc synergy client ${libs})
endif()

if (APPLE)
	FIND_LIBRARY(COCOA_LIBRARY Cocoa)
	target_link_libraries(platform ${COCOA_LIBRARY})
endif()
