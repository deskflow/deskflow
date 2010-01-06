
#=============================================================================
# Copyright 2009 Kitware, Inc.
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

# This file is included in CMakeSystemSpecificInformation.cmake if
# the Eclipse CDT4 extra generator has been selected.

FIND_PROGRAM(CMAKE_ECLIPSE_EXECUTABLE NAMES eclipse DOC "The Eclipse executable")


# The Eclipse generator needs to know the standard include path
# so that Eclipse ca find the headers at runtime and parsing etc. works better
# This is done here by actually running gcc with the options so it prints its
# system include directories, which are parsed then and stored in the cache.
MACRO(_DETERMINE_GCC_SYSTEM_INCLUDE_DIRS _lang _result _resultDefines)
  SET(${_result})
  SET(_gccOutput)
  FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy" "\n" )
  EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -v -E -x ${_lang} -dD dummy
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles
                  ERROR_VARIABLE _gccOutput
                  OUTPUT_VARIABLE _gccStdout )
  FILE(REMOVE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy")

  IF( "${_gccOutput}" MATCHES "> search starts here[^\n]+\n *(.+) *\n *End of (search) list" )
    SET(${_result} ${CMAKE_MATCH_1})
    STRING(REPLACE "\n" " " ${_result} "${${_result}}")
    SEPARATE_ARGUMENTS(${_result})
  ENDIF( "${_gccOutput}" MATCHES "> search starts here[^\n]+\n *(.+) *\n *End of (search) list" )
  
  IF( "${_gccStdout}" MATCHES "built-in>\"\n(.+)# 1 +\"dummy\"" )
    SET(_builtinDefines ${CMAKE_MATCH_1})
    # Remove the '# 1 "<command-line>"' lines
    STRING(REGEX REPLACE "# 1[^\n]+\n" "" _filteredOutput "${_builtinDefines}")
    # Remove the "#define " parts from the output:
    STRING(REGEX REPLACE "#define " "" _defineRemoved "${_filteredOutput}")
    # Replace the line breaks with spaces, so we can use separate arguments afterwards
    STRING(REGEX REPLACE "\n" " " _defineRemoved "${_defineRemoved}")
    # Remove space at the end, this would produce empty list items
    STRING(REGEX REPLACE " +$" "" ${_resultDefines} "${_defineRemoved}")
    SEPARATE_ARGUMENTS(${_resultDefines})
  ENDIF( "${_gccStdout}" MATCHES "built-in>\"\n(.+)# 1 +\"dummy\"" )
ENDMACRO(_DETERMINE_GCC_SYSTEM_INCLUDE_DIRS _lang)

# Save the current LC_ALL, LC_MESSAGES, and LANG environment variables and set them
# to "C" that way GCC's "search starts here" text is in English and we can grok it.
SET(_orig_lc_all      $ENV{LC_ALL})
SET(_orig_lc_messages $ENV{LC_MESSAGES})
SET(_orig_lang        $ENV{LANG})
IF(_orig_lc_all)
  SET(ENV{LC_ALL}      C)
ENDIF(_orig_lc_all)
IF(_orig_lc_messages)
  SET(ENV{LC_MESSAGES} C)
ENDIF(_orig_lc_messages)
IF(_orig_lang)
  SET(ENV{LANG}        C)
ENDIF(_orig_lang)

# Now check for C, works for gcc and Intel compiler at least
IF (NOT CMAKE_ECLIPSE_C_SYSTEM_INCLUDE_DIRS)
  IF ("${CMAKE_C_COMPILER_ID}" MATCHES GNU  OR  "${CMAKE_C_COMPILER_ID}" MATCHES Intel)
    _DETERMINE_GCC_SYSTEM_INCLUDE_DIRS(c _dirs _defines)
    SET(CMAKE_ECLIPSE_C_SYSTEM_INCLUDE_DIRS "${_dirs}" CACHE INTERNAL "C compiler system include directories")
    SET(CMAKE_ECLIPSE_C_SYSTEM_DEFINED_MACROS "${_defines}" CACHE INTERNAL "C compiler system defined macros")
  ENDIF ("${CMAKE_C_COMPILER_ID}" MATCHES GNU  OR  "${CMAKE_C_COMPILER_ID}" MATCHES Intel)
ENDIF (NOT CMAKE_ECLIPSE_C_SYSTEM_INCLUDE_DIRS)

# And now the same for C++
IF (NOT CMAKE_ECLIPSE_CXX_SYSTEM_INCLUDE_DIRS)
  IF ("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU  OR  "${CMAKE_CXX_COMPILER_ID}" MATCHES Intel)
    _DETERMINE_GCC_SYSTEM_INCLUDE_DIRS(c++ _dirs _defines)
    SET(CMAKE_ECLIPSE_CXX_SYSTEM_INCLUDE_DIRS "${_dirs}" CACHE INTERNAL "CXX compiler system include directories")
    SET(CMAKE_ECLIPSE_CXX_SYSTEM_DEFINED_MACROS "${_defines}" CACHE INTERNAL "CXX compiler system defined macros")
  ENDIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES GNU  OR  "${CMAKE_CXX_COMPILER_ID}" MATCHES Intel)
ENDIF (NOT CMAKE_ECLIPSE_CXX_SYSTEM_INCLUDE_DIRS)

# Restore original LC_ALL, LC_MESSAGES, and LANG
IF(_orig_lc_all)
  SET(ENV{LC_ALL}      ${_orig_lc_all})
ENDIF(_orig_lc_all)
IF(_orig_lc_messages)
  SET(ENV{LC_MESSAGES} ${_orig_lc_messages})
ENDIF(_orig_lc_messages)
IF(_orig_lang)
  SET(ENV{LANG}        ${_orig_lang})
ENDIF(_orig_lang)
