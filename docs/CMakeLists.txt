# SPDX-FileCopyrightText: (C) 2019 - 2026 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT


option(BUILD_DEV_DOCS "Build and install developer documentation" OFF)
if (BUILD_DEV_DOCS)
  add_subdirectory(dev)
endif()

# Show our documents in the IDE
add_custom_target(docs ALL
  SOURCES
    Readme.md
    Security.md
    HelpMain.md
    Configuration.md
    Issues.md
)

install(
  FILES
    Readme.md
    HelpMain.md
    Configuration.md
    Issues.md
    Security.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Make sure that help docs are moved.
add_custom_command(
    TARGET docs PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/HelpMain.md
      ${CMAKE_CURRENT_BINARY_DIR}/HelpMain.md
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.md
      ${CMAKE_CURRENT_BINARY_DIR}/Configuration.md
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/Issues.md
      ${CMAKE_CURRENT_BINARY_DIR}/Issues.md
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/Security.md
      ${CMAKE_CURRENT_BINARY_DIR}/Security.md
)

