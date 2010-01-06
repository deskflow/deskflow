# - Check sizeof a type
#  CHECK_TYPE_SIZE(TYPE VARIABLE [BUILTIN_TYPES_ONLY])
# Check if the type exists and determine its size.
# On return, "HAVE_${VARIABLE}" holds the existence of the type,
# and "${VARIABLE}" holds one of the following:
#   <size> = type has non-zero size <size>
#   "0"    = type has arch-dependent size (see below)
#   ""     = type does not exist
# Furthermore, the variable "${VARIABLE}_CODE" holds C preprocessor
# code to define the macro "${VARIABLE}" to the size of the type, or
# leave the macro undefined if the type does not exist.
#
# The variable "${VARIABLE}" may be "0" when CMAKE_OSX_ARCHITECTURES
# has multiple architectures for building OS X universal binaries.
# This indicates that the type size varies across architectures.
# In this case "${VARIABLE}_CODE" contains C preprocessor tests
# mapping from each architecture macro to the corresponding type size.
# The list of architecture macros is stored in "${VARIABLE}_KEYS", and
# the value for each key is stored in "${VARIABLE}-${KEY}".
#
# If the BUILTIN_TYPES_ONLY option is not given, the macro checks for
# headers <sys/types.h>, <stdint.h>, and <stddef.h>, and saves results
# in HAVE_SYS_TYPES_H, HAVE_STDINT_H, and HAVE_STDDEF_H.  The type
# size check automatically includes the available headers, thus
# supporting checks of types defined in the headers.
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link
#  CMAKE_EXTRA_INCLUDE_FILES = list of extra headers to include

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

include(CheckIncludeFile)

cmake_policy(PUSH)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

get_filename_component(__check_type_size_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)

#-----------------------------------------------------------------------------
# Helper function.  DO NOT CALL DIRECTLY.
function(__check_type_size_impl type var map builtin)
  message(STATUS "Check size of ${type}")

  # Include header files.
  set(headers)
  if(builtin)
    if(HAVE_SYS_TYPES_H)
      set(headers "${headers}#include <sys/types.h>\n")
    endif()
    if(HAVE_STDINT_H)
      set(headers "${headers}#include <stdint.h>\n")
    endif()
    if(HAVE_STDDEF_H)
      set(headers "${headers}#include <stddef.h>\n")
    endif()
  endif()
  foreach(h ${CMAKE_EXTRA_INCLUDE_FILES})
    set(headers "${headers}#include \"${h}\"\n")
  endforeach()

  # Perform the check.
  set(src ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckTypeSize/${var}.c)
  set(bin ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckTypeSize/${var}.bin)
  configure_file(${__check_type_size_dir}/CheckTypeSize.c.in ${src} @ONLY)
  try_compile(HAVE_${var} ${CMAKE_BINARY_DIR} ${src}
    COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
    CMAKE_FLAGS
      "-DCOMPILE_DEFINITIONS:STRING=${CMAKE_REQUIRED_FLAGS}"
      "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}"
      "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}"
    OUTPUT_VARIABLE output
    COPY_FILE ${bin}
    )

  if(HAVE_${var})
    # The check compiled.  Load information from the binary.
    file(STRINGS ${bin} strings LIMIT_COUNT 10 REGEX "INFO:size")

    # Parse the information strings.
    set(regex_size ".*INFO:size\\[0*([^]]*)\\].*")
    set(regex_key " key\\[([^]]*)\\]")
    set(keys)
    set(code)
    set(mismatch)
    set(first 1)
    foreach(info ${strings})
      if("${info}" MATCHES "${regex_size}")
        # Get the type size.
        string(REGEX REPLACE "${regex_size}" "\\1" size "${info}")
        if(first)
          set(${var} ${size})
        elseif(NOT "${size}" STREQUAL "${${var}}")
          set(mismatch 1)
        endif()
        set(first 0)

        # Get the architecture map key.
        string(REGEX MATCH   "${regex_key}"       key "${info}")
        string(REGEX REPLACE "${regex_key}" "\\1" key "${key}")
        if(key)
          set(code "${code}\nset(${var}-${key} \"${size}\")")
          list(APPEND keys ${key})
        endif()
      endif()
    endforeach()

    # Update the architecture-to-size map.
    if(mismatch AND keys)
      configure_file(${__check_type_size_dir}/CheckTypeSizeMap.cmake.in ${map} @ONLY)
      set(${var} 0)
    else()
      file(REMOVE ${map})
    endif()

    if(mismatch AND NOT keys)
      message(SEND_ERROR "CHECK_TYPE_SIZE found different results, consider setting CMAKE_OSX_ARCHITECTURES or CMAKE_TRY_COMPILE_OSX_ARCHITECTURES to one or no architecture !")
    endif()

    message(STATUS "Check size of ${type} - done")
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining size of ${type} passed with the following output:\n${output}\n\n")
    set(${var} "${${var}}" CACHE INTERNAL "CHECK_TYPE_SIZE: sizeof(${type})")
  else(HAVE_${var})
    # The check failed to compile.
    message(STATUS "Check size of ${type} - failed")
    file(READ ${src} content)
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining size of ${type} failed with the following output:\n${output}\n${src}:\n${content}\n\n")
    set(${var} "" CACHE INTERNAL "CHECK_TYPE_SIZE: ${type} unknown")
    file(REMOVE ${map})
  endif(HAVE_${var})
endfunction()

#-----------------------------------------------------------------------------
macro(CHECK_TYPE_SIZE TYPE VARIABLE)
  # Optionally check for standard headers.
  if("${ARGV2}" STREQUAL "BUILTIN_TYPES_ONLY")
    set(_builtin 0)
  else()
    set(_builtin 1)
    check_include_file(sys/types.h HAVE_SYS_TYPES_H)
    check_include_file(stdint.h HAVE_STDINT_H)
    check_include_file(stddef.h HAVE_STDDEF_H)
  endif()

  # Compute or load the size or size map.
  set(${VARIABLE}_KEYS)
  set(_map_file ${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/CheckTypeSize/${VARIABLE}.cmake)
  if(NOT DEFINED HAVE_${VARIABLE})
    __check_type_size_impl(${TYPE} ${VARIABLE} ${_map_file} ${_builtin})
  endif()
  include(${_map_file} OPTIONAL)
  set(_map_file)
  set(_builtin)

  # Create preprocessor code.
  if(${VARIABLE}_KEYS)
    set(${VARIABLE}_CODE)
    set(_if if)
    foreach(key ${${VARIABLE}_KEYS})
      set(${VARIABLE}_CODE "${${VARIABLE}_CODE}#${_if} defined(${key})\n# define ${VARIABLE} ${${VARIABLE}-${key}}\n")
      set(_if elif)
    endforeach()
    set(${VARIABLE}_CODE "${${VARIABLE}_CODE}#else\n# error ${VARIABLE} unknown\n#endif")
    set(_if)
  elseif(${VARIABLE})
    set(${VARIABLE}_CODE "#define ${VARIABLE} ${${VARIABLE}}")
  else()
    set(${VARIABLE}_CODE "/* #undef ${VARIABLE} */")
  endif()
endmacro()

#-----------------------------------------------------------------------------
cmake_policy(POP)
