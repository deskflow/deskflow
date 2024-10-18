# SPDX-FileCopyrightText: 2019 - 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

find_package(Doxygen)
option(DOCS "Build and Install Documents" ${DOXYGEN_FOUND})

if(DOCS)
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
  set(DOXYGEN_QUIET YES)

  doxygen_add_docs(docs ${CMAKE_SOURCE_DIR}/doc ${CMAKE_SOURCE_DIR}/src
                   COMMENT "Generating Documentation" ALL)

  #This is only to show files used by the documentation
  #within the project tree
  target_sources(docs PRIVATE build.md contributing.md configuration.md
                              mainpage.md)

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT deskflow_docs)
elseif(NOT DOXYGEN_FOUND)
  message(STATUS:"Doxygen Not Found; Documentation will not be built")
endif()
