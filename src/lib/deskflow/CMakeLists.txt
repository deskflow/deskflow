# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2025 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

find_package(CLI11 QUIET)
if(CLI11_FOUND)
  message(STATUS "CLI11 [System] Version: ${CLI11_VERSION}")
  set(cli11_lib CLI11::CLI11)
else()
  #Be sure to update the version and MD5 together
  set(CLI11_VERSION 2.5.0)
  set(CLI11_MD5 0d3d6898febe3b82fc47b3d608b1ae95)
  file(
    DOWNLOAD "https://github.com/CLIUtils/CLI11/releases/download/v${CLI11_VERSION}/CLI11.hpp"
    "${CMAKE_BINARY_DIR}/include/CLI/CLI.hpp"
    EXPECTED_MD5 ${CLI11_MD5}
  )
  set(cli11_inc_dir "${CMAKE_BINARY_DIR}/include")
  message(STATUS "CLI11 [Downloaded] Version: ${CLI11_VERSION}")
  message(STATUS "CLI11 INC_DIR: ${cli11_inc_dir}")
endif()

####################Start Making Library#########################
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
  ArgParser.cpp
  ArgParser.h
  ArgsBase.h
  Chunk.cpp
  Chunk.h
  ClientApp.cpp
  ClientApp.h
  ClientArgs.cpp
  ClientArgs.h
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
  IAppUtil.h
  IClient.h
  IClipboard.cpp
  IClipboard.h
  IKeyState.cpp
  IKeyState.h
  INode.h
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
  ServerArgs.cpp
  ServerArgs.h
  StreamChunker.cpp
  StreamChunker.h
  languages/LanguageManager.cpp
  languages/LanguageManager.h
  ipc/DaemonIpcServer.cpp
  ipc/DaemonIpcServer.h
)

target_link_libraries(${lib_name} PUBLIC Qt6::Core Qt6::Network)

if(WIN32)
    target_link_libraries(${lib_name} PRIVATE ${cli11_lib})
endif()

target_include_directories(
  ${lib_name}
    PRIVATE
      ${cli11_inc_dir}
)

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
    ${cli11_lib}
  )

  if(NOT APPLE)
    target_link_libraries(${lib_name} PRIVATE Qt6::Xml)
  endif()
endif()
