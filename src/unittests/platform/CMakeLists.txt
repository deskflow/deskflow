# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if (WIN32)
  create_test(
    NAME MSWindowsClipboardTests
    DEPENDS platform
    LIBS base arch
    SOURCE MSWindowsClipboardTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
elseif(APPLE)
  create_test(
    NAME OSXClipboardTests
    DEPENDS platform
    LIBS base arch
    SOURCE OSXClipboardTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
  create_test(
    NAME OSXKeyStateTests
    DEPENDS platform
    LIBS base arch
    SOURCE OSXKeyStateTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
elseif(UNIX)
  create_test(
    NAME XWindowsClipboardTests
    DEPENDS platform
    LIBS base arch
    SOURCE XWindowsClipboardTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
endif()
