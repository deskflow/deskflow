# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT


add_library(common INTERFACE
  basic_types.h
  common.h
  constants.h
  copyright.h
  IInterface.h
  ipc.h
  stdbitset.h
  stddeque.h
  stdexcept.h
  stdfstream.h
  stdistream.h
  stdlist.h
  stdmap.h
  stdostream.h
  stdpost.h
  stdpre.h
  stdset.h
  stdsstream.h
  stdstring.h
  stdvector.h
  version.h
)

if(APPLE)
  target_sources(common INTERFACE MacOSXPrecomp.h)
endif()

target_include_directories(common
  INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
