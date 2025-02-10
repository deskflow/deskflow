# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

if(UNIX AND NOT APPLE)
  pkg_check_modules(LIBEI REQUIRED QUIET "libei-1.0 >= ${REQUIRED_LIBEI_VERSION}")
  message(STATUS "libei version: ${LIBEI_VERSION}")

  pkg_check_modules(LIBPORTAL REQUIRED QUIET "libportal >= ${REQUIRED_LIBPORTAL_VERSION}")
  message(STATUS "libportal version: ${LIBPORTAL_VERSION}")
endif()

if(WIN32)
  set(PLATFORM_SOURCES
    dfwhook.h
    IMSWindowsClipboardFacade.h
    MSWindowsClipboard.cpp
    MSWindowsClipboard.h
    MSWindowsClipboardAnyTextConverter.cpp
    MSWindowsClipboardAnyTextConverter.h
    MSWindowsClipboardBitmapConverter.cpp
    MSWindowsClipboardBitmapConverter.h
    MSWindowsClipboardFacade.cpp
    MSWindowsClipboardFacade.h
    MSWindowsClipboardHTMLConverter.cpp
    MSWindowsClipboardHTMLConverter.h
    MSWindowsClipboardTextConverter.cpp
    MSWindowsClipboardTextConverter.h
    MSWindowsClipboardUTF16Converter.cpp
    MSWindowsClipboardUTF16Converter.h
    MSWindowsDebugOutputter.cpp
    MSWindowsDebugOutputter.h
    MSWindowsDesks.cpp
    MSWindowsDesks.h
    MSWindowsDropTarget.cpp
    MSWindowsDropTarget.h
    MSWindowsEventQueueBuffer.cpp
    MSWindowsEventQueueBuffer.h
    MSWindowsHook.cpp
    MSWindowsHook.h
    MSWindowsKeyState.cpp
    MSWindowsKeyState.h
    MSWindowsPowerManager.cpp
    MSWindowsPowerManager.h
    MSWindowsProcess.cpp
    MSWindowsProcess.h
    MSWindowsScreen.cpp
    MSWindowsScreen.h
    MSWindowsScreenSaver.cpp
    MSWindowsScreenSaver.h
    MSWindowsSession.cpp
    MSWindowsSession.h
    MSWindowsUtil.cpp
    MSWindowsUtil.h
    MSWindowsWatchdog.cpp
    MSWindowsWatchdog.h
  )
elseif(APPLE)
  set(PLATFORM_SOURCES
    IOSXKeyResource.cpp
    IOSXKeyResource.h
    OSXAutoTypes.h
    OSXClipboard.cpp
    OSXClipboard.h
    OSXClipboardAnyBitmapConverter.cpp
    OSXClipboardAnyBitmapConverter.h
    OSXClipboardAnyTextConverter.cpp
    OSXClipboardAnyTextConverter.h
    OSXClipboardBMPConverter.cpp
    OSXClipboardBMPConverter.h
    OSXClipboardHTMLConverter.cpp
    OSXClipboardHTMLConverter.h
    OSXClipboardTextConverter.cpp
    OSXClipboardTextConverter.h
    OSXClipboardUTF8Converter.cpp
    OSXClipboardUTF8Converter.h
    OSXClipboardUTF16Converter.cpp
    OSXClipboardUTF16Converter.h
    OSXDragSimulator.h
    OSXDragSimulator.m
    OSXDragView.h
    OSXDragView.m
    OSXEventQueueBuffer.cpp
    OSXEventQueueBuffer.h
    OSXKeyState.cpp
    OSXKeyState.h
    OSXMediaKeySupport.h
    OSXMediaKeySupport.m
    OSXPasteboardPeeker.h
    OSXPasteboardPeeker.m
    OSXPowerManager.cpp
    OSXPowerManager.h
    OSXScreen.h
    OSXScreen.mm
    OSXScreenSaver.cpp
    OSXScreenSaver.h
    OSXScreenSaverControl.h
    OSXScreenSaverUtil.h
    OSXScreenSaverUtil.m
    OSXUchrKeyResource.cpp
    OSXUchrKeyResource.h
  )
elseif(UNIX)
  set(PLATFORM_SOURCES
    XWindowsClipboard.cpp
    XWindowsClipboard.h
    XWindowsClipboardAnyBitmapConverter.cpp
    XWindowsClipboardAnyBitmapConverter.h
    XWindowsClipboardBMPConverter.cpp
    XWindowsClipboardBMPConverter.h
    XWindowsClipboardHTMLConverter.cpp
    XWindowsClipboardHTMLConverter.h
    XWindowsClipboardTextConverter.cpp
    XWindowsClipboardTextConverter.h
    XWindowsClipboardUCS2Converter.cpp
    XWindowsClipboardUCS2Converter.h
    XWindowsClipboardUTF8Converter.cpp
    XWindowsClipboardUTF8Converter.h
    XWindowsEventQueueBuffer.cpp
    XWindowsEventQueueBuffer.h
    XWindowsKeyState.cpp
    XWindowsKeyState.h
    XWindowsPowerManager.cpp
    XWindowsPowerManager.h
    XWindowsScreen.cpp
    XWindowsScreen.h
    XWindowsScreenSaver.cpp
    XWindowsScreenSaver.h
    XWindowsUtil.cpp
    XWindowsUtil.h
  )

  if(LIBEI_FOUND)
    list(APPEND PLATFORM_SOURCES
      EiEventQueueBuffer.cpp
      EiEventQueueBuffer.h
      EiKeyState.cpp
      EiKeyState.h
      EiScreen.cpp
      EiScreen.h
    )
    # The Portal sources also require EI.
    if(LIBPORTAL_FOUND)
      list(APPEND PLATFORM_SOURCES
        PortalInputCapture.cpp
        PortalInputCapture.h
        PortalRemoteDesktop.cpp
        PortalRemoteDesktop.h
      )
    endif()
  endif()
endif()

if(APPLE)
  list(APPEND inc /System/Library/Frameworks)
endif()
include_directories(${inc})

# wayland.h is included to check for wayland support
add_library(platform STATIC ${PLATFORM_SOURCES} wayland.h)

target_link_libraries(platform client ${libs})

macro(link_wayland_libs)
  target_compile_definitions(platform PUBLIC WINAPI_LIBEI WINAPI_LIBPORTAL HAVE_LIBPORTAL_INPUTCAPTURE)

  target_link_libraries(platform ${LIBXKBCOMMON_LINK_LIBRARIES}
                        ${GLIB2_LINK_LIBRARIES} ${LIBM_LIBRARIES}
                        ${LIBEI_LINK_LIBRARIES} ${LIBPORTAL_LINK_LIBRARIES}
  )

  target_include_directories(platform PUBLIC ${LIBEI_INCLUDE_DIRS} ${LIBPORTAL_INCLUDE_DIRS})

endmacro()

if(UNIX)
  target_link_libraries(
    platform
    io
    net
    app
    client
    ${libs})

  if(NOT APPLE)
    find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus)
    target_link_libraries(platform Qt6::DBus)

    link_wayland_libs()
  endif()
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(platform ${COCOA_LIBRARY})
endif()
