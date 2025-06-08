# SPDX-FileCopyrightText: 2019 - 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# Enable standalone build of docs (independent of the main project build
cmake_minimum_required(VERSION 3.16)
project(DeskFlowDocs)
include(GNUInstallDirs)

if(CMAKE_PROJECT_DESCRIPTION) # Use parent project's info.
  set(DOXYGEN_PROJECT_NAME "${PARENT_PROJECT_NAME}")
  set(DOXYGEN_PROJECT_BRIEF "${CMAKE_PROJECT_DESCRIPTION}")
else()
  # Standalone build.
  set(DOXYGEN_PROJECT_NAME "Deskflow")
  set(DOXYGEN_PROJECT_BRIEF "Mouse and keyboard sharing utility")
endif()

# end standalone

find_package(Doxygen QUIET)
option(BUILD_DOCS "Build and install documents" ${DOXYGEN_FOUND})
option(BUILD_DEV_DOCS "Build developer documentation with source code" OFF)

if (BUILD_DOCS AND DOXYGEN_FOUND)

  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
  set(DOXYGEN_QUIET YES)
  get_filename_component(PROJECT_LOGO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/linux/deskflow.png" ABSOLUTE)
  set(DOXYGEN_PROJECT_LOGO "${PROJECT_LOGO_PATH}")
  set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/doxygen-custom.css")
  set(DOXYGEN_TOC_INCLUDE_HEADINGS 2)

  # Files used to make our documents
  # User facing documents will not include doxy comments in source code
  doxygen_add_docs(user-docs ${CMAKE_SOURCE_DIR}/doc COMMENT "Generating user documentation" ALL)

  # HACK Only these will show in your IDE
  target_sources(user-docs PRIVATE
    mainpage.md
    configuration.md
  )

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT deskflow_docs)
 
  # Developer documentation with source code
  if (BUILD_DEV_DOCS)
    # Additional settings for developer docs
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_INLINE_SOURCES YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_HAVE_DOT NO)
    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_TEMPLATE_RELATIONS YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_INCLUDED_BY_GRAPH YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_GROUP_GRAPHS YES)
    set(DOXYGEN_DIRECTORY_GRAPH YES)
    
    # Include source code directories
    doxygen_add_docs(dev-docs 
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/deskflow
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/client
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/server
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/net
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/io
      ${CMAKE_CURRENT_SOURCE_DIR}/../src/lib/base
      COMMENT "Generating developer documentation with source code")
  endif()

else()
  message(STATUS "Doxygen not found, skipping docs build")
endif()
