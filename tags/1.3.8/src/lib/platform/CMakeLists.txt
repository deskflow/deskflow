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
		CMSWindowsHookLibraryLoader.h
		IMSWindowsClipboardFacade.h
	)

	set(src
		${inc}
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
		CMSWindowsHookLibraryLoader.cpp
	)
	
	set(inc_hook
		CSynergyHook.h
	)
	
	set(src_hook
		CSynergyHook.cpp
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
)

if (UNIX)
	list(APPEND inc
		../../..
	)
endif()

include_directories(${inc})
add_library(platform STATIC ${src})

if (WIN32)
	add_library(synrgyhk SHARED ${inc_hook} ${src_hook})
	
	# move the synrgyhk.dll (and supporting files) from the lib dir to 
	# the bin dir, so that synergyc and synergys can easily find it.
	# we should leave the other libraries compiling to the lib dir,
	# so that the bin dir remains tidy. the path is relative to the
	# build dir (in this case, that's: build\src\lib\platform).
	add_custom_command(
		TARGET synrgyhk 
		POST_BUILD
		COMMAND xcopy /Y /Q
			..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\synrgyhk.*
			..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
	)
endif()

if (UNIX)
	target_link_libraries(platform synergy)
endif()
