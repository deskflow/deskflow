# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

  set(CORE_BINARY "${CMAKE_PROJECT_NAME}-core")
if(WIN32)
  string(APPEND CORE_BINARY ".exe")
endif()

configure_file(Constants.h.in Constants.h @ONLY)

unset(CLIENT_BINARY)
unset(SERVER_BINARY)

add_library(common STATIC
  Common.h
  ExitCodes.h
  I18N.h
  I18N.cpp
  PlatformInfo.h
  Settings.h
  Settings.cpp
  QSettingsProxy.cpp
  QSettingsProxy.h
  UrlConstants.h
  ${CMAKE_CURRENT_BINARY_DIR}/Constants.h
)

target_link_libraries(common PUBLIC Qt6::Core)
