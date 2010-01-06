
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

# determine the compiler to use for Fortran programs
# NOTE, a generator may set CMAKE_Fortran_COMPILER before
# loading this file to force a compiler.
# use environment variable FC first if defined by user, next use 
# the cmake variable CMAKE_GENERATOR_FC which can be defined by a generator
# as a default compiler

IF(NOT CMAKE_Fortran_COMPILER)
  # prefer the environment variable CC
  IF($ENV{FC} MATCHES ".+")
    GET_FILENAME_COMPONENT(CMAKE_Fortran_COMPILER_INIT $ENV{FC} PROGRAM PROGRAM_ARGS CMAKE_Fortran_FLAGS_ENV_INIT)
    IF(CMAKE_Fortran_FLAGS_ENV_INIT)
      SET(CMAKE_Fortran_COMPILER_ARG1 "${CMAKE_Fortran_FLAGS_ENV_INIT}" CACHE STRING "First argument to Fortran compiler")
    ENDIF(CMAKE_Fortran_FLAGS_ENV_INIT)
    IF(EXISTS ${CMAKE_Fortran_COMPILER_INIT})
    ELSE(EXISTS ${CMAKE_Fortran_COMPILER_INIT})
      MESSAGE(FATAL_ERROR "Could not find compiler set in environment variable FC:\n$ENV{FC}.") 
    ENDIF(EXISTS ${CMAKE_Fortran_COMPILER_INIT})
  ENDIF($ENV{FC} MATCHES ".+")
  
  # next try prefer the compiler specified by the generator
  IF(CMAKE_GENERATOR_FC) 
    IF(NOT CMAKE_Fortran_COMPILER_INIT)
      SET(CMAKE_Fortran_COMPILER_INIT ${CMAKE_GENERATOR_FC})
    ENDIF(NOT CMAKE_Fortran_COMPILER_INIT)
  ENDIF(CMAKE_GENERATOR_FC)

  # finally list compilers to try
  IF(CMAKE_Fortran_COMPILER_INIT)
    SET(CMAKE_Fortran_COMPILER_LIST ${CMAKE_Fortran_COMPILER_INIT})
  ELSE(CMAKE_Fortran_COMPILER_INIT)
    # Known compilers:
    #  f77/f90/f95: generic compiler names
    #  g77: GNU Fortran 77 compiler
    #  gfortran: putative GNU Fortran 95+ compiler (in progress)
    #  fort77: native F77 compiler under HP-UX (and some older Crays)
    #  frt: Fujitsu F77 compiler
    #  pgf77/pgf90/pgf95: Portland Group F77/F90/F95 compilers
    #  xlf/xlf90/xlf95: IBM (AIX) F77/F90/F95 compilers
    #  lf95: Lahey-Fujitsu F95 compiler
    #  fl32: Microsoft Fortran 77 "PowerStation" compiler
    #  af77: Apogee F77 compiler for Intergraph hardware running CLIX
    #  epcf90: "Edinburgh Portable Compiler" F90
    #  fort: Compaq (now HP) Fortran 90/95 compiler for Tru64 and Linux/Alpha
    #  ifc: Intel Fortran 95 compiler for Linux/x86
    #  efc: Intel Fortran 95 compiler for IA64
    #
    #  The order is 95 or newer compilers first, then 90, 
    #  then 77 or older compilers, gnu is always last in the group,
    #  so if you paid for a compiler it is picked by default.
    SET(CMAKE_Fortran_COMPILER_LIST
      ifort ifc efc f95 pgf95 lf95 xlf95 fort gfortran gfortran-4 g95 f90
      pgf90 xlf90 epcf90 fort77 frt pgf77 xlf fl32 af77 g77 f77
      )

    # Vendor-specific compiler names.
    SET(_Fortran_COMPILER_NAMES_GNU       gfortran gfortran-4 g95 g77)
    SET(_Fortran_COMPILER_NAMES_Intel     ifort ifc efc)
    SET(_Fortran_COMPILER_NAMES_PGI       pgf95 pgf90 pgf77)
    SET(_Fortran_COMPILER_NAMES_XL        xlf)
    SET(_Fortran_COMPILER_NAMES_VisualAge xlf95 xlf90 xlf)

    # Prefer vendors matching the C and C++ compilers.
    SET(CMAKE_Fortran_COMPILER_LIST
      ${_Fortran_COMPILER_NAMES_${CMAKE_C_COMPILER_ID}}
      ${_Fortran_COMPILER_NAMES_${CMAKE_CXX_COMPILER_ID}}
      ${CMAKE_Fortran_COMPILER_LIST})
    LIST(REMOVE_DUPLICATES CMAKE_Fortran_COMPILER_LIST)
  ENDIF(CMAKE_Fortran_COMPILER_INIT)

  # Look for directories containing the C and C++ compilers.
  SET(_Fortran_COMPILER_HINTS)
  FOREACH(lang C CXX)
    IF(CMAKE_${lang}_COMPILER AND IS_ABSOLUTE "${CMAKE_${lang}_COMPILER}")
      GET_FILENAME_COMPONENT(_hint "${CMAKE_${lang}_COMPILER}" PATH)
      IF(IS_DIRECTORY "${_hint}")
        LIST(APPEND _Fortran_COMPILER_HINTS "${_hint}")
      ENDIF()
      SET(_hint)
    ENDIF()
  ENDFOREACH()

  # Find the compiler.
  IF(_Fortran_COMPILER_HINTS)
    # Prefer directories containing C and C++ compilers.
    LIST(REMOVE_DUPLICATES _Fortran_COMPILER_HINTS)
    FIND_PROGRAM(CMAKE_Fortran_COMPILER
      NAMES ${CMAKE_Fortran_COMPILER_LIST}
      PATHS ${_Fortran_COMPILER_HINTS}
      NO_DEFAULT_PATH
      DOC "Fortran compiler")
  ENDIF()
  FIND_PROGRAM(CMAKE_Fortran_COMPILER NAMES ${CMAKE_Fortran_COMPILER_LIST} DOC "Fortran compiler")
  IF(CMAKE_Fortran_COMPILER_INIT AND NOT CMAKE_Fortran_COMPILER)
    SET(CMAKE_Fortran_COMPILER "${CMAKE_Fortran_COMPILER_INIT}" CACHE FILEPATH "Fortran compiler" FORCE)
  ENDIF(CMAKE_Fortran_COMPILER_INIT AND NOT CMAKE_Fortran_COMPILER)
