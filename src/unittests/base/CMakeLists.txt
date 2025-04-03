# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

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
