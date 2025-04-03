# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

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

create_test(
  NAME UnicodeTests
  DEPENDS base
  LIBS arch ${extra_libs}
  SOURCE UnicodeTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

create_test(
  NAME XBaseTests
  DEPENDS base
  LIBS arch
  SOURCE XBaseTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/base"
)