ELSE(NOT CMAKE_Fortran_COMPILER)
   # we only get here if CMAKE_Fortran_COMPILER was specified using -D or a pre-made CMakeCache.txt
  # (e.g. via ctest) or set in CMAKE_TOOLCHAIN_FILE
  # if CMAKE_Fortran_COMPILER is a list of length 2, use the first item as 
  # CMAKE_Fortran_COMPILER and the 2nd one as CMAKE_Fortran_COMPILER_ARG1

  LIST(LENGTH CMAKE_Fortran_COMPILER _CMAKE_Fortran_COMPILER_LIST_LENGTH)
  IF("${_CMAKE_Fortran_COMPILER_LIST_LENGTH}" EQUAL 2)
    LIST(GET CMAKE_Fortran_COMPILER 1 CMAKE_Fortran_COMPILER_ARG1)
    LIST(GET CMAKE_Fortran_COMPILER 0 CMAKE_Fortran_COMPILER)
  ENDIF("${_CMAKE_Fortran_COMPILER_LIST_LENGTH}" EQUAL 2)

  # if a compiler was specified by the user but without path, 
  # now try to find it with the full path
  # if it is found, force it into the cache, 
  # if not, don't overwrite the setting (which was given by the user) with "NOTFOUND"
  # if the C compiler already had a path, reuse it for searching the CXX compiler
  GET_FILENAME_COMPONENT(_CMAKE_USER_Fortran_COMPILER_PATH "${CMAKE_Fortran_COMPILER}" PATH)
  IF(NOT _CMAKE_USER_Fortran_COMPILER_PATH)
    FIND_PROGRAM(CMAKE_Fortran_COMPILER_WITH_PATH NAMES ${CMAKE_Fortran_COMPILER})
    MARK_AS_ADVANCED(CMAKE_Fortran_COMPILER_WITH_PATH)
    IF(CMAKE_Fortran_COMPILER_WITH_PATH)
      SET(CMAKE_Fortran_COMPILER ${CMAKE_Fortran_COMPILER_WITH_PATH}
        CACHE STRING "Fortran compiler" FORCE)
    ENDIF(CMAKE_Fortran_COMPILER_WITH_PATH)
  ENDIF(NOT _CMAKE_USER_Fortran_COMPILER_PATH)
