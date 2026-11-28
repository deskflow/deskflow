# SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
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
  if(LIBEI_FOUND)
    create_test(
      NAME EiHotKeyModifierMatchingTests
      DEPENDS platform
      LIBS base arch
      SOURCE EiHotKeyModifierMatchingTests.cpp
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
    )

    create_test(
      NAME EiKeyStateTests
      DEPENDS platform
      LIBS base arch
      SOURCE EiKeyStateTests.cpp
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
    )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBEIS QUIET IMPORTED_TARGET "libeis-1.0 >= ${REQUIRED_LIBEI_VERSION}")
    if(LIBEIS_FOUND)
      create_test(
        NAME EiCaptureIntegrationTests
        DEPENDS platform
        LIBS base arch PkgConfig::LIBEIS
        SOURCE EiCaptureIntegrationTests.cpp
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
      )
    else()
      message(STATUS "libeis ${REQUIRED_LIBEI_VERSION}+ unavailable, skipping EiCaptureIntegrationTests")
    endif()
  endif()

  if (BUILD_X11_SUPPORT)
    create_test(
      NAME XWindowsClipboardTests
      DEPENDS platform
      LIBS base arch
      SOURCE XWindowsClipboardTests.cpp
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
    )
  endif()
endif()
