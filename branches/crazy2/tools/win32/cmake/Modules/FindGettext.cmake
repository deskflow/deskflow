# - Find GNU gettext tools
# This module looks for the GNU gettext tools. This module defines the 
# following values:
#  GETTEXT_MSGMERGE_EXECUTABLE: the full path to the msgmerge tool.
#  GETTEXT_MSGFMT_EXECUTABLE: the full path to the msgfmt tool.
#  GETTEXT_FOUND: True if gettext has been found.
#
# Additionally it provides the following macros:
# GETTEXT_CREATE_TRANSLATIONS ( outputFile [ALL] file1 ... fileN )
#    This will create a target "translations" which will convert the 
#    given input po files into the binary output mo file. If the 
#    ALL option is used, the translations will also be created when
#    building the default target.

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

FIND_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)

FIND_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE msgfmt)

MACRO(GETTEXT_CREATE_TRANSLATIONS _potFile _firstPoFileArg)
   # make it a real variable, so we can modify it here
   SET(_firstPoFile "${_firstPoFileArg}")

   SET(_gmoFiles)
   GET_FILENAME_COMPONENT(_potBasename ${_potFile} NAME_WE)
   GET_FILENAME_COMPONENT(_absPotFile ${_potFile} ABSOLUTE)

   SET(_addToAll)
   IF(${_firstPoFile} STREQUAL "ALL")
      SET(_addToAll "ALL")
      SET(_firstPoFile)
   ENDIF(${_firstPoFile} STREQUAL "ALL")

   FOREACH (_currentPoFile ${_firstPoFile} ${ARGN})
      GET_FILENAME_COMPONENT(_absFile ${_currentPoFile} ABSOLUTE)
      GET_FILENAME_COMPONENT(_abs_PATH ${_absFile} PATH)
      GET_FILENAME_COMPONENT(_lang ${_absFile} NAME_WE)
      SET(_gmoFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo)

      ADD_CUSTOM_COMMAND( 
         OUTPUT ${_gmoFile} 
         COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s ${_absFile} ${_absPotFile}
         COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_gmoFile} ${_absFile}
         DEPENDS ${_absPotFile} ${_absFile} 
      )

      INSTALL(FILES ${_gmoFile} DESTINATION share/locale/${_lang}/LC_MESSAGES RENAME ${_potBasename}.mo) 
      SET(_gmoFiles ${_gmoFiles} ${_gmoFile})

   ENDFOREACH (_currentPoFile )

   ADD_CUSTOM_TARGET(translations ${_addToAll} DEPENDS ${_gmoFiles})

ENDMACRO(GETTEXT_CREATE_TRANSLATIONS )

IF (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE )
   SET(GETTEXT_FOUND TRUE)
ELSE (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE )
   SET(GETTEXT_FOUND FALSE)
   IF (GetText_REQUIRED)
      MESSAGE(FATAL_ERROR "GetText not found")
   ENDIF (GetText_REQUIRED)
ENDIF (GETTEXT_MSGMERGE_EXECUTABLE AND GETTEXT_MSGFMT_EXECUTABLE )



