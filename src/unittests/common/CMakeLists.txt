# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME test_Settings
  DEPENDS common
  SOURCE test_Settings.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/common"
)
