
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

PROJECT(DumpInformation)

# first get the standard information for th platform
INCLUDE_DIRECTORIES("This does not exists")
GET_DIRECTORY_PROPERTY(incl INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${DumpInformation_BINARY_DIR};${DumpInformation_SOURCE_DIR}")

CONFIGURE_FILE("${CMAKE_ROOT}/Modules/SystemInformation.in" "${RESULT_FILE}")


FILE(APPEND "${RESULT_FILE}" 
  "\n=================================================================\n")
FILE(APPEND "${RESULT_FILE}" 
  "=== VARIABLES\n")
FILE(APPEND "${RESULT_FILE}" 
  "=================================================================\n")
GET_CMAKE_PROPERTY(res VARIABLES)
FOREACH(var ${res})
  FILE(APPEND "${RESULT_FILE}" "${var} \"${${var}}\"\n")
ENDFOREACH(var ${res})

FILE(APPEND "${RESULT_FILE}" 
  "\n=================================================================\n")
FILE(APPEND "${RESULT_FILE}" 
  "=== COMMANDS\n")
FILE(APPEND "${RESULT_FILE}" 
  "=================================================================\n")
GET_CMAKE_PROPERTY(res COMMANDS)
FOREACH(var ${res})
  FILE(APPEND "${RESULT_FILE}" "${var}\n")
ENDFOREACH(var ${res})

FILE(APPEND "${RESULT_FILE}" 
  "\n=================================================================\n")
FILE(APPEND "${RESULT_FILE}" 
  "=== MACROS\n")
FILE(APPEND "${RESULT_FILE}" 
  "=================================================================\n")
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/AllMacros.txt "")
GET_CMAKE_PROPERTY(res MACROS)
FOREACH(var ${res})
  FILE(APPEND "${RESULT_FILE}" "${var}\n")
ENDFOREACH(var ${res})

FILE(APPEND "${RESULT_FILE}" 
  "\n=================================================================\n")
FILE(APPEND "${RESULT_FILE}" 
  "=== OTHER\n")
FILE(APPEND "${RESULT_FILE}" 
  "=================================================================\n")
GET_DIRECTORY_PROPERTY(res INCLUDE_DIRECTORIES)
FOREACH(var ${res})
  FILE(APPEND "${RESULT_FILE}" "INCLUDE_DIRECTORY: ${var}\n")
ENDFOREACH(var)

GET_DIRECTORY_PROPERTY(res LINK_DIRECTORIES)
FOREACH(var ${res})
  FILE(APPEND "${RESULT_FILE}" "LINK_DIRECTORIES: ${var}\n")
ENDFOREACH(var)

GET_DIRECTORY_PROPERTY(res INCLUDE_REGULAR_EXPRESSION)
FILE(APPEND "${RESULT_FILE}" "INCLUDE_REGULAR_EXPRESSION: ${res}\n")

# include other files if they are present, such as when run from within the
# binary tree
MACRO(DUMP_FILE THE_FILE)
  IF (EXISTS "${THE_FILE}")
    FILE(APPEND "${RESULT_FILE}" 
      "\n=================================================================\n")
    FILE(APPEND "${RESULT_FILE}" 
      "=== ${THE_FILE}\n")
    FILE(APPEND "${RESULT_FILE}" 
      "=================================================================\n")
  
    FILE(READ "${THE_FILE}" FILE_CONTENTS LIMIT 50000)
    FILE(APPEND "${RESULT_FILE}" "${FILE_CONTENTS}")
  ENDIF (EXISTS "${THE_FILE}")
ENDMACRO(DUMP_FILE)

DUMP_FILE("../CMakeCache.txt")
DUMP_FILE("../CMakeFiles/CMakeOutput.log")
DUMP_FILE("../CMakeFiles/CMakeError.log")
DUMP_FILE("../CMakeFiles/CMakeSystem.cmake")

FOREACH (EXTRA_FILE ${EXTRA_DUMP_FILES})
  DUMP_FILE("${EXTRA_FILE}")
ENDFOREACH (EXTRA_FILE)

