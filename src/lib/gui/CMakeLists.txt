# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2024 Symless Ltd
# SPDX-License-Identifier: MIT

set(target gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(APPLE)
  set (platform_sources
    OSXHelpers.h
    OSXHelpers.mm
  )
endif()

add_library(${target} STATIC
  ${platform_sources}
  Diagnostic.cpp
  Diagnostic.h
  DotEnv.cpp
  DotEnv.h
  EnvVars.h
  FileTail.cpp
  FileTail.h
  Logger.cpp
  Logger.h
  Messages.cpp
  Messages.h
  StringUtils.h
  Styles.h
  config/IServerConfig.h
  config/Screen.cpp
  config/Screen.h
  config/ScreenConfig.cpp
  config/ScreenConfig.h
  config/ScreenList.cpp
  config/ScreenList.h
  core/ClientConnection.cpp
  core/ClientConnection.h
  core/CommandProcess.cpp
  core/CommandProcess.h
  core/CoreProcess.cpp
  core/CoreProcess.h
  core/ServerConnection.cpp
  core/ServerConnection.h
  core/ServerMessage.cpp
  core/ServerMessage.h
  core/WaylandWarnings.cpp
  core/WaylandWarnings.h
  ipc/DaemonIpcClient.cpp
  ipc/DaemonIpcClient.h
  proxy/QProcessProxy.cpp
  proxy/QProcessProxy.h
  tls/TlsCertificate.cpp
  tls/TlsCertificate.h
  tls/TlsUtility.cpp
  tls/TlsUtility.h
)

target_link_libraries(
  ${target}
  common
  platform
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

if(WIN32)
  target_link_libraries(
    ${target}
    base
    arch
    net
  )
endif()
