# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(ipc STATIC
  IpcClient.cpp
  IpcClient.h
  IpcClientProxy.cpp
  IpcClientProxy.h
  IpcLogOutputter.cpp
  IpcLogOutputter.h
  IpcMessage.cpp
  IpcMessage.h
  IpcServer.cpp
  IpcServer.h
  IpcServerProxy.cpp
  IpcServerProxy.h
  IpcSettingMessage.cpp
  IpcSettingMessage.h
)

if(UNIX)
  target_link_libraries(
    ipc
    arch
    base
    mt
    io
    net
    app)
endif()
