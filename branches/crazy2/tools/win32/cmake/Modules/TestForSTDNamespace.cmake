# - Test for std:: namespace support
# check if the compiler supports std:: on stl classes
#  CMAKE_NO_STD_NAMESPACE - defined by the results
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

IF("CMAKE_STD_NAMESPACE" MATCHES "^CMAKE_STD_NAMESPACE$")
  MESSAGE(STATUS "Check for STD namespace")
  TRY_COMPILE(CMAKE_STD_NAMESPACE  ${CMAKE_BINARY_DIR} 
    ${CMAKE_ROOT}/Modules/TestForSTDNamespace.cxx
    OUTPUT_VARIABLE OUTPUT)
  IF (CMAKE_STD_NAMESPACE)
    MESSAGE(STATUS "Check for STD namespace - found")
    SET (CMAKE_NO_STD_NAMESPACE 0 CACHE INTERNAL 
         "Does the compiler support std::.")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the CXX compiler has std namespace passed with "
      "the following output:\n${OUTPUT}\n\n")
  ELSE (CMAKE_STD_NAMESPACE)
    MESSAGE(STATUS "Check for STD namespace - not found")
    SET (CMAKE_NO_STD_NAMESPACE 1 CACHE INTERNAL 
       "Does the compiler support std::.")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
      "Determining if the CXX compiler has std namespace failed with "
      "the following output:\n${OUTPUT}\n\n")
  ENDIF (CMAKE_STD_NAMESPACE)
ENDIF("CMAKE_STD_NAMESPACE" MATCHES "^CMAKE_STD_NAMESPACE$")




