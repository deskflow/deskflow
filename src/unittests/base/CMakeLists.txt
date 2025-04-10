# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME test_Path
  DEPENDS base
  LIBS arch
  SOURCE test_Path.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

create_test(
  NAME test_String
  DEPENDS base
  LIBS arch
  SOURCE test_String.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

create_test(
  NAME test_Unicode
  DEPENDS base
  LIBS arch ${extra_libs}
  SOURCE test_Unicode.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

create_test(
  NAME test_XBase
  DEPENDS base
  LIBS arch
  SOURCE test_XBase.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

