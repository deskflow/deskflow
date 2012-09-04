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
		CMSWindowsRelauncher.h
		CMSWindowsHookLibraryLoader.h
		IMSWindowsClipboardFacade.h
		CMSWindowsDebugOutputter.h
		CMSWindowsXInput.h
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
		CMSWindowsRelauncher.cpp
		CMSWindowsHookLibraryLoader.cpp
		CMSWindowsDebugOutputter.cpp
	)
	
	set(inc_hook
		CSynergyHook.h
	)
	
	set(src_hook
		CSynergyHook.cpp
	)
  
  if (GAME_DEVICE_SUPPORT)

    list(APPEND inc
      CMSWindowsXInput.h
    )

    list(APPEND src
      CMSWindowsXInput.cpp
    )
      
  
    set(inc_xinhook
      XInputHook.h
      HookDLL.h
    )
    
    set(src_xinhook
      XInputHook.cpp
      HookDLL.cpp
    )

    set(inc_xinproxy13
      XInput13.h
      XInputProxy13.h
    )

    set(src_xinproxy13
      XInputProxy13.cpp
    )
  
  endif()
  
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

	# copy the dlls (and supporting files) from the lib dir to 
	# the bin dir, so that synergyc and synergys can easily find them.
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

	if (GAME_DEVICE_SUPPORT)
		include_directories($ENV{DXSDK_DIR}/Include)
		add_library(synxinhk SHARED ${inc_xinhook} ${src_xinhook})

		add_custom_command(
			TARGET synxinhk
			POST_BUILD
			COMMAND xcopy /Y /Q
				..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\synxinhk.*
				..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
			)

		# synergy xinput1_3.dll proxy
		include_directories($ENV{DXSDK_DIR}/Include)
		add_library(sxinpx13 SHARED ${inc_xinproxy13} ${src_xinproxy13})
		target_link_libraries(sxinpx13 synxinhk)
		add_custom_command(
			TARGET sxinpx13
			POST_BUILD
			COMMAND xcopy /Y /Q
				..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\sxinpx13.*
				..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
			)
	endif()
endif()

if (UNIX)
	target_link_libraries(platform io net ipc synergy ${libs})
endif()
