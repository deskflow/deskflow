# SPDX-FileCopyrightText: 2019 - 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

find_package(Doxygen QUIET)
option(BUILD_DOCS "Build and install documents" ${DOXYGEN_FOUND})

if (BUILD_DOCS AND DOXYGEN_FOUND)

  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
  set(DOXYGEN_QUIET YES)

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

else()
  message(STATUS "Doxygen not found, skipping docs build")
endif()
