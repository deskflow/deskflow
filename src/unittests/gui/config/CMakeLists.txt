# SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME ComputerTests
  DEPENDS gui
  SOURCE ComputerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME GuiServerConfigTests
  DEPENDS gui
  SOURCE GuiServerConfigTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
