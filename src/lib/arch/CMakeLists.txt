# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Platform Specific Code
if(WIN32)
  set(PLATFORM_CODE
    win32/ArchDaemonWindows.cpp
    win32/ArchDaemonWindows.h
    win32/ArchLogWindows.cpp
    win32/ArchLogWindows.h
    win32/ArchMiscWindows.cpp
    win32/ArchMiscWindows.h
    win32/ArchMultithreadWindows.cpp
    win32/ArchMultithreadWindows.h
    win32/ArchNetworkWinsock.cpp
    win32/ArchNetworkWinsock.h
    win32/XArchWindows.cpp
    win32/XArchWindows.h
  )

elseif(UNIX)
  set(PLATFORM_CODE
    unix/ArchDaemonUnix.cpp
    unix/ArchDaemonUnix.h
    unix/ArchLogUnix.cpp
    unix/ArchLogUnix.h
    unix/ArchMultithreadPosix.cpp
    unix/ArchMultithreadPosix.h
    unix/ArchNetworkBSD.cpp
    unix/ArchNetworkBSD.h
    unix/XArchUnix.cpp
    unix/XArchUnix.h
  )
endif()

add_library(arch STATIC ${PLATFORM_CODE}
  Arch.cpp
  Arch.h
  ArchDaemonNone.cpp
  ArchDaemonNone.h
  ArchException.h
  IArchDaemon.h
  IArchLog.h
  IArchMultithread.h
  IArchNetwork.h
)

target_link_libraries (arch PUBLIC common)

if(UNIX)
  target_link_libraries(arch PUBLIC ${libs})
endif()
