# SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(widgets)

create_test(
  NAME LoggerTests
  DEPENDS gui
  SOURCE LoggerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

# The overview fixture contains only presentation state and synthetic peers.
create_test(
  NAME ConnectionHomeTests
  DEPENDS gui
  SOURCE "${CMAKE_SOURCE_DIR}/native-tests/apple-ui/HomeTests.cpp"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
target_sources(ConnectionHomeTests PRIVATE "${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.qrc")
qt_add_lrelease(TS_FILES "${CMAKE_SOURCE_DIR}/translations/deskflow_zh_CN.ts"
  LRELEASE_TARGET ConnectionHomeTestTranslations QM_FILES_OUTPUT_VARIABLE connectionHomeQm)
qt_add_resources(ConnectionHomeTests connection_home_translations PREFIX "/translations"
  BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${connectionHomeQm})
add_dependencies(ConnectionHomeTests ConnectionHomeTestTranslations)
set_tests_properties(ConnectionHomeTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
