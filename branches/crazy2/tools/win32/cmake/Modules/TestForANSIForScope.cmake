# - Check for ANSI for scope support
# Check if the compiler restricts the scope of variables declared in a for-init-statement to the loop body.
#  CMAKE_NO_ANSI_FOR_SCOPE - holds result
#

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

IF("CMAKE_ANSI_FOR_SCOPE" MATCHES "^CMAKE_ANSI_FOR_SCOPE$")
  MESSAGE(STATUS "Check for ANSI scope")
  TRY_COMPILE(CMAKE_ANSI_FOR_SCOPE  ${CMAKE_BINARY_DIR} 
    ${CMAKE_ROOT}/Modules/TestForAnsiForScope.cxx
    OUTPUT_VARIABLE OUTPUT)
  IF (CMAKE_ANSI_FOR_SCOPE)
    MESSAGE(STATUS "Check for ANSI scope - found")
    SET (CMAKE_NO_ANSI_FOR_SCOPE 0 CACHE INTERNAL 
      "Does the compiler support ansi for scope.")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the CXX compiler understands ansi for scopes passed with "
      "the following output:\n${OUTPUT}\n\n")
  ELSE (CMAKE_ANSI_FOR_SCOPE)
    MESSAGE(STATUS "Check for ANSI scope - not found")
    SET (CMAKE_NO_ANSI_FOR_SCOPE 1 CACHE INTERNAL 
      "Does the compiler support ansi for scope.")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining if the CXX compiler understands ansi for scopes failed with "
      "the following output:\n${OUTPUT}\n\n")
  ENDIF (CMAKE_ANSI_FOR_SCOPE)
ENDIF("CMAKE_ANSI_FOR_SCOPE" MATCHES "^CMAKE_ANSI_FOR_SCOPE$")





