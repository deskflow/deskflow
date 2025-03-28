# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME SettingsTests
  DEPENDS common
  SOURCE SettingsTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/common"
)
