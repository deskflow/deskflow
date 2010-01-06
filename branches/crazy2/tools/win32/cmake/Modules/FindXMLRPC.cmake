# - Find xmlrpc
# Find the native XMLRPC headers and libraries.
#  XMLRPC_INCLUDE_DIRS      - where to find xmlrpc.h, etc.
#  XMLRPC_LIBRARIES         - List of libraries when using xmlrpc.
#  XMLRPC_FOUND             - True if xmlrpc found.
# XMLRPC modules may be specified as components for this find module.
# Modules may be listed by running "xmlrpc-c-config".  Modules include:
#  c++            C++ wrapper code
#  libwww-client  libwww-based client
#  cgi-server     CGI-based server
#  abyss-server   ABYSS-based server
# Typical usage:
#  FIND_PACKAGE(XMLRPC REQUIRED libwww-client)

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
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

# First find the config script from which to obtain other values.
FIND_PROGRAM(XMLRPC_C_CONFIG NAMES xmlrpc-c-config)

# Check whether we found anything.
IF(XMLRPC_C_CONFIG)
  SET(XMLRPC_FOUND 1)
ELSE(XMLRPC_C_CONFIG)
  SET(XMLRPC_FOUND 0)
ENDIF(XMLRPC_C_CONFIG)

# Lookup the include directories needed for the components requested.
IF(XMLRPC_FOUND)
  # Use the newer EXECUTE_PROCESS command if it is available.
  IF(COMMAND EXECUTE_PROCESS)
    EXECUTE_PROCESS(
      COMMAND ${XMLRPC_C_CONFIG} ${XMLRPC_FIND_COMPONENTS} --cflags
      OUTPUT_VARIABLE XMLRPC_C_CONFIG_CFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE XMLRPC_C_CONFIG_RESULT
      )
  ELSE(COMMAND EXECUTE_PROCESS)
    EXEC_PROGRAM(${XMLRPC_C_CONFIG} ARGS "${XMLRPC_FIND_COMPONENTS} --cflags"
      OUTPUT_VARIABLE XMLRPC_C_CONFIG_CFLAGS
      RETURN_VALUE XMLRPC_C_CONFIG_RESULT
      )
  ENDIF(COMMAND EXECUTE_PROCESS)

  # Parse the include flags.
  IF("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
    # Convert the compile flags to a CMake list.
    STRING(REGEX REPLACE " +" ";"
      XMLRPC_C_CONFIG_CFLAGS "${XMLRPC_C_CONFIG_CFLAGS}")

    # Look for -I options.
    SET(XMLRPC_INCLUDE_DIRS)
    FOREACH(flag ${XMLRPC_C_CONFIG_CFLAGS})
      IF("${flag}" MATCHES "^-I")
        STRING(REGEX REPLACE "^-I" "" DIR "${flag}")
        FILE(TO_CMAKE_PATH "${DIR}" DIR)
        SET(XMLRPC_INCLUDE_DIRS ${XMLRPC_INCLUDE_DIRS} "${DIR}")
      ENDIF("${flag}" MATCHES "^-I")
    ENDFOREACH(flag)
  ELSE("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
    MESSAGE("Error running ${XMLRPC_C_CONFIG}: [${XMLRPC_C_CONFIG_RESULT}]")
    SET(XMLRPC_FOUND 0)
  ENDIF("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(XMLRPC_FOUND)

# Lookup the libraries needed for the components requested.
IF(XMLRPC_FOUND)
  # Use the newer EXECUTE_PROCESS command if it is available.
  IF(COMMAND EXECUTE_PROCESS)
    EXECUTE_PROCESS(
      COMMAND ${XMLRPC_C_CONFIG} ${XMLRPC_FIND_COMPONENTS} --libs
      OUTPUT_VARIABLE XMLRPC_C_CONFIG_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE XMLRPC_C_CONFIG_RESULT
      )
  ELSE(COMMAND EXECUTE_PROCESS)
    EXEC_PROGRAM(${XMLRPC_C_CONFIG} ARGS "${XMLRPC_FIND_COMPONENTS} --libs"
      OUTPUT_VARIABLE XMLRPC_C_CONFIG_LIBS
      RETURN_VALUE XMLRPC_C_CONFIG_RESULT
      )
  ENDIF(COMMAND EXECUTE_PROCESS)

  # Parse the library names and directories.
  IF("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
    STRING(REGEX REPLACE " +" ";"
      XMLRPC_C_CONFIG_LIBS "${XMLRPC_C_CONFIG_LIBS}")

    # Look for -L flags for directories and -l flags for library names.
    SET(XMLRPC_LIBRARY_DIRS)
    SET(XMLRPC_LIBRARY_NAMES)
    FOREACH(flag ${XMLRPC_C_CONFIG_LIBS})
      IF("${flag}" MATCHES "^-L")
        STRING(REGEX REPLACE "^-L" "" DIR "${flag}")
        FILE(TO_CMAKE_PATH "${DIR}" DIR)
        SET(XMLRPC_LIBRARY_DIRS ${XMLRPC_LIBRARY_DIRS} "${DIR}")
      ELSEIF("${flag}" MATCHES "^-l")
        STRING(REGEX REPLACE "^-l" "" NAME "${flag}")
        SET(XMLRPC_LIBRARY_NAMES ${XMLRPC_LIBRARY_NAMES} "${NAME}")
      ENDIF("${flag}" MATCHES "^-L")
    ENDFOREACH(flag)

    # Search for each library needed using the directories given.
    FOREACH(name ${XMLRPC_LIBRARY_NAMES})
      # Look for this library.
      FIND_LIBRARY(XMLRPC_${name}_LIBRARY
        NAMES ${name}
        HINTS ${XMLRPC_LIBRARY_DIRS}
        )
      MARK_AS_ADVANCED(XMLRPC_${name}_LIBRARY)

      # If any library is not found then the whole package is not found.
      IF(NOT XMLRPC_${name}_LIBRARY)
        SET(XMLRPC_FOUND 0)
      ENDIF(NOT XMLRPC_${name}_LIBRARY)

      # Build an ordered list of all the libraries needed.
      SET(XMLRPC_LIBRARIES ${XMLRPC_LIBRARIES} "${XMLRPC_${name}_LIBRARY}")
    ENDFOREACH(name)
  ELSE("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
    MESSAGE("Error running ${XMLRPC_C_CONFIG}: [${XMLRPC_C_CONFIG_RESULT}]")
    SET(XMLRPC_FOUND 0)
  ENDIF("${XMLRPC_C_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(XMLRPC_FOUND)

# Report the results.
IF(NOT XMLRPC_FOUND)
  SET(XMLRPC_DIR_MESSAGE
    "XMLRPC was not found. Make sure the entries XMLRPC_* are set.")
  IF(NOT XMLRPC_FIND_QUIETLY)
    MESSAGE(STATUS "${XMLRPC_DIR_MESSAGE}")
  ELSE(NOT XMLRPC_FIND_QUIETLY)
    IF(XMLRPC_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "${XMLRPC_DIR_MESSAGE}")
    ENDIF(XMLRPC_FIND_REQUIRED)
  ENDIF(NOT XMLRPC_FIND_QUIETLY)
ENDIF(NOT XMLRPC_FOUND)
