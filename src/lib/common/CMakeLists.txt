# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

configure_file(constants.h.in constants.h @ONLY)

add_library(common STATIC
  Common.h
  IInterface.h
  stddeque.h
  stdexcept.h
  stdistream.h
  stdlist.h
  stdmap.h
  stdostream.h
  stdpost.h
  stdpre.h
  stdset.h
  stdvector.h
  Settings.h
  Settings.cpp
  QSettingsProxy.cpp
  QSettingsProxy.h
  ${CMAKE_CURRENT_BINARY_DIR}/constants.h
)

target_link_libraries(common PUBLIC Qt6::Core)

if(APPLE)
  target_sources(common PUBLIC MacOSXPrecomp.h)
endif()
