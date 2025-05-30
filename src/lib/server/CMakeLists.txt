# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(server STATIC
  BaseClientProxy.cpp
  BaseClientProxy.h
  ClientListener.cpp
  ClientListener.h
  ClientProxy.cpp
  ClientProxy.h
  ClientProxy1_0.cpp
  ClientProxy1_0.h
  ClientProxy1_1.cpp
  ClientProxy1_1.h
  ClientProxy1_2.cpp
  ClientProxy1_2.h
  ClientProxy1_3.cpp
  ClientProxy1_3.h
  ClientProxy1_4.cpp
  ClientProxy1_4.h
  ClientProxy1_5.cpp
  ClientProxy1_5.h
  ClientProxy1_6.cpp
  ClientProxy1_6.h
  ClientProxy1_7.cpp
  ClientProxy1_7.h
  ClientProxy1_8.cpp
  ClientProxy1_8.h
  ClientProxyUnknown.cpp
  ClientProxyUnknown.h
  Config.cpp
  Config.h
  InputFilter.cpp
  InputFilter.h
  PrimaryClient.cpp
  PrimaryClient.h
  Server.cpp
  Server.h
)

target_link_libraries(server PUBLIC common)

if(UNIX)
  target_link_libraries(server PUBLIC app)
endif()
