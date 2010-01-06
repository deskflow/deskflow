
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
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
# determine that that selected C compiler can actually compile
# and link the most basic of programs.   If not, a fatal error
# is set and cmake stops processing commands and will not generate
# any makefiles or projects.
IF(NOT CMAKE_C_COMPILER_WORKS)
  MESSAGE(STATUS "Check for working C compiler: ${CMAKE_C_COMPILER}")
  FILE(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testCCompiler.c
    "#ifdef __cplusplus\n"
    "# error \"The CMAKE_C_COMPILER is set to a C++ compiler\"\n"
    "#endif\n"
    "#if defined(__CLASSIC_C__)\n"
    "int main(argc, argv)\n"
    "  int argc;\n"
    "  char* argv[];\n"
    "#else\n"
    "int main(int argc, char* argv[])\n"
    "#endif\n"
    "{ (void)argv; return argc-1;}\n")
  TRY_COMPILE(CMAKE_C_COMPILER_WORKS ${CMAKE_BINARY_DIR} 
    ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testCCompiler.c
    OUTPUT_VARIABLE OUTPUT) 
  SET(C_TEST_WAS_RUN 1)
ENDIF(NOT CMAKE_C_COMPILER_WORKS)

IF(NOT CMAKE_C_COMPILER_WORKS)
  MESSAGE(STATUS "Check for working C compiler: ${CMAKE_C_COMPILER} -- broken")
  FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Determining if the C compiler works failed with "
    "the following output:\n${OUTPUT}\n\n")
  # if the compiler is broken make sure to remove the platform file
  # since Windows-cl configures both c/cxx files both need to be removed
  # when c or c++ fails
  FILE(REMOVE ${CMAKE_PLATFORM_ROOT_BIN}/CMakeCPlatform.cmake )
  FILE(REMOVE ${CMAKE_PLATFORM_ROOT_BIN}/CMakeCXXPlatform.cmake )
  MESSAGE(FATAL_ERROR "The C compiler \"${CMAKE_C_COMPILER}\" "
    "is not able to compile a simple test program.\nIt fails "
    "with the following output:\n ${OUTPUT}\n\n"
    "CMake will not be able to correctly generate this project.")
ELSE(NOT CMAKE_C_COMPILER_WORKS)
  IF(C_TEST_WAS_RUN)
    MESSAGE(STATUS "Check for working C compiler: ${CMAKE_C_COMPILER} -- works")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Determining if the C compiler works passed with "
      "the following output:\n${OUTPUT}\n\n") 
  ENDIF(C_TEST_WAS_RUN)
  SET(CMAKE_C_COMPILER_WORKS 1 CACHE INTERNAL "")

  IF(CMAKE_C_COMPILER_FORCED)
    # The compiler configuration was forced by the user.
    # Assume the user has configured all compiler information.
  ELSE(CMAKE_C_COMPILER_FORCED)
    # Try to identify the ABI and configure it into CMakeCCompiler.cmake
    INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineCompilerABI.cmake)
    CMAKE_DETERMINE_COMPILER_ABI(C ${CMAKE_ROOT}/Modules/CMakeCCompilerABI.c)
    CONFIGURE_FILE(
      ${CMAKE_ROOT}/Modules/CMakeCCompiler.cmake.in
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeCCompiler.cmake
      @ONLY IMMEDIATE # IMMEDIATE must be here for compatibility mode <= 2.0
      )
  ENDIF(CMAKE_C_COMPILER_FORCED)
ENDIF(NOT CMAKE_C_COMPILER_WORKS)

