# - Check if the Fortran function exists.
# CHECK_FORTRAN_FUNCTION_EXISTS(FUNCTION VARIABLE)
# - macro which checks if the Fortran function exists
#  FUNCTION - the name of the Fortran function
#  VARIABLE - variable to store the result
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
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

macro(CHECK_FORTRAN_FUNCTION_EXISTS FUNCTION VARIABLE)
  if(NOT DEFINED ${VARIABLE})
    message(STATUS "Looking for Fortran ${FUNCTION}")
    if(CMAKE_REQUIRED_LIBRARIES)
      set(CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    else(CMAKE_REQUIRED_LIBRARIES)
      set(CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    endif(CMAKE_REQUIRED_LIBRARIES)
    FILE(WRITE
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f
    "
      program TESTFortran
      external ${FUNCTION}
      call ${FUNCTION}()
      end program TESTFortran
    "
    )
    try_compile(${VARIABLE}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f
    CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
    OUTPUT_VARIABLE OUTPUT
    )
#    message(STATUS "${OUTPUT}")
    if(${VARIABLE})
      set(${VARIABLE} 1 CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      message(STATUS "Looking for Fortran ${FUNCTION} - found")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
        "Determining if the Fortran ${FUNCTION} exists passed with the following output:\n"
        "${OUTPUT}\n\n")
    else(${VARIABLE})
      message(STATUS "Looking for Fortran ${FUNCTION} - not found")
      set(${VARIABLE} "" CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log 
        "Determining if the Fortran ${FUNCTION} exists failed with the following output:\n"
        "${OUTPUT}\n\n")
    endif(${VARIABLE})
  endif(NOT DEFINED ${VARIABLE})
endmacro(CHECK_FORTRAN_FUNCTION_EXISTS)
