# SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
# SPDX-License-Identifier: MIT

create_test(
  NAME IpcClientTests
  DEPENDS gui
  SOURCE IpcClientTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
