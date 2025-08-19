# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

if(BUILD_UNIFIED)
  set(CLIENT_BINARY "${CMAKE_PROJECT_NAME}-core")
  set(SERVER_BINARY "${CMAKE_PROJECT_NAME}-core")
else()
  set(CLIENT_BINARY "${CMAKE_PROJECT_NAME}-client")
  set(SERVER_BINARY "${CMAKE_PROJECT_NAME}-server")
endif()

if(WIN32)
  string(APPEND CLIENT_BINARY ".exe")
  string(APPEND SERVER_BINARY ".exe")
endif()

configure_file(Constants.h.in Constants.h @ONLY)

unset(CLIENT_BINARY)
unset(SERVER_BINARY)

add_library(common STATIC
  Common.h
  Settings.h
  Settings.cpp
  QSettingsProxy.cpp
  QSettingsProxy.h
  UrlConstants.h
  ${CMAKE_CURRENT_BINARY_DIR}/Constants.h
)

target_link_libraries(common PUBLIC Qt6::Core)
