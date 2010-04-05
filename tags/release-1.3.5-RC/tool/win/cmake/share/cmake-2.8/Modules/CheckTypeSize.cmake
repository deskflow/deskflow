# - Check sizeof a type
#  CHECK_TYPE_SIZE(TYPE VARIABLE [BUILTIN_TYPES_ONLY])
# Check if the type exists and determine size of type.  if the type
# exists, the size will be stored to the variable. This also
# calls check_include_file for sys/types.h stdint.h
# and stddef.h, setting HAVE_SYS_TYPES_H, HAVE_STDINT_H, 
# and HAVE_STDDEF_H.  This is because many types are stored
# in these include files.  
#  VARIABLE - variable to store size if the type exists.
#  HAVE_${VARIABLE} - does the variable exists or not
#  BUILTIN_TYPES_ONLY - The third argument is optional and if 
#                       it is set to the string BUILTIN_TYPES_ONLY
#                       this macro will not check for any header files.
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link

# These variables are referenced in CheckTypeSizeC.c so we have 
# to check for them.

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

include(CheckIncludeFile)

MACRO(CHECK_TYPE_SIZE TYPE VARIABLE)
  IF(NOT "${ARGV2}" STREQUAL "BUILTIN_TYPES_ONLY")
    check_include_file(sys/types.h HAVE_SYS_TYPES_H)
    check_include_file(stdint.h HAVE_STDINT_H)
    check_include_file(stddef.h HAVE_STDDEF_H)
  ENDIF(NOT "${ARGV2}" STREQUAL "BUILTIN_TYPES_ONLY")
    
  IF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
    MESSAGE(STATUS "Check size of ${TYPE}")
    SET(CHECK_TYPE_SIZE_TYPE "${TYPE}")
    SET(MACRO_CHECK_TYPE_SIZE_FLAGS
      "${CMAKE_REQUIRED_FLAGS}")
    FOREACH(def HAVE_SYS_TYPES_H HAVE_STDINT_H HAVE_STDDEF_H)
      IF("${def}")
        SET(MACRO_CHECK_TYPE_SIZE_FLAGS
          "${MACRO_CHECK_TYPE_SIZE_FLAGS} -D${def}")
      ENDIF("${def}")
    ENDFOREACH(def)
    SET(CHECK_TYPE_SIZE_PREINCLUDE)
    SET(CHECK_TYPE_SIZE_PREMAIN)
    SET(CHECK_TYPE_SIZE_ADD_LIBRARIES)
    SET(CHECK_TYPE_SIZE_ADD_INCLUDES)

    FOREACH(def ${CMAKE_EXTRA_INCLUDE_FILES})
      SET(CHECK_TYPE_SIZE_PREMAIN "${CHECK_TYPE_SIZE_PREMAIN}#include \"${def}\"\n")
    ENDFOREACH(def)
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(CHECK_TYPE_SIZE_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ENDIF(CMAKE_REQUIRED_LIBRARIES)
    IF(CMAKE_REQUIRED_INCLUDES)
      SET(CHECK_TYPE_SIZE_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    ENDIF(CMAKE_REQUIRED_INCLUDES)

    CONFIGURE_FILE("${CMAKE_ROOT}/Modules/CheckTypeSizeC.c.in"
      "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTypeSizeC.c" IMMEDIATE @ONLY)
    FILE(READ "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTypeSizeC.c"
      CHECK_TYPE_SIZE_FILE_CONTENT)
    TRY_COMPILE(HAVE_${VARIABLE}
      ${CMAKE_BINARY_DIR}
      "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckTypeSizeC.c"
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_TYPE_SIZE_FLAGS}
      "${CHECK_TYPE_SIZE_ADD_LIBRARIES}"
      "${CHECK_TYPE_SIZE_ADD_INCLUDES}"
      OUTPUT_VARIABLE OUTPUT
      COPY_FILE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckTypeSize.bin" )

    IF(HAVE_${VARIABLE})
      FILE(STRINGS "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CheckTypeSize.bin"
        CMAKE_CHECKTYPESIZE_STRINGS LIMIT_COUNT 2 REGEX "INFO:sizeof")

      SET(CMAKE_CHECKTYPESIZE_FIRST_RESULT "FIRST_LOOP")
      FOREACH(info ${CMAKE_CHECKTYPESIZE_STRINGS})
        IF("${info}" MATCHES ".*INFO:sizeof\\[0*([^]]*)\\].*")
          STRING(REGEX REPLACE ".*INFO:sizeof\\[0*([^]]*)\\].*" "\\1" ${VARIABLE} "${info}")
        ENDIF("${info}" MATCHES ".*INFO:sizeof\\[0*([^]]*)\\].*")
        IF("${CMAKE_CHECKTYPESIZE_FIRST_RESULT}" STREQUAL "FIRST_LOOP")
          SET(CMAKE_CHECKTYPESIZE_FIRST_RESULT "${${VARIABLE}}")
        ENDIF("${CMAKE_CHECKTYPESIZE_FIRST_RESULT}" STREQUAL "FIRST_LOOP")
        IF(NOT "${CMAKE_CHECKTYPESIZE_FIRST_RESULT}" STREQUAL "${${VARIABLE}}")
          MESSAGE(SEND_ERROR "CHECK_TYPE_SIZE found different results, consider setting CMAKE_OSX_ARCHITECTURES or CMAKE_TRY_COMPILE_OSX_ARCHITECTURES to one or no architecture !")
        ENDIF(NOT "${CMAKE_CHECKTYPESIZE_FIRST_RESULT}" STREQUAL "${${VARIABLE}}")

      ENDFOREACH(info ${CMAKE_CHECKTYPESIZE_STRINGS})
      MESSAGE(STATUS "Check size of ${TYPE} - done")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining size of ${TYPE} passed with the following output:\n${OUTPUT}\n\n")
    ELSE(HAVE_${VARIABLE})
      MESSAGE(STATUS "Check size of ${TYPE} - failed")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining size of ${TYPE} failed with the following output:\n${OUTPUT}\nCheckTypeSizeC.c:\n${CHECK_TYPE_SIZE_FILE_CONTENT}\n\n")
      SET(${VARIABLE})
    ENDIF(HAVE_${VARIABLE})
    SET(${VARIABLE} "${${VARIABLE}}" CACHE INTERNAL "Result of CHECK_TYPE_SIZE" FORCE)
  ENDIF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
ENDMACRO(CHECK_TYPE_SIZE)
