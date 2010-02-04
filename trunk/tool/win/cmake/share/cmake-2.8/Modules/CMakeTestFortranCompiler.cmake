
#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
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

# This file is used by EnableLanguage in cmGlobalGenerator to
# determine that that selected Fortran compiler can actually compile
# and link the most basic of programs.   If not, a fatal error
# is set and cmake stops processing commands and will not generate
# any makefiles or projects.
IF(NOT CMAKE_Fortran_COMPILER_WORKS)
  MESSAGE(STATUS "Check for working Fortran compiler: ${CMAKE_Fortran_COMPILER}")
  FILE(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f "
        PROGRAM TESTFortran
        PRINT *, 'Hello'
        END
  ")
  TRY_COMPILE(CMAKE_Fortran_COMPILER_WORKS ${CMAKE_BINARY_DIR} 
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f
    OUTPUT_VARIABLE OUTPUT)
  SET(FORTRAN_TEST_WAS_RUN 1)
ENDIF(NOT CMAKE_Fortran_COMPILER_WORKS)

IF(NOT CMAKE_Fortran_COMPILER_WORKS)
  MESSAGE(STATUS "Check for working Fortran compiler: ${CMAKE_Fortran_COMPILER} -- broken")
  FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Determining if the Fortran compiler works failed with "
    "the following output:\n${OUTPUT}\n\n")
  MESSAGE(FATAL_ERROR "The Fortran compiler \"${CMAKE_Fortran_COMPILER}\" "
    "is not able to compile a simple test program.\nIt fails "
    "with the following output:\n ${OUTPUT}\n\n"
    "CMake will not be able to correctly generate this project.")
ELSE(NOT CMAKE_Fortran_COMPILER_WORKS)
  IF(FORTRAN_TEST_WAS_RUN)
    MESSAGE(STATUS "Check for working Fortran compiler: ${CMAKE_Fortran_COMPILER} -- works")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the Fortran compiler works passed with "
      "the following output:\n${OUTPUT}\n\n")
  ENDIF(FORTRAN_TEST_WAS_RUN)
  SET(CMAKE_Fortran_COMPILER_WORKS 1 CACHE INTERNAL "")

  IF(CMAKE_Fortran_COMPILER_FORCED)
    # The compiler configuration was forced by the user.
    # Assume the user has configured all compiler information.
  ELSE(CMAKE_Fortran_COMPILER_FORCED)
    # Try to identify the ABI and configure it into CMakeFortranCompiler.cmake
    INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineCompilerABI.cmake)
    CMAKE_DETERMINE_COMPILER_ABI(Fortran ${CMAKE_ROOT}/Modules/CMakeFortranCompilerABI.F)

    # Test for Fortran 90 support by using an f90-specific construct.
    IF(NOT DEFINED CMAKE_Fortran_COMPILER_SUPPORTS_F90)
      MESSAGE(STATUS "Checking whether ${CMAKE_Fortran_COMPILER} supports Fortran 90")
      FILE(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompilerF90.f90 "
      PROGRAM TESTFortran90
      stop = 1 ; do while ( stop .eq. 0 ) ; end do
      END PROGRAM TESTFortran90
")
      TRY_COMPILE(CMAKE_Fortran_COMPILER_SUPPORTS_F90 ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompilerF90.f90
        OUTPUT_VARIABLE OUTPUT)
      IF(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
        MESSAGE(STATUS "Checking whether ${CMAKE_Fortran_COMPILER} supports Fortran 90 -- yes")
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran compiler supports Fortran 90 passed with "
          "the following output:\n${OUTPUT}\n\n")
        SET(CMAKE_Fortran_COMPILER_SUPPORTS_F90 1)
      ELSE(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
        MESSAGE(STATUS "Checking whether ${CMAKE_Fortran_COMPILER} supports Fortran 90 -- no")
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the Fortran compiler supports Fortran 90 failed with "
          "the following output:\n${OUTPUT}\n\n")
        SET(CMAKE_Fortran_COMPILER_SUPPORTS_F90 0)
      ENDIF(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
      UNSET(CMAKE_Fortran_COMPILER_SUPPORTS_F90 CACHE)
    ENDIF(NOT DEFINED CMAKE_Fortran_COMPILER_SUPPORTS_F90)

    CONFIGURE_FILE(
      ${CMAKE_ROOT}/Modules/CMakeFortranCompiler.cmake.in
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeFortranCompiler.cmake
      @ONLY IMMEDIATE # IMMEDIATE must be here for compatibility mode <= 2.0
      )
  ENDIF(CMAKE_Fortran_COMPILER_FORCED)
ENDIF(NOT CMAKE_Fortran_COMPILER_WORKS)
