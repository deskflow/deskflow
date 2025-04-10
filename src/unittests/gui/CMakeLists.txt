# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(core)

create_test(
  NAME test_DotEnv
  DEPENDS gui
  SOURCE test_DotEnv.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME test_Logger
  DEPENDS gui
  SOURCE test_Logger.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
