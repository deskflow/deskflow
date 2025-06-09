# SPDX-FileCopyrightText: 2019 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

find_package(Doxygen QUIET)
option(BUILD_USER_DOCS "Build and install user documentation" ${DOXYGEN_FOUND})
option(BUILD_DEV_DOCS "Build and install developer documentation" OFF)

if (DOXYGEN_FOUND)
  # Generic Doxygen options
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_QUIET YES)
  set(DOXYGEN_PROJECT_NAME ${CMAKE_PROJECT_PROPER_NAME})

  if (BUILD_USER_DOCS)
    add_subdirectory(user)
  endif()
  if (BUILD_DEV_DOCS)
    add_subdirectory(dev)
  endif()
else()
  message(STATUS "Doxygen not found, skipping docs build")
endif()
