
#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# This module is shared by multiple languages; use include blocker.
if(__DARWIN_COMPILER_GNU)
  return()
endif()
set(__DARWIN_COMPILER_GNU 1)

macro(__darwin_compiler_gnu lang)
  # GNU does not have -shared on OS X
  set(CMAKE_SHARED_LIBRARY_CREATE_${lang}_FLAGS "-dynamiclib -headerpad_max_install_names")
  set(CMAKE_SHARED_MODULE_CREATE_${lang}_FLAGS "-bundle -headerpad_max_install_names")
endmacro()

macro(cmake_gnu_has_isysroot lang)
  if("x${CMAKE_${lang}_HAS_ISYSROOT}" STREQUAL "x")
    set(_doc "${lang} compiler has -isysroot")
    message(STATUS "Checking whether ${_doc}")
    execute_process(
      COMMAND ${CMAKE_${lang}_COMPILER} "-v" "--help"
      OUTPUT_VARIABLE _gcc_help
      ERROR_VARIABLE _gcc_help
      )
    if("${_gcc_help}" MATCHES "isysroot")
      message(STATUS "Checking whether ${_doc} - yes")
      set(CMAKE_${lang}_HAS_ISYSROOT 1)
    else()
      message(STATUS "Checking whether ${_doc} - no")
      set(CMAKE_${lang}_HAS_ISYSROOT 0)
    endif()
  endif()
endmacro()
