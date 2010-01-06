
#=============================================================================
# Copyright 2008-2009 Kitware, Inc.
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

# Function to compile a source file to identify the compiler ABI.
# This is used internally by CMake and should not be included by user
# code.

INCLUDE(${CMAKE_ROOT}/Modules/CMakeParseImplicitLinkInfo.cmake)

FUNCTION(CMAKE_DETERMINE_COMPILER_ABI lang src)
  IF(NOT DEFINED CMAKE_DETERMINE_${lang}_ABI_COMPILED)
    MESSAGE(STATUS "Detecting ${lang} compiler ABI info")

    # Compile the ABI identification source.
    SET(BIN "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeDetermineCompilerABI_${lang}.bin")
    TRY_COMPILE(CMAKE_DETERMINE_${lang}_ABI_COMPILED
      ${CMAKE_BINARY_DIR} ${src}
      CMAKE_FLAGS "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_${lang}_VERBOSE_FLAG}"
                  "-DCMAKE_${lang}_STANDARD_LIBRARIES="
      OUTPUT_VARIABLE OUTPUT
      COPY_FILE "${BIN}"
      )

    # Load the resulting information strings.
    IF(CMAKE_DETERMINE_${lang}_ABI_COMPILED)
      MESSAGE(STATUS "Detecting ${lang} compiler ABI info - done")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Detecting ${lang} compiler ABI info compiled with the following output:\n${OUTPUT}\n\n")
      FILE(STRINGS "${BIN}" ABI_STRINGS LIMIT_COUNT 2 REGEX "INFO:[^[]*\\[")
      FOREACH(info ${ABI_STRINGS})
        IF("${info}" MATCHES ".*INFO:sizeof_dptr\\[0*([^]]*)\\].*")
          STRING(REGEX REPLACE ".*INFO:sizeof_dptr\\[0*([^]]*)\\].*" "\\1" ABI_SIZEOF_DPTR "${info}")
        ENDIF("${info}" MATCHES ".*INFO:sizeof_dptr\\[0*([^]]*)\\].*")
        IF("${info}" MATCHES ".*INFO:abi\\[([^]]*)\\].*")
          STRING(REGEX REPLACE ".*INFO:abi\\[([^]]*)\\].*" "\\1" ABI_NAME "${info}")
        ENDIF("${info}" MATCHES ".*INFO:abi\\[([^]]*)\\].*")
      ENDFOREACH(info)

      IF(ABI_SIZEOF_DPTR)
        SET(CMAKE_${lang}_SIZEOF_DATA_PTR "${ABI_SIZEOF_DPTR}" PARENT_SCOPE)
        SET(CMAKE_SIZEOF_VOID_P "${ABI_SIZEOF_DPTR}" PARENT_SCOPE)
      ENDIF(ABI_SIZEOF_DPTR)

      IF(ABI_NAME)
        SET(CMAKE_${lang}_COMPILER_ABI "${ABI_NAME}" PARENT_SCOPE)
        SET(CMAKE_INTERNAL_PLATFORM_ABI "${ABI_NAME}" PARENT_SCOPE)
      ENDIF(ABI_NAME)

      # Parse implicit linker information for this language, if available.
      SET(implicit_dirs "")
      SET(implicit_libs "")
      IF(CMAKE_${lang}_VERBOSE_FLAG
          # Implicit link information cannot be used explicitly for
          # multiple OS X architectures, so we skip it.
          AND NOT "${CMAKE_OSX_ARCHITECTURES}" MATCHES ";"
          # Skip this with Xcode for now.
          AND NOT "${CMAKE_GENERATOR}" MATCHES Xcode)
        CMAKE_PARSE_IMPLICIT_LINK_INFO("${OUTPUT}" implicit_libs implicit_dirs log)
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Parsed ${lang} implicit link information from above output:\n${log}\n\n")
      ENDIF()
      SET(CMAKE_${lang}_IMPLICIT_LINK_LIBRARIES "${implicit_libs}" PARENT_SCOPE)
      SET(CMAKE_${lang}_IMPLICIT_LINK_DIRECTORIES "${implicit_dirs}" PARENT_SCOPE)

    ELSE(CMAKE_DETERMINE_${lang}_ABI_COMPILED)
      MESSAGE(STATUS "Detecting ${lang} compiler ABI info - failed")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Detecting ${lang} compiler ABI info failed to compile with the following output:\n${OUTPUT}\n\n")
    ENDIF(CMAKE_DETERMINE_${lang}_ABI_COMPILED)
  ENDIF(NOT DEFINED CMAKE_DETERMINE_${lang}_ABI_COMPILED)
ENDFUNCTION(CMAKE_DETERMINE_COMPILER_ABI)
