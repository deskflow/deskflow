
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

# Function to compile a source file to identify the compiler.  This is
# used internally by CMake and should not be included by user code.
# If successful, sets CMAKE_<lang>_COMPILER_ID and CMAKE_<lang>_PLATFORM_ID

FUNCTION(CMAKE_DETERMINE_COMPILER_ID lang flagvar src)
  # Make sure the compiler arguments are clean.
  STRING(STRIP "${CMAKE_${lang}_COMPILER_ARG1}" CMAKE_${lang}_COMPILER_ID_ARG1)

  # Make sure user-specified compiler flags are used.
  IF(CMAKE_${lang}_FLAGS)
    SET(CMAKE_${lang}_COMPILER_ID_FLAGS ${CMAKE_${lang}_FLAGS})
  ELSE(CMAKE_${lang}_FLAGS)
    SET(CMAKE_${lang}_COMPILER_ID_FLAGS $ENV{${flagvar}})
  ENDIF(CMAKE_${lang}_FLAGS)
  STRING(REGEX REPLACE " " ";" CMAKE_${lang}_COMPILER_ID_FLAGS_LIST "${CMAKE_${lang}_COMPILER_ID_FLAGS}")

  # Compute the directory in which to run the test.
  SET(CMAKE_${lang}_COMPILER_ID_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CompilerId${lang})

  # Try building with no extra flags and then try each set
  # of helper flags.  Stop when the compiler is identified.
  FOREACH(flags "" ${CMAKE_${lang}_COMPILER_ID_TEST_FLAGS})
    IF(NOT CMAKE_${lang}_COMPILER_ID)
      CMAKE_DETERMINE_COMPILER_ID_BUILD("${lang}" "${flags}" "${src}")
      FOREACH(file ${COMPILER_${lang}_PRODUCED_FILES})
        CMAKE_DETERMINE_COMPILER_ID_CHECK("${lang}" "${CMAKE_${lang}_COMPILER_ID_DIR}/${file}" "${src}")
      ENDFOREACH(file)
    ENDIF(NOT CMAKE_${lang}_COMPILER_ID)
  ENDFOREACH(flags)

  # if the format is unknown after all files have been checked, put "Unknown" in the cache
  IF(NOT CMAKE_EXECUTABLE_FORMAT)
    SET(CMAKE_EXECUTABLE_FORMAT "Unknown" CACHE INTERNAL "Executable file format")
  ENDIF(NOT CMAKE_EXECUTABLE_FORMAT)

  # Display the final identification result.
  IF(CMAKE_${lang}_COMPILER_ID)
    MESSAGE(STATUS "The ${lang} compiler identification is "
      "${CMAKE_${lang}_COMPILER_ID}")
  ELSE(CMAKE_${lang}_COMPILER_ID)
    MESSAGE(STATUS "The ${lang} compiler identification is unknown")
  ENDIF(CMAKE_${lang}_COMPILER_ID)

  SET(CMAKE_${lang}_COMPILER_ID "${CMAKE_${lang}_COMPILER_ID}" PARENT_SCOPE)
  SET(CMAKE_${lang}_PLATFORM_ID "${CMAKE_${lang}_PLATFORM_ID}" PARENT_SCOPE)
  SET(MSVC_${lang}_ARCHITECTURE_ID "${MSVC_${lang}_ARCHITECTURE_ID}" 
    PARENT_SCOPE)
ENDFUNCTION(CMAKE_DETERMINE_COMPILER_ID)

#-----------------------------------------------------------------------------
# Function to write the compiler id source file.
FUNCTION(CMAKE_DETERMINE_COMPILER_ID_WRITE lang src)
  FILE(READ ${CMAKE_ROOT}/Modules/${src}.in ID_CONTENT_IN)
  STRING(CONFIGURE "${ID_CONTENT_IN}" ID_CONTENT_OUT @ONLY)
  FILE(WRITE ${CMAKE_${lang}_COMPILER_ID_DIR}/${src} "${ID_CONTENT_OUT}")
ENDFUNCTION(CMAKE_DETERMINE_COMPILER_ID_WRITE)

