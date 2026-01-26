# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(client STATIC
  Client.cpp
  Client.h
  ServerProxy.cpp
  ServerProxy.h
)

target_link_libraries(client PUBLIC common)

if(UNIX)
  target_link_libraries(client PUBLIC app io)
endif()
