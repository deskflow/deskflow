# - Test for compiler support of ANSI sstream header
# check if the compiler supports the standard ANSI sstream header
#  CMAKE_NO_ANSI_STRING_STREAM - defined by the results
#

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
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

IF("CMAKE_HAS_ANSI_STRING_STREAM" MATCHES "^CMAKE_HAS_ANSI_STRING_STREAM$")
  MESSAGE(STATUS "Check for sstream")
  TRY_COMPILE(CMAKE_HAS_ANSI_STRING_STREAM  ${CMAKE_BINARY_DIR} 
    ${CMAKE_ROOT}/Modules/TestForSSTREAM.cxx
    OUTPUT_VARIABLE OUTPUT)
  IF (CMAKE_HAS_ANSI_STRING_STREAM)
    MESSAGE(STATUS "Check for sstream - found")
    SET (CMAKE_NO_ANSI_STRING_STREAM 0 CACHE INTERNAL 
         "Does the compiler support sstream")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the CXX compiler has sstream passed with "
      "the following output:\n${OUTPUT}\n\n")
  ELSE (CMAKE_HAS_ANSI_STRING_STREAM)
    MESSAGE(STATUS "Check for sstream - not found")
    SET (CMAKE_NO_ANSI_STRING_STREAM 1 CACHE INTERNAL 
       "Does the compiler support sstream")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining if the CXX compiler has sstream failed with "
      "the following output:\n${OUTPUT}\n\n")
  ENDIF (CMAKE_HAS_ANSI_STRING_STREAM)
ENDIF("CMAKE_HAS_ANSI_STRING_STREAM" MATCHES "^CMAKE_HAS_ANSI_STRING_STREAM$")




