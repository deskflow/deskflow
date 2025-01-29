# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Platform Specific Code
if(WIN32)
  set(PLATFORM_CODE
    win32/ArchConsoleWindows.cpp
    win32/ArchConsoleWindows.h
    win32/ArchDaemonWindows.cpp
    win32/ArchDaemonWindows.h
    win32/ArchFileWindows.cpp
    win32/ArchFileWindows.h
    win32/ArchLogWindows.cpp
    win32/ArchLogWindows.h
    win32/ArchMiscWindows.cpp
    win32/ArchMiscWindows.h
    win32/ArchMultithreadWindows.cpp
    win32/ArchMultithreadWindows.h
    win32/ArchNetworkWinsock.cpp
    win32/ArchNetworkWinsock.h
    win32/ArchSleepWindows.cpp
    win32/ArchSleepWindows.h
    win32/ArchStringWindows.cpp
    win32/ArchStringWindows.h
    win32/ArchSystemWindows.cpp
    win32/ArchSystemWindows.h
    win32/ArchTimeWindows.cpp
    win32/ArchTimeWindows.h
    win32/XArchWindows.cpp
    win32/XArchWindows.h
  )

elseif(UNIX)
  set(PLATFORM_CODE
    unix/ArchConsoleUnix.cpp
    unix/ArchConsoleUnix.h
    unix/ArchDaemonUnix.cpp
    unix/ArchDaemonUnix.h
    unix/ArchFileUnix.cpp
    unix/ArchFileUnix.h
    unix/ArchLogUnix.cpp
    unix/ArchLogUnix.h
    unix/ArchMultithreadPosix.cpp
    unix/ArchMultithreadPosix.h
    unix/ArchNetworkBSD.cpp
    unix/ArchNetworkBSD.h
    unix/ArchSleepUnix.cpp
    unix/ArchSleepUnix.h
    unix/ArchStringUnix.cpp
    unix/ArchStringUnix.h
    unix/ArchSystemUnix.cpp
    unix/ArchSystemUnix.h
    unix/ArchTimeUnix.cpp
    unix/ArchTimeUnix.h
    unix/XArchUnix.cpp
    unix/XArchUnix.h
  )
endif()

add_library(arch STATIC ${PLATFORM_CODE}
  Arch.cpp
  Arch.h
  ArchConsoleStd.cpp
  ArchConsoleStd.h
  ArchDaemonNone.cpp
  ArchDaemonNone.h
  IArchConsole.h
  IArchDaemon.h
  IArchFile.h
  IArchLog.h
  IArchMultithread.h
  IArchNetwork.h
  IArchSleep.h
  IArchString.cpp
  IArchString.h
  IArchSystem.h
  IArchTime.h
  multibyte.h
  XArch.h
)

if(UNIX)
  target_link_libraries(arch ${CMAKE_DL_LIBS} ${libs})
  if(NOT APPLE)
    find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus)
    target_link_libraries(arch Qt6::DBus)
  endif()
endif()
