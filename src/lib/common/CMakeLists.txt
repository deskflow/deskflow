# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

configure_file(Constants.h.in Constants.h @ONLY)

add_library(common STATIC
  Common.h
  IInterface.h
  StdExcept.h
  StdIStream.h
  StdOStream.h
  StdSet.h
  Settings.h
  Settings.cpp
  QSettingsProxy.cpp
  QSettingsProxy.h
  ${CMAKE_CURRENT_BINARY_DIR}/Constants.h
)

target_link_libraries(common PUBLIC Qt6::Core)

if(APPLE)
  target_sources(common PUBLIC MacOSXPrecomp.h)
endif()
