# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME PathTests
  DEPENDS base
  LIBS arch
  SOURCE PathTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

create_test(
  NAME StringTests
  DEPENDS base
  LIBS arch
  SOURCE StringTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)
