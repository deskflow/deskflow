# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME ElectionStateTests
  DEPENDS coordination
  SOURCE ElectionStateTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME CoordinationProtocolTests
  DEPENDS coordination
  SOURCE CoordinationProtocolTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)
