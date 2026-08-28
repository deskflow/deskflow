# SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
  # Generic Doxygen options
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_QUIET YES)
  set(DOXYGEN_PROJECT_NAME ${CMAKE_PROJECT_PROPER_NAME})
  set(DOXYGEN_PROJECT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/deskflow-logo.png")
  set(DOXYGEN_PROJECT_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/deskflow-logo.png")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
  set(DOXYGEN_EXCLUDE_PATTERNS "*unittests/*")
  set(DOXYGEN_DOT_GRAPH_MAX_NODES 100)

  # Files used to make our documents
  doxygen_add_docs(dev-docs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    COMMENT "Generating developer documentation" ALL)

  # HACK Only these will show in your IDE
  target_sources(dev-docs PRIVATE
    mainpage.md
    contributing.md
    build.md
    protocol_reference.md
  )
else()
  message(STATUS "Doxygen not found, skipping docs build")
endif()