ENDIF(NOT CMAKE_Fortran_COMPILER)

MARK_AS_ADVANCED(CMAKE_Fortran_COMPILER)  

# Build a small source file to identify the compiler.
IF(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  SET(CMAKE_Fortran_COMPILER_ID_RUN 1)
  SET(CMAKE_Fortran_PLATFORM_ID "Windows")

  # TODO: Set the compiler id.  It is probably MSVC but
  # the user may be using an integrated Intel compiler.
  # SET(CMAKE_Fortran_COMPILER_ID "MSVC")
ENDIF(${CMAKE_GENERATOR} MATCHES "Visual Studio")

IF(NOT CMAKE_Fortran_COMPILER_ID_RUN)
  SET(CMAKE_Fortran_COMPILER_ID_RUN 1)

  # Each entry in this list is a set of extra flags to try
  # adding to the compile line to see if it helps produce
  # a valid identification executable.
  SET(CMAKE_Fortran_COMPILER_ID_TEST_FLAGS
    # Try compiling to an object file only.
    "-c"

    # Intel on windows does not preprocess by default.
    "-fpp"
    )

  # Try to identify the compiler.
  SET(CMAKE_Fortran_COMPILER_ID)
  INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineCompilerId.cmake)
  CMAKE_DETERMINE_COMPILER_ID(Fortran FFLAGS CMakeFortranCompilerId.F)

  # Fall back to old is-GNU test.
  IF(NOT CMAKE_Fortran_COMPILER_ID)
    EXEC_PROGRAM(${CMAKE_Fortran_COMPILER}
      ARGS ${CMAKE_Fortran_COMPILER_ID_FLAGS_LIST} -E "\"${CMAKE_ROOT}/Modules/CMakeTestGNU.c\""
      OUTPUT_VARIABLE CMAKE_COMPILER_OUTPUT RETURN_VALUE CMAKE_COMPILER_RETURN)
    IF(NOT CMAKE_COMPILER_RETURN)
      IF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_GNU.*" )
        SET(CMAKE_Fortran_COMPILER_ID "GNU")
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran compiler is GNU succeeded with "
          "the following output:\n${CMAKE_COMPILER_OUTPUT}\n\n")
      ELSE("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_GNU.*" )
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran compiler is GNU failed with "
          "the following output:\n${CMAKE_COMPILER_OUTPUT}\n\n")
      ENDIF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_GNU.*" )
      IF(NOT CMAKE_Fortran_PLATFORM_ID)
        IF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_MINGW.*" )
          SET(CMAKE_Fortran_PLATFORM_ID "MinGW")
        ENDIF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_MINGW.*" )
        IF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_CYGWIN.*" )
          SET(CMAKE_Fortran_PLATFORM_ID "Cygwin")
        ENDIF("${CMAKE_COMPILER_OUTPUT}" MATCHES ".*THIS_IS_CYGWIN.*" )
      ENDIF(NOT CMAKE_Fortran_PLATFORM_ID)
    ENDIF(NOT CMAKE_COMPILER_RETURN)
  ENDIF(NOT CMAKE_Fortran_COMPILER_ID)

  # Set old compiler and platform id variables.
  IF("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
    SET(CMAKE_COMPILER_IS_GNUG77 1)
  ENDIF("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU")
  IF("${CMAKE_Fortran_PLATFORM_ID}" MATCHES "MinGW")
    SET(CMAKE_COMPILER_IS_MINGW 1)
  ELSEIF("${CMAKE_Fortran_PLATFORM_ID}" MATCHES "Cygwin")
    SET(CMAKE_COMPILER_IS_CYGWIN 1)
  ENDIF("${CMAKE_Fortran_PLATFORM_ID}" MATCHES "MinGW")
ENDIF(NOT CMAKE_Fortran_COMPILER_ID_RUN)

INCLUDE(CMakeFindBinUtils)

# configure variables set in this file for fast reload later on
CONFIGURE_FILE(${CMAKE_ROOT}/Modules/CMakeFortranCompiler.cmake.in
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeFortranCompiler.cmake
  @ONLY IMMEDIATE # IMMEDIATE must be here for compatibility mode <= 2.0
  )
SET(CMAKE_Fortran_COMPILER_ENV_VAR "FC")
