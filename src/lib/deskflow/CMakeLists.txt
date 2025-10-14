# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2025 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

set(lib_name app)

# arch
if(WIN32)
  set(PLATFORM_CODE
    win32/AppUtilWindows.cpp
    win32/AppUtilWindows.h
  )
elseif(UNIX)
  set(PLATFORM_CODE
    unix/AppUtilUnix.cpp
    unix/AppUtilUnix.h
    unix/DeskflowXkbKeyboard.cpp
    unix/DeskflowXkbKeyboard.h
    unix/ISO639Table.h
    unix/X11LayoutsParser.cpp
    unix/X11LayoutsParser.h
  )
endif()

add_library(${lib_name} STATIC ${PLATFORM_CODE}
  App.cpp
  App.h
  AppUtil.cpp
  AppUtil.h
  Chunk.cpp
  Chunk.h
  ClientApp.cpp
  ClientApp.h
  ClipboardTypes.h
  Clipboard.cpp
  Clipboard.h
  ClipboardChunk.cpp
  ClipboardChunk.h
  DaemonApp.cpp
  DaemonApp.h
  DeskflowException.cpp
  DeskflowException.h
  DisplayInvalidException.h
  IApp.h
  IClient.h
  IClipboard.cpp
  IClipboard.h
  IKeyState.cpp
  IKeyState.h
  IPlatformScreen.cpp
  IPlatformScreen.h
  IPrimaryScreen.cpp
  IPrimaryScreen.h
  IScreen.h
  IScreenSaver.h
  ISecondaryScreen.h
  KeyTypes.cpp
  KeyTypes.h
  KeyMap.cpp
  KeyMap.h
  KeyState.cpp
  KeyState.h
  MouseTypes.h
  OptionTypes.h
  PacketStreamFilter.cpp
  PacketStreamFilter.h
  PlatformScreen.cpp
  PlatformScreen.h
  ProtocolTypes.cpp
  ProtocolTypes.h
  ProtocolUtil.cpp
  ProtocolUtil.h
  Screen.cpp
  Screen.h
  ScreenException.cpp
  ScreenException.h
  ServerApp.cpp
  ServerApp.h
  StreamChunker.cpp
  StreamChunker.h
  languages/LanguageManager.cpp
  languages/LanguageManager.h
  ipc/DaemonIpcServer.cpp
  ipc/DaemonIpcServer.h
)

target_link_libraries(${lib_name} PUBLIC common Qt6::Core Qt6::Network)

if(UNIX)
  target_link_libraries(
    ${lib_name}
    PRIVATE
    arch
    client
    net
    base
    platform
    mt
    server
  )

  if(NOT APPLE)
    target_link_libraries(${lib_name} PRIVATE Qt6::Xml)
  endif()
endif()
