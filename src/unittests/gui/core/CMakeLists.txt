# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME CommandProcessTests
  DEPENDS gui
  SOURCE CommandProcessTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME ClientConnectionTests
  DEPENDS gui
  SOURCE ClientConnectionTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
