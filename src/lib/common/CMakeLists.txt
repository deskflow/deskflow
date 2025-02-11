# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

configure_file(constants.h.in constants.h @ONLY)

add_library(common INTERFACE
  common.h
  IInterface.h
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
  stdvector.h
  ${CMAKE_CURRENT_BINARY_DIR}/constants.h
)

if(APPLE)
  target_sources(common INTERFACE MacOSXPrecomp.h)
endif()

