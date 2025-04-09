# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if (WIN32)
  create_test(
    NAME test_MSWindowsClipboard
    DEPENDS platform
    LIBS base arch
    SOURCE test_MSWindowsClipboard.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
elseif(APPLE)
elseif(UNIX)
  create_test(
    NAME test_XWindowsClipboard
    DEPENDS platform
    LIBS base arch
    SOURCE test_XWindowsClipboard.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
  )
endif()
