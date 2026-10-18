# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(config)
add_subdirectory(core)

if(APPLE)
  create_test(
    NAME MacPreferencesTests
    DEPENDS gui
    LIBS Qt::Widgets
    SOURCE MacPreferencesTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
  )
  set_tests_properties(MacPreferencesTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  target_include_directories(MacPreferencesTests PRIVATE "${CMAKE_SOURCE_DIR}/src/lib/gui")
  create_test(
    NAME MacSmoothModeMenuTests
    DEPENDS gui
    LIBS Qt::Widgets
    SOURCE MacSmoothModeMenuTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
  )
  set_tests_properties(MacSmoothModeMenuTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  create_test(
    NAME ServerConfigDialogTests
    DEPENDS gui
    LIBS Qt::Widgets
    SOURCE ServerConfigDialogTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
  )
  target_include_directories(ServerConfigDialogTests PRIVATE "${CMAKE_SOURCE_DIR}/src/lib/gui")
  set_tests_properties(ServerConfigDialogTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

create_test(
  NAME LoggerTests
  DEPENDS gui
  SOURCE LoggerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
