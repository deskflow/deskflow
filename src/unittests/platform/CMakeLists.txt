# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
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
  if (BUILD_X11_SUPPORT)
    create_test(
      NAME XWindowsClipboardTests
      DEPENDS platform
      LIBS base arch
      SOURCE XWindowsClipboardTests.cpp
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
    )
  endif()

  # Add Wayland clipboard tests when Wayland support is available
  if(LIBEI_FOUND AND LIBPORTAL_FOUND)
    create_test(
      NAME WlClipboardTests
      DEPENDS platform
      LIBS base arch
      SOURCE WlClipboardTests.cpp
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/platform"
    )
    # Keep the red-phase state-machine target out of run_tests until the SUT exists.
    add_executable(WaylandCaptureStateTests WaylandCaptureStateTests.cpp)
    target_link_libraries(WaylandCaptureStateTests platform base arch Qt::Test)
    if (CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
      target_compile_definitions(WaylandCaptureStateTests PUBLIC WORDS_BIGENDIAN=1)
    endif()
  endif()
endif()
