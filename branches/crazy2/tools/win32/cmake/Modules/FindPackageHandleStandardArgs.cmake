# FIND_PACKAGE_HANDLE_STANDARD_ARGS(NAME (DEFAULT_MSG|"Custom failure message") VAR1 ... )
#    This macro is intended to be used in FindXXX.cmake modules files.
#    It handles the REQUIRED and QUIET argument to FIND_PACKAGE() and
#    it also sets the <UPPERCASED_NAME>_FOUND variable.
#    The package is found if all variables listed are TRUE.
#    Example:
#
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibXml2 DEFAULT_MSG LIBXML2_LIBRARIES LIBXML2_INCLUDE_DIR)
#
#    LibXml2 is considered to be found, if both LIBXML2_LIBRARIES and 
#    LIBXML2_INCLUDE_DIR are valid. Then also LIBXML2_FOUND is set to TRUE.
#    If it is not found and REQUIRED was used, it fails with FATAL_ERROR, 
#    independent whether QUIET was used or not.
#    If it is found, the location is reported using the VAR1 argument, so 
#    here a message "Found LibXml2: /usr/lib/libxml2.so" will be printed out.
#    If the second argument is DEFAULT_MSG, the message in the failure case will 
#    be "Could NOT find LibXml2", if you don't like this message you can specify
#    your own custom failure message there.

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

INCLUDE(FindPackageMessage)
FUNCTION(FIND_PACKAGE_HANDLE_STANDARD_ARGS _NAME _FAIL_MSG _VAR1 )

  IF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    SET(_FAIL_MESSAGE "Could NOT find ${_NAME}")
  ELSE("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    SET(_FAIL_MESSAGE "${_FAIL_MSG}")
  ENDIF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")

  STRING(TOUPPER ${_NAME} _NAME_UPPER)

  # collect all variables which were not found, so they can be printed, so the 
  # user knows better what went wrong (#6375)
  SET(MISSING_VARS "")
  SET(DETAILS "")
  SET(${_NAME_UPPER}_FOUND TRUE)
  IF(NOT ${_VAR1})
    SET(${_NAME_UPPER}_FOUND FALSE)
    SET(MISSING_VARS " ${_VAR1}")
  ELSE(NOT ${_VAR1})
    SET(DETAILS "${DETAILS}[${${_VAR1}}]")
  ENDIF(NOT ${_VAR1})

  # check if all passed variables are valid
  FOREACH(_CURRENT_VAR ${ARGN})
    IF(NOT ${_CURRENT_VAR})
      SET(${_NAME_UPPER}_FOUND FALSE)
      SET(MISSING_VARS "${MISSING_VARS} ${_CURRENT_VAR}")
    ELSE(NOT ${_CURRENT_VAR})
      SET(DETAILS "${DETAILS}[${${_CURRENT_VAR}}]")
    ENDIF(NOT ${_CURRENT_VAR})
  ENDFOREACH(_CURRENT_VAR)

  IF (${_NAME_UPPER}_FOUND)
    FIND_PACKAGE_MESSAGE(${_NAME} "Found ${_NAME}: ${${_VAR1}}" "${DETAILS}")
  ELSE (${_NAME_UPPER}_FOUND)
    IF (${_NAME}_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "${_FAIL_MESSAGE} (missing: ${MISSING_VARS})")
    ELSE (${_NAME}_FIND_REQUIRED)
      IF (NOT ${_NAME}_FIND_QUIETLY)
        MESSAGE(STATUS "${_FAIL_MESSAGE}  (missing: ${MISSING_VARS})")
      ENDIF (NOT ${_NAME}_FIND_QUIETLY)
    ENDIF (${_NAME}_FIND_REQUIRED)
  ENDIF (${_NAME_UPPER}_FOUND)

  SET(${_NAME_UPPER}_FOUND ${${_NAME_UPPER}_FOUND} PARENT_SCOPE)

ENDFUNCTION(FIND_PACKAGE_HANDLE_STANDARD_ARGS)