#-----------------------------------------------------------------------------
# Function to build the compiler id source file and look for output
# files.
FUNCTION(CMAKE_DETERMINE_COMPILER_ID_BUILD lang testflags src)
  # Create a clean working directory.
  FILE(REMOVE_RECURSE ${CMAKE_${lang}_COMPILER_ID_DIR})
  FILE(MAKE_DIRECTORY ${CMAKE_${lang}_COMPILER_ID_DIR})
  CMAKE_DETERMINE_COMPILER_ID_WRITE("${lang}" "${src}")

  # Construct a description of this test case.
  SET(COMPILER_DESCRIPTION
    "Compiler: ${CMAKE_${lang}_COMPILER} ${CMAKE_${lang}_COMPILER_ID_ARG1}
Build flags: ${CMAKE_${lang}_COMPILER_ID_FLAGS_LIST}
Id flags: ${testflags}
")

  # Compile the compiler identification source.
  IF(COMMAND EXECUTE_PROCESS)
    EXECUTE_PROCESS(
      COMMAND ${CMAKE_${lang}_COMPILER}
              ${CMAKE_${lang}_COMPILER_ID_ARG1}
              ${CMAKE_${lang}_COMPILER_ID_FLAGS_LIST}
              ${testflags}
              "${src}"
      WORKING_DIRECTORY ${CMAKE_${lang}_COMPILER_ID_DIR}
      OUTPUT_VARIABLE CMAKE_${lang}_COMPILER_ID_OUTPUT
      ERROR_VARIABLE CMAKE_${lang}_COMPILER_ID_OUTPUT
      RESULT_VARIABLE CMAKE_${lang}_COMPILER_ID_RESULT
      )
  ELSE(COMMAND EXECUTE_PROCESS)
    EXEC_PROGRAM(
      ${CMAKE_${lang}_COMPILER} ${CMAKE_${lang}_COMPILER_ID_DIR}
      ARGS ${CMAKE_${lang}_COMPILER_ID_ARG1}
           ${CMAKE_${lang}_COMPILER_ID_FLAGS_LIST}
           ${testflags}
           \"${src}\"
      OUTPUT_VARIABLE CMAKE_${lang}_COMPILER_ID_OUTPUT
      RETURN_VALUE CMAKE_${lang}_COMPILER_ID_RESULT
      )
  ENDIF(COMMAND EXECUTE_PROCESS)

  # Check the result of compilation.
  IF(CMAKE_${lang}_COMPILER_ID_RESULT)
    # Compilation failed.
    SET(MSG
      "Compiling the ${lang} compiler identification source file \"${src}\" failed.
${COMPILER_DESCRIPTION}
The output was:
${CMAKE_${lang}_COMPILER_ID_RESULT}
${CMAKE_${lang}_COMPILER_ID_OUTPUT}

")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log "${MSG}")
    #IF(NOT CMAKE_${lang}_COMPILER_ID_ALLOW_FAIL)
    #  MESSAGE(FATAL_ERROR "${MSG}")
    #ENDIF(NOT CMAKE_${lang}_COMPILER_ID_ALLOW_FAIL)

    # No output files should be inspected.
    SET(COMPILER_${lang}_PRODUCED_FILES)
  ELSE(CMAKE_${lang}_COMPILER_ID_RESULT)
    # Compilation succeeded.
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
      "Compiling the ${lang} compiler identification source file \"${src}\" succeeded.
${COMPILER_DESCRIPTION}
The output was:
${CMAKE_${lang}_COMPILER_ID_RESULT}
${CMAKE_${lang}_COMPILER_ID_OUTPUT}

")

    # Find the executable produced by the compiler, try all files in the
    # binary dir.
    FILE(GLOB COMPILER_${lang}_PRODUCED_FILES
      RELATIVE ${CMAKE_${lang}_COMPILER_ID_DIR}
      ${CMAKE_${lang}_COMPILER_ID_DIR}/*)
    LIST(REMOVE_ITEM COMPILER_${lang}_PRODUCED_FILES "${src}")
    FOREACH(file ${COMPILER_${lang}_PRODUCED_FILES})
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Compilation of the ${lang} compiler identification source \""
        "${src}\" produced \"${file}\"\n\n")
    ENDFOREACH(file)

    IF(NOT COMPILER_${lang}_PRODUCED_FILES)
      # No executable was found.
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Compilation of the ${lang} compiler identification source \""
        "${src}\" did not produce an executable in \""
        "${CMAKE_${lang}_COMPILER_ID_DIR}\".\n\n")
    ENDIF(NOT COMPILER_${lang}_PRODUCED_FILES)
  ENDIF(CMAKE_${lang}_COMPILER_ID_RESULT)

  # Return the files produced by the compilation.
  SET(COMPILER_${lang}_PRODUCED_FILES "${COMPILER_${lang}_PRODUCED_FILES}" PARENT_SCOPE)
ENDFUNCTION(CMAKE_DETERMINE_COMPILER_ID_BUILD lang testflags src)

#-----------------------------------------------------------------------------
# Function to extract the compiler id from an executable.
FUNCTION(CMAKE_DETERMINE_COMPILER_ID_CHECK lang file)
  # Look for a compiler id if not yet known.
  IF(NOT CMAKE_${lang}_COMPILER_ID)
    # Read the compiler identification string from the executable file.
    SET(COMPILER_ID)
    SET(PLATFORM_ID)
    FILE(STRINGS ${file}
      CMAKE_${lang}_COMPILER_ID_STRINGS LIMIT_COUNT 3 REGEX "INFO:")
    SET(HAVE_COMPILER_TWICE 0)
    FOREACH(info ${CMAKE_${lang}_COMPILER_ID_STRINGS})
      IF("${info}" MATCHES ".*INFO:compiler\\[([^]\"]*)\\].*")
        IF(COMPILER_ID)
          SET(COMPILER_ID_TWICE 1)
        ENDIF(COMPILER_ID)
        STRING(REGEX REPLACE ".*INFO:compiler\\[([^]]*)\\].*" "\\1"
          COMPILER_ID "${info}")
      ENDIF("${info}" MATCHES ".*INFO:compiler\\[([^]\"]*)\\].*")
      IF("${info}" MATCHES ".*INFO:platform\\[([^]\"]*)\\].*")
        STRING(REGEX REPLACE ".*INFO:platform\\[([^]]*)\\].*" "\\1"
          PLATFORM_ID "${info}")
      ENDIF("${info}" MATCHES ".*INFO:platform\\[([^]\"]*)\\].*")
      IF("${info}" MATCHES ".*INFO:arch\\[([^]\"]*)\\].*")
        STRING(REGEX REPLACE ".*INFO:arch\\[([^]]*)\\].*" "\\1"
          ARCHITECTURE_ID "${info}")
      ENDIF("${info}" MATCHES ".*INFO:arch\\[([^]\"]*)\\].*")
    ENDFOREACH(info)

    # Check if a valid compiler and platform were found.
    IF(COMPILER_ID AND NOT COMPILER_ID_TWICE)
      SET(CMAKE_${lang}_COMPILER_ID "${COMPILER_ID}")
      SET(CMAKE_${lang}_PLATFORM_ID "${PLATFORM_ID}")
      SET(MSVC_${lang}_ARCHITECTURE_ID "${ARCHITECTURE_ID}")
    ENDIF(COMPILER_ID AND NOT COMPILER_ID_TWICE)

    # Check the compiler identification string.
    IF(CMAKE_${lang}_COMPILER_ID)
      # The compiler identification was found.
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "The ${lang} compiler identification is ${CMAKE_${lang}_COMPILER_ID}, found in \""
        "${file}\"\n\n")
    ELSE(CMAKE_${lang}_COMPILER_ID)
      # The compiler identification could not be found.
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "The ${lang} compiler identification could not be found in \""
        "${file}\"\n\n")
    ENDIF(CMAKE_${lang}_COMPILER_ID)
  ENDIF(NOT CMAKE_${lang}_COMPILER_ID)

  # try to figure out the executable format: ELF, COFF, Mach-O
  IF(NOT CMAKE_EXECUTABLE_FORMAT)
    FILE(READ ${file} CMAKE_EXECUTABLE_MAGIC LIMIT 4 HEX)

    # ELF files start with 0x7f"ELF"
    IF("${CMAKE_EXECUTABLE_MAGIC}" STREQUAL "7f454c46")
      SET(CMAKE_EXECUTABLE_FORMAT "ELF" CACHE INTERNAL "Executable file format")
    ENDIF("${CMAKE_EXECUTABLE_MAGIC}" STREQUAL "7f454c46")

#    # COFF (.exe) files start with "MZ"
#    IF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "4d5a....")
#      SET(CMAKE_EXECUTABLE_FORMAT "COFF" CACHE STRING "Executable file format")
#    ENDIF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "4d5a....")
#
#    # Mach-O files start with CAFEBABE or FEEDFACE, according to http://radio.weblogs.com/0100490/2003/01/28.html
#    IF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "cafebabe")
#      SET(CMAKE_EXECUTABLE_FORMAT "MACHO" CACHE STRING "Executable file format")
#    ENDIF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "cafebabe")
#    IF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "feedface")
#      SET(CMAKE_EXECUTABLE_FORMAT "MACHO" CACHE STRING "Executable file format")
#    ENDIF("${CMAKE_EXECUTABLE_MAGIC}" MATCHES "feedface")

  ENDIF(NOT CMAKE_EXECUTABLE_FORMAT)

  # Return the information extracted.
  SET(CMAKE_${lang}_COMPILER_ID "${CMAKE_${lang}_COMPILER_ID}" PARENT_SCOPE)
  SET(CMAKE_${lang}_PLATFORM_ID "${CMAKE_${lang}_PLATFORM_ID}" PARENT_SCOPE)
  SET(MSVC_${lang}_ARCHITECTURE_ID "${MSVC_${lang}_ARCHITECTURE_ID}" 
    PARENT_SCOPE)
  SET(CMAKE_EXECUTABLE_FORMAT "${CMAKE_EXECUTABLE_FORMAT}" PARENT_SCOPE)
ENDFUNCTION(CMAKE_DETERMINE_COMPILER_ID_CHECK lang)
